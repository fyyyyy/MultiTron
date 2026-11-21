cmake_minimum_required(VERSION 3.21)

include(FetchContent)

# Headers only (openxr.h, openxr_loader_negotiation.h); SOURCE_SUBDIR keeps the
# SDK's own CMake project out of the build. Pin GIT_TAG to a current release.
FetchContent_Declare(
    openxr_sdk
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG        release-1.0.34
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  _headers_only
)
FetchContent_MakeAvailable(openxr_sdk)

add_library(XR_APILAYER_MULTITRON_stick SHARED src/layer.cpp)
target_compile_features(XR_APILAYER_MULTITRON_stick PRIVATE cxx_std_20)
target_compile_definitions(XR_APILAYER_MULTITRON_stick PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(XR_APILAYER_MULTITRON_stick PRIVATE
    ${openxr_sdk_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared)
set_target_properties(XR_APILAYER_MULTITRON_stick PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    PREFIX "")

# <config>/openxr-layer/{DLL, manifest, install/uninstall scripts}
set(LAYER_OUT "${CMAKE_BINARY_DIR}/$<CONFIG>/openxr-layer")
add_custom_command(TARGET XR_APILAYER_MULTITRON_stick POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${LAYER_OUT}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:XR_APILAYER_MULTITRON_stick>" "${LAYER_OUT}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/XR_APILAYER_MULTITRON_stick.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/install-openxr-layer.ps1"
            "${CMAKE_CURRENT_SOURCE_DIR}/uninstall-openxr-layer.ps1" "${LAYER_OUT}")
