// ----- SafetyHandler.cpp ----- //

#include "SafetyHandler.hpp"
#include <iostream>

/*
 * x
 */
SafetyHandler::SafetyHandler() 
    : m_active(false), m_emergency(false), m_hotkey_running(false) 
{}

/*
 * x
 */
SafetyHandler::~SafetyHandler() {
    m_hotkey_running = false;
    if (m_hotkey_thread.joinable()) {
        // send dummy message to wake thread
        PostThreadMessage(GetThreadId(m_hotkey_thread.native_handle()), WM_QUIT, 0, 0);
        m_hotkey_thread.join();
    }
}

/*
 * x
 */
void SafetyHandler::start_emergency_listener() {
    if (m_hotkey_running) return;
    
    m_hotkey_running = true;
    m_hotkey_thread = std::thread(&SafetyHandler::hotkey_thread_func, this);
}

/*
 * x
 */
void SafetyHandler::hotkey_thread_func() {
    // register F10 as global hotkey
    if (!RegisterHotKey(nullptr, 1, 0, VK_F10)) {
        std::cerr << "[ERROR] Failed to register F10 hotkey. Error: " 
                  << GetLastError() << std::endl;
        return;
    }

    MSG msg;
    while (m_hotkey_running && GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_HOTKEY && msg.wParam == 1) {
            m_emergency = true;
            std::cout << "\nEMERGENCY STOP TRIGGERED VIA HOTKEY\n";
            break;
        }
    }
    
    UnregisterHotKey(nullptr, 1);
}

/*
 * x
 */
void SafetyHandler::update() {
    // toggle with F2 (polling)
    if (GetAsyncKeyState(VK_F2) & 1) m_active = !m_active;
}