// ----- SafetyHandler.hpp ----- //

#pragma once
#include <atomic> // thread safe bool flags
#include <Windows.h> // used for keyboard inputs
#include <thread> // thread support
#include <functional> // for callbacks

/*
 * thread safe minimal overhead handler to moniter and toggle aimbot
 * global hotkey support
 */
class SafetyHandler {

    public : 
        // initializes m_active & m_emergency where...
        // (m_active) : aimbot toggle state
        // (m_emergency) : kill switch state
        
        SafetyHandler();
        ~SafetyHandler();

        void start_emergency_listener(); //
        void update(); // for F2 polling
    
        // state getters
        bool is_active() const { return m_active && !m_emergency; }
        bool emergency() const { return m_emergency; }
    
    private :
        void hotkey_thread_func(); // 

        std::atomic<bool> m_active; // start/stop tracking
        std::atomic<bool> m_emergency; // full shutdown
        std::atomic<bool> m_hotkey_running; //
        std::thread m_hotkey_thread; //
    };
