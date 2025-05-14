///// ScreenCapturer.cpp

#include "ScreenCapturer.hpp"
#include <Windows.h> // enables access to DirectX

/*
 * low latency screen capture
 */
ScreenCapturer::ScreenCapturer() {
    // initialize DirectX + create DXGI factory for graphics adapter
    IDXGIFactory1* factory;
    CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    IDXGIAdapter1* adapter;
    factory->EnumAdapters1(0, &adapter);
    
    // create D3D11 Device w/ minimal feature level
    HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &m_device, nullptr, &m_context);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create D3D11 device");
    }

    // get (primary) monitor output
    IDXGIOutput* output;
    adapter->EnumOutputs(0, &output);
    DXGI_OUTPUT_DESC outputDesc;
    output->GetDesc(&outputDesc);
    const int screen_width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
    const int screen_height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
    
    // initialize duplication interface
    // might need admin access to execute
    IDXGIOutput1* output1;
    output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
    hr = output1->DuplicateOutput(m_device, &m_duplication);
    output1->Release();
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create output duplication");
    }

    // create reusable staging texture (CPU readback)
    // preallocated once to prevent heap allocation during loop
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = screen_width;
    stagingDesc.Height = screen_height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // desktop format (avoids conversion)
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING; // enables CPU acces but disables GPU writes
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    
    hr = m_device->CreateTexture2D(&stagingDesc, nullptr, &m_stagingTex);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create staging texture");
    }
    
    // hardcoded ROI (adjustable values)
    m_roi = cv::Rect( // assuming res is 1920x1080
        760, // x (center - 200)
        340, // y (center - 200)
        400, // width 
        400  // height
    );

    // clamp ROI to screen bounds
    m_roi &= cv::Rect(0, 0, screen_width, screen_height);

    // preallocate ROI buffer once
    m_cropped.create(m_roi.height, m_roi.width, CV_8UC3); // avoids dynamic allocation
}


/*
 * capture single frame and returns as OpenCV Mat for target detection
 */
cv::Mat ScreenCapturer::capture() {
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* resource = nullptr;
    HRESULT hr = m_duplication->AcquireNextFrame(0, &frameInfo, &resource);
    // on 0 timeout enables nonblocking capture , returns last frame
    
    // handle frame acquisition errors
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return m_buffer;
    if (FAILED(hr)) return cv::Mat(); // signal error
    
    // access texture
    ID3D11Texture2D* tex = nullptr;
    resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);

    // reuse existing staging texture
    m_context->CopyResource(m_stagingTex, tex); // GPU-GPU copy to staging texture

    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_stagingTex, 0, D3D11_MAP_READ, 0, &mapped); // gives CPU access via pointer

    if (m_buffer.empty()) {
        D3D11_TEXTURE2D_DESC desc;
        m_stagingTex->GetDesc(&desc);
        m_buffer.create(desc.Height, desc.Width, CV_8UC3);
    }

    // create temp channel from DirectX BGRA data
    // convert to 3 channel BGR for OpenCV
    cv::Mat temp(m_buffer.rows, m_buffer.cols, CV_8UC4, mapped.pData, mapped.RowPitch);
    cv::cvtColor(temp, m_buffer, cv::COLOR_BGRA2BGR);

    // ROI extraction
    if (!m_roi.empty() && m_roi.width <= m_buffer.cols && m_roi.height <= m_buffer.rows) {
        m_buffer(m_roi).copyTo(m_cropped);
    } else {
        m_cropped = cv::Mat();
    }
    
    // cleanup
    m_context->Unmap(m_stagingTex, 0);
    tex->Release();
    resource->Release();
    m_duplication->ReleaseFrame();
    
    return m_cropped;
}

ScreenCapturer::~ScreenCapturer() {
    // cleanup DirectX resources
    if (m_stagingTex) {
        m_stagingTex->Release();
        m_stagingTex = nullptr;
    }
    if (m_duplication) {
        m_duplication->Release();
        m_duplication = nullptr;
    }
    if (m_context) m_context->Release();
    if (m_device) m_device->Release();
}