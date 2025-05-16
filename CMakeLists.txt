cmake_minimum_required(VERSION 3.10)
project(cAimbot)

# Include headers from include/
include_directories(include)

# Find OpenCV
find_package(OpenCV REQUIRED COMPONENTS
	core # both
	imgproc # both
	cudaarithm # nvidia only
	cudafilters # nvidia only
	cudaimgproc # nvidia only
	cudev # nvidia only , added for GpuMat support
)

# Check if OpenCV CUDA modules exist (optional)
if(OpenCV_CUDA_FOUND)
    message(STATUS "CUDA-accelerated OpenCV detected")
    target_compile_definitions(AimTrainer PRIVATE USE_CUDA=1)
else()
    message(WARNING "CUDA not available - falling back to CPU")
endif()

# DirectX libraries (Windows only)
if (WIN32)
    find_package(DirectX REQUIRED)
endif()

# Source files
add_executable(cAimbot
    src/main.cpp
    src/ScreenCapturer.cpp
    src/TargetDetector.cpp
    src/InputController.cpp
)

# Link libraries
target_link_libraries(AimTrainer
    ${OpenCV_LIBS}
    d3d11 dxgi  # nvidia only , Windows DirectX
)
