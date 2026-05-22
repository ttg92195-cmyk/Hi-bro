// ============================================================================
// EOS Shooter - main.cpp
// Entry point for the game application.
// On Android, raylib's native_app_glue provides android_main() which calls
// main(). We just need to ensure InitWindow() is called BEFORE any other
// raylib functions on Android.
// ============================================================================

#include "game/Game.h"
#include "utils/AudioManager.h"
#include <iostream>

using namespace EOSShooter;

int main() {
#if defined(PLATFORM_ANDROID)
    // CRITICAL: On Android, InitWindow() MUST be called before any other
    // raylib functions. It sets up the android_app context, creates the
    // EGL surface, and initializes the OpenGL ES context.
    // Game::Initialize() will detect Android and skip its own InitWindow() call.
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "EOS Shooter");

    TraceLog(LOG_INFO, "EOS SHOOTER: Android launch - window initialized");

    // Get the actual screen dimensions from the initialized window
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    TraceLog(LOG_INFO, "EOS SHOOTER: Screen size %dx%d", screenWidth, screenHeight);

    // Initialize game with real screen dimensions
    // Game::Initialize() will skip InitWindow() on Android since we already called it
    Game game;
    if (!game.Initialize(screenWidth, screenHeight, "EOS Shooter")) {
        TraceLog(LOG_ERROR, "EOS SHOOTER: Failed to initialize game on Android!");
        CloseWindow();
        return -1;
    }

    // Main game loop
    game.Run();

    // Cleanup window (Game::Shutdown() skips CloseWindow() on Android)
    CloseWindow();
#else
    // Desktop: standard initialization
    std::cout << "========================================" << std::endl;
    std::cout << "    EOS SHOOTER - Multiplayer FPS" << std::endl;
    std::cout << "    Powered by Raylib + Epic Online" << std::endl;
    std::cout << "========================================" << std::endl;

    Game game;

    if (!game.Initialize(1280, 720, "EOS Shooter - Multiplayer FPS")) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return -1;
    }

    // Main game loop
    game.Run();
#endif

    return 0;
}
