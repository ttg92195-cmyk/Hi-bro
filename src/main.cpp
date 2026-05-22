// ============================================================================
// EOS Shooter - main.cpp
// Entry point for the game application.
// On Android, uses android_main() as required by NativeActivity + Raylib.
// On Desktop, uses standard int main().
// ============================================================================

#include "game/Game.h"
#include "utils/AudioManager.h"

#if defined(PLATFORM_ANDROID)
#include <android_native_app_glue.h>
#endif

#include <iostream>

using namespace EOSShooter;

#if defined(PLATFORM_ANDROID)
// ============================================================================
// Android Entry Point
// On Android, the NativeActivity framework calls android_main() instead of
// main(). Raylib requires InitWindow() to be called BEFORE any other raylib
// functions because it sets up the android_app context, EGL surface, and
// OpenGL ES context. We call InitWindow() here, then pass the real screen
// dimensions to Game::Initialize().
// ============================================================================
void android_main(struct android_app* app) {
    // CRITICAL: InitWindow MUST be called first on Android.
    // This sets up the android_app context, creates the EGL surface,
    // and initializes the OpenGL ES context.
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "EOS Shooter");

    TraceLog(LOG_INFO, "EOS SHOOTER: Android launch - window initialized");

    // Get the actual screen dimensions from the initialized window
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    TraceLog(LOG_INFO, "EOS SHOOTER: Screen size %dx%d", screenWidth, screenHeight);

    // Create and initialize the game (InitWindow is NOT called inside
    // Game::Initialize on Android — it was already called above)
    Game game;
    if (!game.Initialize(screenWidth, screenHeight, "EOS Shooter")) {
        TraceLog(LOG_ERROR, "EOS SHOOTER: Failed to initialize game on Android!");
        CloseWindow();
        return;
    }

    // Main game loop
    game.Run();

    // Cleanup
    CloseWindow();
}
#else
// ============================================================================
// Desktop Entry Point (Linux, Windows, macOS)
// ============================================================================
int main() {
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

    return 0;
}
#endif
