// ----- InputController.hpp ----- //

#pragma once
#include <mutex> // include for std::mutex + std::lock_guard

/*
 * thread-safe interface for mouse control
 * uses locking mechanism to prevent concurrent mouse movements
 */
class InputController {
    public :
        void move_mouse(int x, int y) const; // stateless no allocation w/ thread safety
    
    private : 
        static std::mutex s_mouseMutex; // shared mutex for all instances
};
