cmake_minimum_required(VERSION 3.15)

project(WaterStick VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the VST3 SDK path
set(VST3SDK_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vst3sdk)

# Include VST3 SDK
if(NOT EXISTS ${VST3SDK_ROOT_PATH})
    message(FATAL_ERROR "VST3 SDK not found. Please clone it to ${VST3SDK_ROOT_PATH}")
endif()

# Override XCode version check for command line tools
set(SMTG_XCODE_VERSION "15.0" CACHE STRING "Override XCode version check" FORCE)

list(APPEND CMAKE_MODULE_PATH "${VST3SDK_ROOT_PATH}/cmake/modules")
include(SMTG_VST3_SDK)

add_subdirectory(${VST3SDK_ROOT_PATH} EXCLUDE_FROM_ALL)

# Set up compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define the plugin target
set(target WaterStick)

# Create the plugin
smtg_add_vst3plugin(${target}
    source/WaterStick/WaterStickController.cpp
    source/WaterStick/WaterStickController.h
    source/WaterStick/WaterStickProcessor.cpp
    source/WaterStick/WaterStickProcessor.h
    source/WaterStick/WaterStickParameters.h
    source/WaterStick/WaterStickCIDs.h
    source/WaterStick/version.h
    source/WaterStick/factory.cpp
    source/WaterStick/moduleentry.cpp
)

# Setup as VST3 plugin
smtg_target_setup_universal_binary(${target})

# Include directories
target_include_directories(${target} PRIVATE
    include/WaterStick
    source/WaterStick
    ${VST3SDK_ROOT_PATH}
)

# Link with required SDK libraries
target_link_libraries(${target} PRIVATE
    sdk
)