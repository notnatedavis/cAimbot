// ----- ScreenCapturer.hpp ----- //

#pragma once
#include <opencv2/opencv.hpp> // OpenCV's cv::Mat for image storage and processing
#include <dxgi.h> // screen duplication
#include <d3d11.h> // api to interface with GPU
#include <mutex> // important

/*
 * high performance minimal overhead for interfacing with GPU resources w/o latency
 * thread-safe screen capture interface
 * uses mutex protection for shared buffers
 */
class ScreenCapturer {

    public :
        ScreenCapturer(); // initialize DirectX/GPU resources once
        
        // needs to be released so GPU driver doesnt crash
        ~ScreenCapturer(); // release COM objects + GPU resources
        
        cv::Mat capture(); // reuse preallocated buffers (thread-safe capture)
        
    private :
        bool reinitializeDupilcation();

        // DirectX resources
        ID3D11Device* m_device = nullptr; // Direct3D GPU handle
        ID3D11DeviceContext* m_context = nullptr; // command queue
        IDXGIOutputDuplication* m_duplication = nullptr; // screen duplicator
        ID3D11Texture2D* m_stagingTex = nullptr;

        // image buffers
        cv::Rect m_roi; // Region Of Interest coordinates (ROI)
        cv::Mat m_cropped; // preallocated buffer for ROI
        cv::Mat m_buffer; // preallocated matrix (BGRA)

        mutable std::mutex m_bufferMutex; // protects buffer access
};
