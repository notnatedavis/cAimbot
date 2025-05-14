cmake_minimum_required(VERSION 3.10)
project(AimTrainer)

# Find OpenCV
find_package(OpenCV REQUIRED COMPONENTS
	core
	imgproc
	cudaarithm
	cudafilters
	cudaimgproc
	cudev # added for GpuMat support
)

if(OpenCV_FOUND AND OpenCV_CUDA_FOUND)
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
add_executable(AimTrainer
    main.cpp
    ScreenCapturer.cpp
    TargetDetector.cpp
    InputController.cpp
)

# Link libraries
target_link_libraries(AimTrainer
    ${OpenCV_LIBS}
    d3d11 dxgi  # Windows DirectX
)