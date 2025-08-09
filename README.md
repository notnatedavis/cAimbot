# C Aimbot

This is a repository containing an Aimbot program entirely written in C focusing on pure performance and execution. (Currently only supports Windows, need to update to replace DirectX capture + windows API with X11/Wayland or ScreenCapture (cross platform library))

_current project structure_  

cAimbot/
 - include/
    - InputController.hpp
    - SafetyHandler.hpp
    - ScreenCapturer.hpp
    - TargetDetector.hpp
 - src/
    - main.cpp
    - InputController.cpp
    - ScreenCapturer.cpp
    - TargetDetector.cpp
 - CMakeLists.txt
 - ReadMe.md
 - .gitignore (update!)

## Table of Contents
- [Introduction](#introduction)
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Usage](#usage)
- [Additional-Information](#Additional-Info)

## Introduction

cAimbot is a high-performance , low-latency aimbot written in C/C++ that hooks into the graphics pipeline and mouse APIs to automatically move the cursor onto a target region detected in real time. Structured modularly such that; 
- ScreenCapturer : grabs desktop image via DirectX11 desktop duplication API
- TargetDetector : scans passed image for a specified HSV color range (target outline color) using OpenCV (w/ optional CUDA)
- InputController : smoothly moves mouse cursor toward detected target
- SafetyHandler : moniters keyboard input (F2 toggle on/of , F10 kill switch) in thread safe way

### Order of Operations :  
1. **Initialization** : 
- TargetDetector is constructed with HSV bounds
- ScreenCapturer sets up a D3D11 device/contect + acquires desktop duplication + preallocates staging textures and OpenCV Mats for fixed ROI (RangeOfInterest)
- InputController and SafetyHandler stand by
2. **Main Loop** : 
- Poll SafetyHandler.update() : toggle via F2
- if (active) :   
2.1 - capturer.capture() grabs latest desktop frame into BGR Mat , cropped to ROI  
2.2 - detector.find_shape_centroid() downsamples + HSV threshold , pass through mask then find largest contour and compute its centroid  
2.3 - input.move_mouse(x,y) retrieves current cursor position, computes delta, sends serires of relative `SendInput()` steps with an easing curve
- if inactive sleep to save CPU
- Loop exists on kill switch / safety trigger and logs report to user + cleans up resources

## Features

- execute C based program for aimbot with toggleable settings and overlay?

## Prerequisites

_Asahi Linux (Fedora)_
- (update)

_Windows_
- C++17 Compiler
- CMake3.10+
- OpenCV(>=4.x)
- CUDA ToolKit
- Windows SDK / DirectX11
- Windows API

_MacOS_
- (update)

## Usage 
review + update all this

_Asahi Linux (Fedora)_
1. `sudo dnf update`
2. `sudo dnf install cmake gcc-c++ git make` # compiler & build tools
3. `sudo dnf install opencv-devel` # OpenCV dev libs
4. `cd ~/projects/cAimbot`
5. `mkdir -p build && cd build`

_Windows_
1. Install : (Visual Studio 2019+ , Windows 10 SDK, CMake 3.10+ [cmake.org] , OpenCV for Windows , (optional) CUDA Toolkit)
2. Configure & Generate : `cd Aimbot`  
`mkdir build && cd build`  
`cmake (update)`

_MacOS_
1. `update`

## Additional-Info

This portion is for logging or storing notes relevent to the project and its scope.

## Current Focus (check off as I go)

Critical Fixes & Stability

1. Memory Management & Resource Leaks

- Fix DXGI resource leaks in `ScreenCapturer` (missing Release() for factory/adapter/output)
- Add error handling in ScreenCapturer::capture() for DXGI_ERROR_ACCESS_LOST (requires reinitialization)
- Validate COM object initialization with SUCCEEDED() checks

2. Thread Safety

- Make InputController::move_mouse() thread-safe (currently non-reentrant)
- Add mutexes for shared resources (e.g., ScreenCapturer buffers)
- Implement atomic flags in main loop for emergency stop

3. Emergency System

- Global hotkey listener (separate thread) for F10 kill switch
- Add stack unwinding for graceful shutdown
- Windows UAC compatibility notes in README

Performance Optimizations

1. Capture Pipeline

- ROI cropping during texture mapping (avoid full-screen conversion)
- Direct BGRA→HSV conversion (eliminate intermediate BGR step)
- DXGI desktop duplication with partial framebuffer updates

2. Detection System

- Dynamic downscaling (adjust based on FPS: 0.5x → 0.8x)
- CUDA-accelerated contour detection (cv::cuda::findContours)
- Background subtraction for moving targets
- Temporal coherence (reuse previous frame's mask)

3. Input Pipeline

- Replace Sleep() with high-precision timers (std::chrono)
- Mouse movement prediction (linear extrapolation)
- Configurable smoothing curves (easing functions)

Core Features

1. Dynamic Targeting

- Kalman filtering for target trajectory prediction
- Multi-contour analysis (closest to crosshair, largest area)
- HSV range auto-calibration (F3 to sample target area)

2. Adaptive ROI

- ROI centering around last detection
- Dynamic sizing based on target velocity
- Manual ROI adjustment via config

3. Input Modes

- Raw input API support (for protected games)
- Absolute/relative mouse mode toggle
- Humanizer module (randomized movement curves) ?

Configuration & Usability

1. Config System

- INI/JSON configuration (color ranges, ROI, hotkeys, smoothing)
- Runtime reloading (F5 to refresh config)
- CLI argument parsing

2. Diagnostic UI

- DirectX overlay (ROI boundaries, target lock indicator)
- Performance metrics (FPS, processing time)
- Detection preview window (debug mode)

3. Calibration Tools

- HSV range tester with sliders
- ROI visual positioning tool
- Mouse sensitivity profiler

Code Quality & Maintenance

1. Platform Abstraction

- Interface classes for input/capture (enable Linux/Wine support)
- CMake options for CUDA/DirectX
- Error code standardization

2. Build System

- Fix target names (cAimbot vs AimTrainer)
- OpenCV CUDA conditional compilation
- CI/CD pipeline (GitHub Actions)

3. Testing

- Unit tests for coordinate transformations
- Capture simulation framework
- Performance benchmarking suite

Anti-Cheat Mitigations

1. Obfuscation

- Randomize window titles/class names
- DirectX hook masking
- Mouse event spoofing (hardware IDs)

2. Behavioral

- Variable activation delays
- "Human" jitter simulation
- Process hollowing techniques

Roadmap

Phase 1 (Stability): Resource leaks, thread safety, config system  
Phase 2 (Performance): CUDA optimization, pipeline refactoring  
Phase 3 (Features): Prediction algorithms, diagnostic UI  
Phase 4 (Stealth): Anti-cheat evasions, driver-level input  