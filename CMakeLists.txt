# --- Build Configuration --- #
cmake_minimum_required(VERSION 3.10)
project(cAimbot)

# --- Directory Configuration --- #
# Include headers from include/
include_directories(include)

# --- Platform Detection --- #
if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

# --- OpenCV Configuration --- #
# Find OpenCV with CUDA components optional
set(OpenCV_COMPONENTS
	core
 	imgproc
)

# add CUDA components if not on ARM
if(NOT ARM) # CUDA not available on apple
	list(APPEND OpenCV_COMPONENTS
		cudaarithm # nvidia only
		cudafilters # nvidia only
		cudaimgproc
		cudev
	)
endif()

find_package(OpenCV REQUIRED COMPONENTS ${OpenCV_COMPONENTS})

# --- CUDA Configuration --- #
# CUDA detection + handling
set(USE_CUDA FALSE)
if(Open_CV_CUDA_FOUND AND NOT ARM)
	message(STATUS "CUDA accelerated OpenCV detected")
	set(USE_CUDA TRUE)
	target_compile_definitions(cAimbot PRIVATE USE_CUDA=1)
else()
	message(STATUS "Using CPU-only OpenCV")
 	target_compile_definitions(cAimbot PRIVATE USE_CUDA=0)
endif()

# --- Platform Dependencies --- #
# Windows 
if(WIN32)
	find_package(DirectX REQUIRED)
 	# nvidia cuda sdk (optional)
  	if(USE_CUDA)
   		find_package(CUDA REQUIRED)
     	endif()
endif()

# Linux specific dependencies
if (LINUX)
	find_package(X11 REQUIRED)
endif()

# --- Build Target --- #
# Source files
add_executable(cAimbot
	src/main.cpp
 	src/ScreenCapturer.cpp
  	src/TargetDetector.cpp
   	src/InputController.cpp
)

# --- Linking Configuration --- #
target_link_libraries(cAimbot
	${OpenCV_LIBS}
)

# Platform specific linking
if(WIN32)
    target_link_libraries(cAimbot
        d3d11 
        dxgi
        ${CUDA_LIBRARIES}
    )
    target_compile_definitions(cAimbot PRIVATE OS_WINDOWS=1)
elseif(LINUX)
    target_link_libraries(cAimbot
        X11
    )
    target_compile_definitions(cAimbot PRIVATE OS_LINUX=1)
endif()

# --- Additional Configuration --- #
# other shit here
