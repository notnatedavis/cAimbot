// ----- TargetDetector.hpp ----- //

#pragma once
#include <opencv2/opencv.hpp> // for real time computer vision (needed for low latency)

/*
 * high performance target detection (adheres to modularity)
 */
class TargetDetector {
    public :
        TargetDetector(const cv::Scalar& lower, const cv::Scalar& upper); // params for hues
        cv::Point find_shape_centroid(const cv::Mat& screen) const; // returns pos of detection, const for statelessness
    private :
        cv::Scalar m_lower, m_upper; // border color range (HSV)
        // reusable buffers (avoid per-frame allocations)
        mutable cv::Mat m_resized, m_mask; // CPU buffers
        mutable cv::cuda::GpuMat m_gpu_resized, m_gpu_hsv, m_gpu_mask; // GPU buffers
};
