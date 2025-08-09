// ----- InputController.cpp ----- //

#include "InputController.hpp"
#include <Windows.h> // mouse control
#include <cmath>
#include <mutex> // required for mutex operations

// initialize static mutex (shared accross all instances)
std::mutex InputController::s_mouseMutex;

/*
 * thread-safe mouse movement with smoothing
 * locking ensures only one thread controls mouse at a time
 */
void InputController::move_mouse(int x, int y) const {
    // lock mutex for duration of mouse movement
    std::lock_guard<std::mutex> lock(s_mouseMutex);

    // calculate steps for smooth movement
    POINT cursor;
    GetCursorPos(&cursor); // critical ; must be atomic operation
    
    // distance to target
    double dx = x - cursor.x;
    double dy = y - cursor.y;

    // smoothing params
    const int steps = 10;
    double prev_eased = 0.0;

    for (int i = 1; i <= steps; ++i) {
    
        const double t = static_cast<double>(i) / steps;
        const double eased_t = t < 0.5 ? 4*t*t*t : 1 - pow(-2*t + 2, 3) / 2;
        
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE; // relative movement
        
        // delta calculations w/ smoothing
        // needs to be <LONG> because of how SendInput() takes pixel values
        input.mi.dx = static_cast<LONG>((eased_t - prev_eased) * dx);
        input.mi.dy = static_cast<LONG>((eased_t - prev_eased) * dy);
        prev_eased = eased_t;
        
        SendInput(1, &input, sizeof(INPUT)); // atomic WinAPI call
        Sleep(5); // modifiable (update)
    }
}
