///// InputController.cpp

#include "InputController.hpp"
#include <Windows.h> // mouse control
#include <cmath>

/*
 * handles calculations & reading/writing movements with low latency
 */
void InputController::move_mouse(int x, int y) const {
    // calculate steps for smooth movement
    POINT cursor;
    GetCursorPos(&cursor);
    // distance to target
    double dx = x - cursor.x;
    double dy = y - cursor.y;
    
    // smoothing params
    const int steps = 10;
    double prev_eased = 0.0;

    for(int i = 1; i <= steps; ++i) {
        double t = (double) i / steps;
        double eased_t = t < 0.5 ? 4*t*t*t : 1 - pow(-2*t + 2, 3) / 2;
        
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE; // relative movement
        // delta calculations w/ smoothing
        // needs to be <LONG> because of how SendInput() takes pixel values
        input.mi.dx = static_cast<LONG>((eased_t - prev_eased) * dx);
        input.mi.dy = static_cast<LONG>((eased_t - prev_eased) * dy);
        prev_eased = eased_t;
        
        SendInput(1, &input, sizeof(INPUT));
        Sleep(5); // modifiable (future)
    }
}