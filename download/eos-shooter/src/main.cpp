// ============================================================================
// EOS Shooter - main.cpp
// Entry point for the game application.
// ============================================================================

#include "game/Game.h"
#include "utils/AudioManager.h"
#include <iostream>

#if defined(PLATFORM_ANDROID)
#include <android_native_app_glue.h>
#endif

using namespace EOSShooter;

// ============================================================================
// Desktop Entry Point
// ============================================================================
#if !defined(PLATFORM_ANDROID)

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    EOS SHOOTER - Multiplayer FPS" << std::endl;
    std::cout << "    Powered by Raylib + Epic Online" << std::endl;
    std::cout << "========================================" << std::endl;

    Game game;

    // Initialize with 1280x720 window
    if (!game.Initialize(1280, 720, "EOS Shooter - Multiplayer FPS")) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return -1;
    }

    // Main game loop
    game.Run();

    return 0;
}

// ============================================================================
// Android Entry Point
// ============================================================================
#else

void android_main(struct android_app* app) {
    // Initialize Raylib for Android
    InitWindow(0, 0, "EOS Shooter");

    Game game;
    if (!game.Initialize(GetScreenWidth(), GetScreenHeight(), "EOS Shooter")) {
        TraceLog(LOG_ERROR, "Failed to initialize game on Android!");
        return;
    }

    game.Run();
    CloseWindow();
}

#endif
