if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions(-DDEBUGFLAGUP)
endif(${CMAKE_BUILD_TYPE} MATCHES "Debug")

include_directories(${CMAKE_SOURCE_DIR}/inc ${CUDA_INCLUDE_DIRS} ../simulation)
add_library(updater
    baseUpdater.cpp
    velocityVerlet.cpp
    energyMinimizerFIRE.cpp
    energyMinimizerAdam.cpp
    energyMinimizerNesterovAG.cpp
    energyMinimizerLoLBFGS.cpp
    energyMinimizerGradientDescent.cpp
    )
cuda_add_library(updaterGPU
    velocityVerlet.cu
    energyMinimizerAdam.cu
    energyMinimizerFIRE.cu
    energyMinimizerNesterovAG.cu
    )
