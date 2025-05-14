///// TargetDetector.cpp

#include "TargetDetector.hpp"
#include <opencv2/core/cuda.hpp>  // required for CUDA (gpu acceleration)

/*
 * stores HSV color bounds 
 */
TargetDetector::TargetDetector(const cv::Scalar& lower, const cv::Scalar& upper) 
    : m_lower(lower), m_upper(upper) {}

/*
 * low latency color based targeting
 * img downsampling to reduce processing load (update)
 * issue w/ loss of quality (tweak 0.5 -> 0.8~9 ?)
 */
cv::Point TargetDetector::find_shape_centroid(const cv::Mat& screen) const {
    // downsample parameters (adjust via config later)
    const double scale_factor = 0.5; 
    cv::resize(screen, m_resized, cv::Size(), scale_factor, scale_factor, cv::INTER_AREA);

    // GPU Path
    if (cv::cuda::getCudaEnabledDeviceCount() > 0) {
        m_gpu_resized.upload(m_resized); // copies img to GPU mem
        cv::cuda::cvtColor(m_gpu_resized, m_gpu_hsv, cv::COLOR_BGR2HSV); // converts to HSV
        cv::cuda::inRange(m_gpu_hsv, m_lower, m_upper, m_gpu_mask); // creates binary mask w/ color bounds
        m_gpu_mask.download(m_mask);  // bring mask back to CPU
    }

    // CPU Path
    else {
        cv::Mat hsv; // stack allocated matrix
        cv::cvtColor(m_resized, hsv, cv::COLOR_BGR2HSV); // converts to HSV
        cv::inRange(hsv, m_lower, m_upper, m_mask); // creates binary mask w/ color bounds
    }

    // contour detection (should now use properly initialized m_mask)
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(m_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    // RETR_EXTERNAL ignores holes in targets + CHAIN_APPROX_SIMPLE compresses segments (hor/ver)
    
    if (contours.empty()) return cv::Point(-1, -1);

    // find largest contour
    auto largest = std::max_element(contours.begin(), contours.end(),
        [](const auto& a, const auto& b) { return cv::contourArea(a) < cv::contourArea(b); });

    // scale centroid back to original ROI coordinates
    cv::Moments m = cv::moments(*largest); // calculate distribution of contour pixles
    if (m.m00 <= 0.01) return cv::Point(-1, -1);  // add zero-area check
    // multiplication is cheaper than cv::resize upscaling
    return cv::Point(static_cast<int>(m.m10/m.m00 * (1/scale_factor)), static_cast<int>(m.m01/m.m00 * (1/scale_factor)));
}