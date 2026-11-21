cmake_minimum_required(VERSION 3.21)

include(FetchContent)

# Only the headers are needed (openvr_driver.h); SOURCE_SUBDIR points at a
# nonexistent folder so OpenVR's own CMake project isn't added to the build.
# Pin GIT_TAG to a current OpenVR release tag.
FetchContent_Declare(
    openvr
    GIT_REPOSITORY https://github.com/ValveSoftware/openvr.git
    GIT_TAG        v2.5.1
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  _headers_only
)
FetchContent_MakeAvailable(openvr)

add_library(driver_multitron SHARED src/driver_main.cpp)
target_compile_features(driver_multitron PRIVATE cxx_std_20)
target_include_directories(driver_multitron PRIVATE
    ${openvr_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared)

# Static CRT: the DLL is loaded into vrserver.exe, so don't depend on a
# particular VC++ redistributable being present there.
set_target_properties(driver_multitron PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    PREFIX "")

# Assemble the driver folder SteamVR expects:
#   <config>/steamvr-driver/multitron/{driver.vrdrivermanifest, resources/, bin/win64/driver_multitron.dll}
# Placed beside MultiTron2.exe's output folder so build-installer.bat packages it.
set(DRIVER_OUT "${CMAKE_BINARY_DIR}/$<CONFIG>/steamvr-driver/multitron")
add_custom_command(TARGET driver_multitron POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/multitron" "${DRIVER_OUT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DRIVER_OUT}/bin/win64"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:driver_multitron>" "${DRIVER_OUT}/bin/win64/driver_multitron.dll")
