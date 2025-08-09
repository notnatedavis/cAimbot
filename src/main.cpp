// ----- main.cpp ----- //

#include "ScreenCapturer.hpp"
#include "TargetDetector.hpp"
#include "InputController.hpp"
#include "SafetyHandler.hpp"
#include <iostream>
#include <csignal> // signal handling

// global flag for graceful shutdown
std::atomic<bool> g_shutdown_requested(false);

// signal handler for ctrl+C
void signal_handler(int signal) {
    if (signal == SIGINT) {
        g_shutdown_requested = true;
        std::cout << "\nSHUTDOWN REQUEST RECEIVED\n";
    }
}

// low latency control loop for main execution of program
int main() {
    // register signal handler
    std::signal(SIGINT, signal_handler);

    // load color range for borders (e.g., red: HSV 0-10)
    TargetDetector detector(
        cv::Scalar(164, 100, 71), // lower bound (H,S,V)
        cv::Scalar(164, 100, 71) // upper bound (H,S,V)
    );

    // component initialization (order matters)
    ScreenCapturer capturer; // first to claim DXGI resources
    InputController input; // avoid input conflicts
    SafetyHandler safety; // moniter subsystems

    // start global hotkey listener
    safety.start_emergency_listener();

    std::cout << "Press F2 to start/stop\n";
    std::cout << "Press F10 (global) or Ctrl+C to exit\n";

    try {
        while (!safety.emergency() && !g_shutdown_requested) {
            safety.update();

            if (safety.is_active()) {
                cv::Mat screen = capturer.capture();
                if (!screen.empty()) {
                    cv::Point target = detector.find_shape_centroid(screen);
                    if (target.x != -1) {
                        input.move_mouse(target.x, target.y);
                        Sleep(50);
                    }
                }
            } else {
                Sleep(100);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
    }

    std::cout << "Shutting down gracefully...\n";
    // stack unwinding happens automatically via destructors
    return 0;
}