// ----- ScreenCapturer.cpp ----- //

#include "ScreenCapturer.hpp"
#include <Windows.h> // enables access to DirectX
#include <stdexcept> // this does { purpose }  

/*
 * low latency screen capture
 * constructor : initializes DirectX resources and screen duplication
 * throws std::runtime_error on critical failures
 * manages COM object lifetimes with careful release sequencing
 */
ScreenCapturer::ScreenCapturer() {

    // initialize COM pointers (released in reverse order!)
    IDXGIFactory1* factory = nullptr;
    IDXGIAdapter1* adapter = nullptr;
    IDXGIOutput* output = nullptr;
    IDXGIOutput1* output1 = nullptr;

    // creat DXGI factory - entry point to graphics subsystem
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    if (FAILED(hr)) {
        throw std::runtime_error("[DXGI] Failed to create DXGI factory");
    }

    try {
        // get first graphics adapter (GPU)
        hr = factory->EnumAdapters1(0, &adapter);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to enumerate adapter");
        }

        // create D3D11 devie and context - core Direct3D interfaces
        hr = D3D11CreateDevice(
            adapter,                 // graphics adapter
            D3D_DRIVER_TYPE_UNKNOWN, // driver type (hardware-accelerated)
            nullptr,                 // no software rasterizer
            0,                       // no runtime layers
            nullptr,                 // default feature levels
            0,                       // no feature levels array
            D3D11_SDK_VERSION,       // SDK version
            &m_device,               // output device
            nullptr,                 // actual feature level
            &m_context               // device context
        );
        if (FAILED(hr)) {
            throw std::runtime_error("[D3D11] Failed to create D3D11 device");
        }

        // get (primary) monitor output
        hr = adapter->EnumOutputs(0, &output);
        if (FAILED(hr)) {
            throw std::runtime_error("[DXGI] Failed to enumerate output");
        }
        
        // retrieve moniter dimensions
        DXGI_OUTPUT_DESC outputDesc;
        hr = output->GetDesc(&outputDesc);
        if (FAILED(hr)) {
            throw std::runtime_error("[DXGI] Failed to get output description");
        }

        const int screen_width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
        const int screen_height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

        // get DXGIOutput1 interface for duplication
        hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
        if (FAILED(hr)) {
            throw std::runtime_error("[DXGI] Failed to get output1 interface");
        }

        // create screen duplication interface (requires admin priv)
        hr = output1->DuplicateOutput(m_device, &m_duplication);
        if (FAILED(hr)) {
            throw std::runtime_error("[DXGI] Failed to create output duplication");
        }

        // release temporary COM objects
        output1->Release();
        output->Release();
        adapter->Release();
        factory->Release();

        // configure staging texture for CPU readback
        D3D11_TEXTURE2D_DESC stagingDesc = {};
        stagingDesc.Width = screen_width;
        stagingDesc.Height = screen_height;
        stagingDesc.MipLevels = 1;
        stagingDesc.ArraySize = 1;
        stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // desktop color format
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.Usage = D3D11_USAGE_STAGING; // cpu-readable texture
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        // create reusable GPU texture for screen data
        hr = m_device->CreateTexture2D(&stagingDesc, nullptr, &m_stagingTex);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create staging texture");
        }

        // configure Region Of Interest (ROI)

        // default centered 400x400 area (adjust?)
        m_roi = cv::Rect(760, 340, 400, 400); 
        // clamp ROI to screen bounds (prevents out-of-range errors)
        m_roi &= cv::Rect(0, 0, screen_width, screen_height);
        // preallocate OpenCV buffer for cropped ROI
        m_cropped.create(m_roi.height, m_roi.width, CV_8UC3);

    } catch (...) {
        // emergency cleanup if exception occurs mid-initialization
        if (output1) output1->Release();
        if (output) output->Release();
        if (adapter) adapter->Release();
        if (factory) factory->Release();

        // propagate exception to caller
        throw;
    }
}

/*
 * reinitializes duplication interface after display configuration changes
 * returns true if successful, false else
 * called automatically when DXGI_ERROR_ACCESS_LOST occurs
 */
bool ScreenCapturer::reinitializeDuplication() {

    // release existing duplication interface
    if (m_duplication) {
        m_duplication->Release();
        m_duplication = nullptr;
    }

    // get DXGI device from D3D11 device
    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT hr = m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) return false;

    // get parent adapter
    IDXGIAdapter* adapter = nullptr;
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&adapter);
    dxgiDevice->Release();
    if (FAILED(hr)) return false;

    // get primary output
    IDXGIOutput* output = nullptr;
    hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) {
        adapter->Release();
        return false;
    }

    // get DXGIOutput1 for duplication
    IDXGIOutput1* output1 = nullptr;
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
    output->Release();
    adapter->Release();
    if (FAILED(hr)) return false;

    // create new duplication interface
    hr = output1->DuplicateOutput(m_device, &m_duplication);
    output1->Release();

    return SUCCEEDED(hr);
}

/*
 * captures current screen frame and returns cropped ROI as OpenCV Mat
 * returns empty Mat on critical errors
 * implements automatic recovery for display configuration changes
 */
cv::Mat ScreenCapturer::capture() {
    // lock buffer access for thread safety
    std::lock_guard<std::mutex> lock(m_bufferMutex);

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* resource = nullptr;
    
    // attempt to acquire next frame (0ms timeout ; non-blocking)
    HRESULT hr = m_duplication->AcquireNextFrame(0, &frameInfo, &resource);
    
    // handle display configuration changes (res change, moniter error, etc)
    if (hr == DXGI_ERROR_ACCESS_LOST) {
        // attempt to recreate duplication interface
        if (!reinitializeDuplication()) {
            return cv::Mat(); // critical failure
        }

        return m_cropped;  // return last valid frame while recovering
    }

    // timeout means no new frames available
    else if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return m_cropped;  // return  previous frame
    }

    // handle other failures
    else if (FAILED(hr)) {
        return cv::Mat(); // signal error to caller
    }

    // get texture interface from resource
    ID3D11Texture2D* tex = nullptr;
    resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);

    // copy frame data to staging texture (GPU-GPU transfer)
    m_context->CopyResource(m_stagingTex, tex);

    // map staging texture for CPU access
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = m_context->Map(m_stagingTex, 0, D3D11_MAP_READ, 0, &mapped);
    
    // handle mapping failure
    if (FAILED(hr)) {
        // release resources before returning
        tex->Release();
        resource->Release();
        m_duplication->ReleaseFrame();
        return cv::Mat();
    }

    // initialize main buffer on first capture
    if (m_buffer.empty()) {
        D3D11_TEXTURE2D_DESC desc;
        m_stagingTex->GetDesc(&desc);
        m_buffer.create(desc.Height, desc.Width, CV_8UC3);
    }

    // create temporary wrapper for BGRA data
    cv::Mat temp(m_buffer.rows, m_buffer.cols, CV_8UC4, mapped.pData, mapped.RowPitch);
    
    // convert BGRA to BGR (remove alpha channel)
    cv::cvtColor(temp, m_buffer, cv::COLOR_BGRA2BGR);

    // extract ROI if valid
    if (!m_roi.empty() && m_roi.width <= m_buffer.cols && m_roi.height <= m_buffer.rows) {
        m_buffer(m_roi).copyTo(m_cropped);
    } else {
        m_cropped = cv::Mat(); // invalid ROI
    }
    
    // unmap and release resources
    m_context->Unmap(m_stagingTex, 0);
    tex->Release();
    resource->Release();
    m_duplication->ReleaseFrame();
    
    return m_cropped;
}

/*
 * destructor : releasees all DirectX resources
 * release dependent objects first (PLEASE)
 */
ScreenCapturer::~ScreenCapturer() {
    // release in reverse initialization order
    if (m_stagingTex) {
        m_stagingTex->Release();
        m_stagingTex = nullptr;
    }
    if (m_duplication) {
        m_duplication->Release();
        m_duplication = nullptr;
    }
    if (m_context) {
        m_context->Release();
        m_context = nullptr;
    }
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
}
