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
