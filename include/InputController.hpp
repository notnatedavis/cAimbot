///// InputController.hpp

#pragma once

/*
 * minimal stateless interface for mouse control, needs to be called 100+/sec without
 * latency
 */
class InputController {
    public :
        void move_mouse(int x, int y) const; // stateless no allocation
};