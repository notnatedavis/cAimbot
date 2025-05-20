// ----- SafetyHandler.hpp ----- //

#pragma once
#include <atomic> // thread safe bool flags
#include <Windows.h> // used for keyboard inputs

/*
 * thread safe minimal overhead handler to moniter and toggle aimbot
 */
class SafetyHandler {

    public : 
        // initializes m_active & m_emergency
        // (m_active) : aimbot toggle state
        // (m_emergency) : kill switch state
        
        SafetyHandler() : m_active(false), m_emergency(false) {}
        
        // handles input polling and state mangagement
        void update() {
        
            // toggle with F2
            if (GetAsyncKeyState(VK_F2) & 1) m_active = !m_active; // update keys (future)
            
            // emergency stop with F10
            if (GetAsyncKeyState(VK_F10)) m_emergency = true; // update keys (future)
        }
    
        // state getters
        bool is_active() const { return m_active && !m_emergency; }
        bool emergency() const { return m_emergency; }
    
    private :
        // thread safe w/o locks
        std::atomic<bool> m_active; // start/stop tracking
        std::atomic<bool> m_emergency; // full shutdown
    };
