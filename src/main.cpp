// ----- main.cpp ----- //

#include "ScreenCapturer.hpp"
#include "TargetDetector.hpp"
#include "InputController.hpp"
#include "SafetyHandler.hpp"
#include <iostream>

/*
 * low latency control loop for main execution of program
 */
int main() {

    // load color range for borders (e.g., red: HSV 0-10)
    TargetDetector detector(
        cv::Scalar(164, 100, 71), // lower bound (H,S,V)
        cv::Scalar(164, 100, 71) // upper bound (H,S,V)
    );

    // component initialization (order matters)
    ScreenCapturer capturer; // first to claim DXGI resources
    InputController input; // avoid input conflicts
    SafetyHandler safety; // moniter subsystems

    std::cout << "Press F2 to start/stop, F10 to exit\n"; // update keys to custom in SafetyHandler

    while (!safety.emergency()) {
    
        safety.update();

        if (safety.is_active()) {
        
            cv::Mat screen = capturer.capture();

            // empty check
            if (!screen.empty()) {
            
                cv::Point target = detector.find_shape_centroid(screen);
            
                if (target.x != -1) {
                    // move
                    input.move_mouse(target.x, target.y);
                    Sleep(50);  // prevent over-triggering (update future)
                }
            }
        } else {
            Sleep(100);  // reduce CPU usage when inactive (update)
        }
    }

    std::cout << "Emergency stop activated\n";

    return 0;
}
