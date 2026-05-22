// ============================================================================
// EOS Shooter - main.cpp
// Entry point for the game application.
// ============================================================================

#include "game/Game.h"
#include "utils/AudioManager.h"
#include <iostream>

using namespace EOSShooter;

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    EOS SHOOTER - Multiplayer FPS" << std::endl;
    std::cout << "    Powered by Raylib + Epic Online" << std::endl;
    std::cout << "========================================" << std::endl;

    Game game;

    // Initialize with 1280x720 window (on Android, use GetScreenWidth/Height)
    int width = 1280;
    int height = 720;

#if defined(PLATFORM_ANDROID)
    // On Android, window size will be set by the platform
    width = 0;
    height = 0;
#endif

    if (!game.Initialize(width, height, "EOS Shooter - Multiplayer FPS")) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return -1;
    }

    // Main game loop
    game.Run();

    return 0;
}
