cmake_minimum_required(VERSION 3.21)

project(MultiTron2 VERSION 0.1.1 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(vigemclient
    GIT_REPOSITORY https://github.com/nefarius/ViGEmClient.git
    GIT_TAG master
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(vigemclient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT WIN32)
    message(FATAL_ERROR "MultiTron2 currently targets Windows Raw Input and vJoy.")
endif()

add_executable(MultiTron2 WIN32
    src/main.cpp
    src/InterceptionInput.cpp
    src/InterceptionInput.h
    src/VJoyOutput.cpp
    src/VJoyOutput.h
    src/AudioCadenceInput.cpp
    src/AudioCadenceInput.h
    src/XboxOutput.cpp
    src/XboxOutput.h
    installer/MultiTron2.rc
)

target_compile_definitions(MultiTron2 PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
set(ULTRALIGHT_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ultralight-sdk")
if (NOT EXISTS "${ULTRALIGHT_SDK_ROOT}/include/AppCore/App.h")
    message(FATAL_ERROR "Ultralight SDK not found. Extract it to ${ULTRALIGHT_SDK_ROOT}.")
endif()

target_include_directories(MultiTron2 PRIVATE "${ULTRALIGHT_SDK_ROOT}/include" "${vigemclient_SOURCE_DIR}/include")
target_link_directories(MultiTron2 PRIVATE "${ULTRALIGHT_SDK_ROOT}/lib")
target_link_libraries(MultiTron2 PRIVATE user32 comctl32 comdlg32 shell32 winmm setupapi ViGEmClient UltralightCore AppCore Ultralight WebCore)

set(INTERCEPTION_DLL "${CMAKE_CURRENT_SOURCE_DIR}/third_party/interception/x64/interception.dll")
if (EXISTS "${INTERCEPTION_DLL}")
    add_custom_command(TARGET MultiTron2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${INTERCEPTION_DLL}" "$<TARGET_FILE_DIR:MultiTron2>")
endif()

# Ultralight loads its renderer DLLs and WebCore resources beside the executable.
add_custom_command(TARGET MultiTron2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ULTRALIGHT_SDK_ROOT}/bin" "$<TARGET_FILE_DIR:MultiTron2>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ULTRALIGHT_SDK_ROOT}/resources" "$<TARGET_FILE_DIR:MultiTron2>/resources"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/src/UI" "$<TARGET_FILE_DIR:MultiTron2>/assets"
)
