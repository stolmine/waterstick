cmake_minimum_required(VERSION 3.15)

project(WaterStick VERSION 1.0.0)

# Testing disabled for now

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define build configuration before including VST3 SDK
add_definitions(-DRELEASE=1)

# Enable VST validator for final validation
set(SMTG_RUN_VST_VALIDATOR ON CACHE BOOL "Run VST validator on VST3 Plug-ins" FORCE)

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
    add_compile_definitions(RELEASE=1)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_definitions(RELEASE=1)
endif()

# Define the plugin target
set(target WaterStick)

# Create the plugin
smtg_add_vst3plugin(${target}
    source/WaterStick/WaterStickController.cpp
    source/WaterStick/WaterStickController.h
    source/WaterStick/WaterStickEditor.cpp
    source/WaterStick/WaterStickEditor.h
    source/WaterStick/WaterStickProcessor.cpp
    source/WaterStick/WaterStickProcessor.h
    source/WaterStick/WaterStickParameters.h
    source/WaterStick/WaterStickCIDs.h
    source/WaterStick/ThreeSistersFilter.cpp
    source/WaterStick/ThreeSistersFilter.h
    source/WaterStick/ControlFactory.cpp
    source/WaterStick/ControlFactory.h
    source/WaterStick/DecoupledDelayArchitecture.cpp
    source/WaterStick/DecoupledDelayArchitecture.h
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
    vstgui_support
)

# Code signing configuration
if(SMTG_MAC)
    # Use adhoc signing with custom entitlements to allow plugin loading
    set_target_properties(${target} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/WaterStick.entitlements"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
    )
endif()

# Pitch shifting dropout test program
add_executable(test_pitch_dropout
    test_pitch_dropout.cpp
)

# Set C++17 for test program
set_target_properties(test_pitch_dropout PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Link with threading library for test program
find_package(Threads REQUIRED)
target_link_libraries(test_pitch_dropout PRIVATE Threads::Threads)

# Tests will be added later