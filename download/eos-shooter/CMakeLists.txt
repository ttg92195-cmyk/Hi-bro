# ============================================================================
# EOS Shooter - CMakeLists.txt
# Cross-platform build configuration for Desktop and Android (NDK).
# Supports both Real EOS SDK and Dev Mode (no SDK).
# ============================================================================

cmake_minimum_required(VERSION 3.22)
project(EOS_Shooter VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Platform Detection ===
if(ANDROID)
    set(PLATFORM_ANDROID TRUE)
    add_definitions(-DPLATFORM_ANDROID)
    message(STATUS "Building for Android")
elseif(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
    message(STATUS "Building for Windows")
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
    message(STATUS "Building for macOS")
else()
    add_definitions(-DPLATFORM_LINUX)
    message(STATUS "Building for Linux")
endif()

# === Source Files ===
set(GAME_SOURCES
    src/main.cpp
    src/game/Game.cpp
    src/game/Player.cpp
    src/game/Bullet.cpp
    src/game/Weapon.cpp
    src/game/Enemy.cpp
    src/game/Map.cpp
    src/game/ParticleSystem.cpp
    src/game/CameraController.cpp
    src/multiplayer/EOSManager.cpp
    src/multiplayer/NetworkManager.cpp
    src/utils/InputManager.cpp
    src/utils/AudioManager.cpp
)

set(GAME_HEADERS
    src/game/Game.h
    src/game/Player.h
    src/game/Bullet.h
    src/game/Weapon.h
    src/game/Enemy.h
    src/game/Map.h
    src/game/ParticleSystem.h
    src/game/CameraController.h
    src/multiplayer/EOSManager.h
    src/multiplayer/NetworkManager.h
    src/utils/Math.h
    src/utils/InputManager.h
    src/utils/AudioManager.h
)

# === Find Raylib ===
if(NOT ANDROID)
    # Desktop: Use find_package or pkg-config
    find_package(raylib QUIET)

    if(NOT raylib_FOUND)
        # Try pkg-config
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(RAYLIB QUIET raylib)
        endif()

        if(NOT RAYLIB_FOUND)
            # Fallback: Build raylib from source
            include(FetchContent)
            FetchContent_Declare(
                raylib
                GIT_REPOSITORY https://github.com/raysan5/raylib.git
                GIT_TAG master
            )
            FetchContent_MakeAvailable(raylib)
        endif()
    endif()
else()
    # Android: Raylib is built as a static library with NDK
    set(RAYLIB_PATH "${CMAKE_SOURCE_DIR}/third_party/raylib" CACHE PATH "Raylib source path")
    if(EXISTS "${RAYLIB_PATH}/CMakeLists.txt")
        add_subdirectory(${RAYLIB_PATH} raylib)
    else()
        message(WARNING "Raylib not found at ${RAYLIB_PATH}. Please clone raylib into third_party/")
    endif()
endif()

# ============================================================================
# === EOS SDK Integration ===
# ============================================================================
set(EOS_SDK_PATH "${CMAKE_SOURCE_DIR}/third_party/eos-sdk" CACHE PATH "EOS SDK path")

# Check if real EOS SDK headers exist
if(EXISTS "${EOS_SDK_PATH}/Include/eos_sdk.h")
    message(STATUS "✅ EOS SDK found at ${EOS_SDK_PATH} - Real Multiplayer enabled!")
    add_definitions(-DEOS_SDK_ENABLED)

    if(ANDROID)
        # === Android: Use .so shared library ===
        # The Android SDK provides .so files inside the AAR
        # Check for .so files
        if(EXISTS "${EOS_SDK_PATH}/Lib/Android/arm64-v8a/libEOSSDK.so")
            add_library(eos_sdk SHARED IMPORTED)
            set_target_properties(eos_sdk PROPERTIES
                IMPORTED_LOCATION "${EOS_SDK_PATH}/Lib/Android/arm64-v8a/libEOSSDK.so"
                INTERFACE_INCLUDE_DIRECTORIES "${EOS_SDK_PATH}/Include"
            )
            message(STATUS "  Android arm64-v8a: Using libEOSSDK.so")
        else()
            message(WARNING "  Android libEOSSDK.so not found!")
        endif()

    elseif(WIN32)
        # === Windows: Use .lib ===
        if(EXISTS "${EOS_SDK_PATH}/Lib/Win64/Release/EOSSDK.lib")
            add_library(eos_sdk STATIC IMPORTED)
            set_target_properties(eos_sdk PROPERTIES
                IMPORTED_LOCATION "${EOS_SDK_PATH}/Lib/Win64/Release/EOSSDK.lib"
                INTERFACE_INCLUDE_DIRECTORIES "${EOS_SDK_PATH}/Include"
            )
            message(STATUS "  Windows: Using EOSSDK.lib")
        else()
            message(WARNING "  Windows EOSSDK.lib not found - download C SDK")
        endif()

    elseif(APPLE)
        # === macOS: Use .a ===
        if(EXISTS "${EOS_SDK_PATH}/Lib/Mac/Release/libEOSSDK.a")
            add_library(eos_sdk STATIC IMPORTED)
            set_target_properties(eos_sdk PROPERTIES
                IMPORTED_LOCATION "${EOS_SDK_PATH}/Lib/Mac/Release/libEOSSDK.a"
                INTERFACE_INCLUDE_DIRECTORIES "${EOS_SDK_PATH}/Include"
            )
        else()
            message(WARNING "  macOS libEOSSDK.a not found - download C SDK")
        endif()

    else()
        # === Linux: Use .a ===
        if(EXISTS "${EOS_SDK_PATH}/Lib/Linux/Release/libEOSSDK.a")
            add_library(eos_sdk STATIC IMPORTED)
            set_target_properties(eos_sdk PROPERTIES
                IMPORTED_LOCATION "${EOS_SDK_PATH}/Lib/Linux/Release/libEOSSDK.a"
                INTERFACE_INCLUDE_DIRECTORIES "${EOS_SDK_PATH}/Include"
            )
        else()
            message(WARNING "  Linux libEOSSDK.a not found - download C SDK")
        endif()
    endif()

    if(TARGET eos_sdk)
        set(EOS_LINK_LIBS eos_sdk)
    else()
        message(WARNING "EOS SDK header found but library not found for this platform - building dev mode")
        set(EOS_LINK_LIBS)
    endif()
else()
    message(STATUS "❌ EOS SDK not found - building in Dev Mode (no multiplayer)")
    message(STATUS "   To enable multiplayer: Download EOS SDK to third_party/eos-sdk/")
    set(EOS_LINK_LIBS)
endif()

# === Executable ===
if(ANDROID)
    # Android shared library
    add_library(eos-shooter SHARED ${GAME_SOURCES} ${GAME_HEADERS})

    target_include_directories(eos-shooter PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party/raylib/src
        ${EOS_SDK_PATH}/Include
    )

    target_link_libraries(eos-shooter
        raylib
        ${EOS_LINK_LIBS}
        android
        log
        m
    )

    # Copy EOS .so to libs directory for packaging
    if(EXISTS "${EOS_SDK_PATH}/Lib/Android/arm64-v8a/libEOSSDK.so")
        add_custom_command(TARGET eos-shooter POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${EOS_SDK_PATH}/Lib/Android/arm64-v8a/libEOSSDK.so"
                $<TARGET_FILE_DIR:eos-shooter>/libEOSSDK.so
        )
    endif()
else()
    # Desktop executable
    add_executable(eos-shooter ${GAME_SOURCES} ${GAME_HEADERS})

    target_include_directories(eos-shooter PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${EOS_SDK_PATH}/Include
    )

    target_link_libraries(eos-shooter
        raylib
        ${EOS_LINK_LIBS}
        m
        pthread
    )

    if(WIN32)
        target_link_libraries(eos-shooter winmm gdi32 opengl32)
    endif()

    if(APPLE)
        find_library(FOUNDATION Foundation)
        find_library(COREVIDEO CoreVideo)
        find_library(IOKIT IOKit)
        target_link_libraries(eos-shooter ${FOUNDATION} ${COREVIDEO} ${IOKIT})
    endif()

    # Copy EOS shared library next to executable (Linux)
    if(EXISTS "${EOS_SDK_PATH}/Lib/Linux/Release/libEOSSDK.so")
        add_custom_command(TARGET eos-shooter POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${EOS_SDK_PATH}/Lib/Linux/Release/libEOSSDK.so"
                $<TARGET_FILE_DIR:eos-shooter>/libEOSSDK.so
        )
    endif()
endif()

# === Compiler Options ===
target_compile_options(eos-shooter PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    $<$<CONFIG:Debug>:-g3 -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# === Copy assets to build directory ===
if(NOT ANDROID)
    add_custom_command(TARGET eos-shooter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:eos-shooter>/assets
    )
endif()

# === Install ===
install(TARGETS eos-shooter
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

install(DIRECTORY assets/ DESTINATION share/eos-shooter/assets)
