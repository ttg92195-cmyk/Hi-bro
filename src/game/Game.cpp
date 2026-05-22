// ============================================================================
// EOS Shooter - Game.cpp
// Full implementation of the main Game class.
// ============================================================================

#include "Game.h"
#include "../utils/Math.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <chrono>

namespace EOSShooter {

// ============================================================================
// Construction / Destruction
// ============================================================================

Game::Game()
    : map_(std::make_unique<Map>())
    , particleSystem_(std::make_unique<ParticleSystem>(5000))
    , cameraController_(std::make_unique<CameraController>())
    , eosManager_(std::make_unique<EOSManager>())
    , networkManager_(std::make_unique<NetworkManager>())
{
}

Game::~Game() {
    Shutdown();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool Game::Initialize(int screenWidth, int screenHeight, const std::string& title) {
    if (isInitialized_) return true;

    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;

#if defined(PLATFORM_ANDROID)
    // On Android, InitWindow() is called from android_main() BEFORE this function.
    // Calling it again here would crash because the EGL context is already created.
    // Just get the actual screen dimensions from the already-initialized window.
    // FIX #4: Validate screen dimensions - GetScreenWidth/Height can return 0
    // on some Android devices if the surface isn't fully ready.
    screenWidth_ = GetScreenWidth();
    screenHeight_ = GetScreenHeight();
    if (screenWidth_ <= 0) screenWidth_ = 1280;
    if (screenHeight_ <= 0) screenHeight_ = 720;
    TraceLog(LOG_INFO, "Game: Android detected - using existing window (%dx%d)", screenWidth_, screenHeight_);
#else
    // On Desktop, InitWindow() is called here for the first time
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth_, screenHeight_, title.c_str());
    SetWindowMinSize(800, 600);
#endif

    SetTargetFPS(60);

    // FIX #3: InitAudioDevice() can crash on some Android devices where
    // OpenSLES initialization fails. Wrap in a safety check - if the audio
    // device can't be initialized, the game should still run without sound.
#if defined(PLATFORM_ANDROID)
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        if (!IsAudioDeviceReady()) {
            TraceLog(LOG_WARNING, "Game: Audio device initialization failed - continuing without audio");
        }
    }
#else
    InitAudioDevice();
#endif

    // Initialize EOS
    if (!eosManager_->Initialize()) {
        TraceLog(LOG_WARNING, "EOS: Failed to initialize - multiplayer will be unavailable");
    }

    // Initialize network manager with EOS reference
    networkManager_->Initialize(eosManager_.get());

    // Set up network callbacks
    networkManager_->SetOnPlayerJoinedCallback(
        [this](uint64_t id, const std::string& name) { OnPlayerJoined(id, name); });
    networkManager_->SetOnPlayerLeftCallback(
        [this](uint64_t id) { OnPlayerLeft(id); });
    networkManager_->SetOnPlayerUpdateCallback(
        [this](uint64_t id, const Vector3& pos, const Vector3& rot) { OnPlayerUpdate(id, pos, rot); });
    networkManager_->SetOnPlayerShotCallback(
        [this](uint64_t id, const Vector3& origin, const Vector3& dir) { OnPlayerShot(id, origin, dir); });
    networkManager_->SetOnPlayerHitCallback(
        [this](uint64_t id, float dmg, uint64_t attacker) { OnPlayerHit(id, dmg, attacker); });

    // Initialize camera controller
    cameraController_->Initialize(screenWidth_, screenHeight_);

    // Set initial state
    SetState(GameState::MENU);

    isInitialized_ = true;
    isRunning_ = true;

    TraceLog(LOG_INFO, "Game: Initialized successfully (%dx%d)", screenWidth_, screenHeight_);
    return true;
}

void Game::Run() {
    if (!isInitialized_) {
        TraceLog(LOG_ERROR, "Game: Cannot run - not initialized");
        return;
    }

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (isRunning_ && !WindowShouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        lastTime = currentTime;
        deltaTime_ = std::min(elapsed.count(), 0.05f); // Cap at 50ms to prevent spiral

        // FPS counter
        frameCount_++;
        fpsTimer_ += deltaTime_;
        if (fpsTimer_ >= 1.0f) {
            fps_ = frameCount_;
            frameCount_ = 0;
            fpsTimer_ -= 1.0f;
        }

        // Fixed timestep for physics
        fixedAccumulator_ += deltaTime_;
        while (fixedAccumulator_ >= fixedDeltaTime_) {
            // Physics update would go here at 60fps
            fixedAccumulator_ -= fixedDeltaTime_;
        }

        // Update EOS platform frame
        eosManager_->Tick();

        // Main update and render
        Update(deltaTime_);
        Render();
    }

    Shutdown();
}

void Game::Shutdown() {
    if (!isInitialized_) return;

    TraceLog(LOG_INFO, "Game: Shutting down...");

    // Clean up in reverse order of initialization
    networkManager_->Shutdown();
    eosManager_->Shutdown();

    players_.clear();
    enemies_.clear();
    bullets_.clear();
    playerMap_.clear();
    leaderboard_.clear();
    scoreMap_.clear();

    // FIX #5: Only close audio device if it was successfully initialized.
    // On Android, CloseAudioDevice() can crash if OpenSLES was never initialized.
    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }

#if !defined(PLATFORM_ANDROID)
    // On Android, CloseWindow() is called from android_main() after Run() returns.
    // Calling it here would double-close and crash.
    CloseWindow();
#endif

    isInitialized_ = false;
    isRunning_ = false;
}

// ============================================================================
// State Management
// ============================================================================

void Game::SetState(GameState newState) {
    if (currentState_ == newState) return;

    previousState_ = currentState_;
    currentState_ = newState;
    stateTimer_ = 0.0f;

    TraceLog(LOG_INFO, "Game: State changed from %d to %d", (int)previousState_, (int)currentState_);

    // Handle state entry logic
    switch (currentState_) {
    case GameState::MENU:
#if !defined(PLATFORM_ANDROID)
        EnableCursor();
#endif
        break;

    case GameState::PLAYING:
#if !defined(PLATFORM_ANDROID)
        DisableCursor(); // First-person camera needs cursor locked
#endif
        break;

    case GameState::PAUSED:
#if !defined(PLATFORM_ANDROID)
        EnableCursor();
#endif
        break;

    case GameState::GAME_OVER:
#if !defined(PLATFORM_ANDROID)
        EnableCursor();
#endif
        break;

    default:
        break;
    }
}

// ============================================================================
// Game Session
// ============================================================================

bool Game::StartGame(const GameConfig& config) {
    config_ = config;
    matchTimer_ = 0.0f;
    matchEnded_ = false;
    gameTime_ = 0.0f;

    // Load the map
    if (!map_->Load(config_.mapName)) {
        TraceLog(LOG_ERROR, "Game: Failed to load map '%s'", config_.mapName.c_str());
        return false;
    }

    // Create local player
    Vector3 spawnPos = GetRandomSpawnPoint();
    localPlayer_ = new Player(0, "LocalPlayer", spawnPos);
    localPlayer_->SetIsLocal(true);
    players_.emplace_back(localPlayer_);
    playerMap_[0] = localPlayer_;

    // Add to leaderboard
    ScoreEntry entry;
    entry.playerId = 0;
    entry.playerName = "LocalPlayer";
    leaderboard_.push_back(entry);
    scoreMap_[0] = &leaderboard_.back();

    // If coop survival, prepare wave system
    if (config_.mode == GameMode::COOP_SURVIVAL) {
        waveInfo_ = WaveInfo{};
        StartNextWave();
    }

    // Initialize camera at player position
    cameraController_->SetPosition(spawnPos);
    cameraController_->SetEnabled(true);

    SetState(GameState::PLAYING);

    TraceLog(LOG_INFO, "Game: Started game mode %d on map '%s'", (int)config_.mode, config_.mapName.c_str());
    return true;
}

void Game::EndGame() {
    matchEnded_ = true;
    SortLeaderboard();
    SetState(GameState::GAME_OVER);

    TraceLog(LOG_INFO, "Game: Match ended. Winner: %s",
        leaderboard_.empty() ? "None" : leaderboard_[0].playerName.c_str());
}

void Game::PauseGame() {
    if (currentState_ == GameState::PLAYING) {
        isPaused_ = true;
        SetState(GameState::PAUSED);
    }
}

void Game::ResumeGame() {
    if (currentState_ == GameState::PAUSED) {
        isPaused_ = false;
        SetState(GameState::PLAYING);
    }
}

void Game::RestartGame() {
    // Clean up current session
    players_.clear();
    enemies_.clear();
    bullets_.clear();
    playerMap_.clear();
    leaderboard_.clear();
    scoreMap_.clear();
    localPlayer_ = nullptr;

    // Restart with same config
    StartGame(config_);
}

// ============================================================================
// Update
// ============================================================================

void Game::Update(float deltaTime) {
    stateTimer_ += deltaTime;

    switch (currentState_) {
    case GameState::MENU:       UpdateMenu(deltaTime); break;
    case GameState::LOBBY:      UpdateLobby(deltaTime); break;
    case GameState::LOADING:    UpdateLoading(deltaTime); break;
    case GameState::PLAYING:    UpdatePlaying(deltaTime); break;
    case GameState::PAUSED:     /* Handled by UI */ break;
    case GameState::GAME_OVER:  UpdateGameOver(deltaTime); break;
    case GameState::SPECTATING: UpdateSpectating(deltaTime); break;
    }
}

void Game::UpdatePlaying(float deltaTime) {
    gameTime_ += deltaTime;
    matchTimer_ += deltaTime;

    // Check match time limit
    if (config_.timeLimit > 0 && matchTimer_ >= config_.timeLimit && !matchEnded_) {
        EndGame();
        return;
    }

    // Update local player input and movement
    if (localPlayer_) {
#if defined(PLATFORM_ANDROID)
        HandleTouchInput(deltaTime);
        localPlayer_->Update(deltaTime);
#else
        localPlayer_->Update(deltaTime);
#endif
        cameraController_->FollowPlayer(localPlayer_, deltaTime);

        // Handle player shooting
        if (localPlayer_->IsShooting()) {
            Weapon* weapon = localPlayer_->GetCurrentWeapon();
            if (weapon && weapon->CanFire()) {
                Vector3 origin = cameraController_->GetCameraPosition();
                Vector3 direction = cameraController_->GetCameraForward();

                // Apply weapon spread
                direction = Math::ApplySpread(direction, weapon->GetCurrentSpread());

                // Create bullet
                Bullet bullet;
                bullet.position = origin;
                bullet.direction = direction;
                bullet.velocity = { direction.x * weapon->GetBulletSpeed(), direction.y * weapon->GetBulletSpeed(), direction.z * weapon->GetBulletSpeed() };
                bullet.damage = weapon->GetDamage();
                bullet.range = weapon->GetRange();
                bullet.ownerId = localPlayer_->GetPlayerId();
                bullet.type = weapon->GetBulletType();
                bullet.lifetime = 0.0f;
                bullet.maxLifetime = weapon->GetRange() / weapon->GetBulletSpeed();

                bullets_.push_back(bullet);
                weapon->Fire();

                // Muzzle flash particle
                particleSystem_->Emit(ParticleEmitterType::MUZZLE_FLASH, origin, direction, 5);

                // Notify network
                networkManager_->SendPlayerShot(origin, direction);
            }
        }
    }

    // Update remote players
    for (auto& player : players_) {
        if (!player->IsLocal()) {
            player->Update(deltaTime);
        }
    }

    // Update enemies
    for (auto& enemy : enemies_) {
        enemy->Update(deltaTime, localPlayer_);
    }

    // Update bullets
    for (auto& bullet : bullets_) {
        bullet.Update(deltaTime);
    }

    // Remove dead bullets
    bullets_.erase(
        std::remove_if(bullets_.begin(), bullets_.end(),
            [](const Bullet& b) { return b.IsExpired(); }),
        bullets_.end());

    // Check collisions
    CheckBulletCollisions();
    CheckPickupCollisions();

    // Update particles
    particleSystem_->Update(deltaTime);

    // Update wave system for coop
    if (config_.mode == GameMode::COOP_SURVIVAL) {
        UpdateWaveSystem(deltaTime);
    }

    // Remove dead enemies
    enemies_.erase(
        std::remove_if(enemies_.begin(), enemies_.end(),
            [](const std::unique_ptr<Enemy>& e) { return e->IsDead() && e->GetDeathTimer() > 3.0f; }),
        enemies_.end());

    // Process network messages
    networkManager_->ProcessMessages();

    // Respawn dead players
    for (auto& player : players_) {
        if (player->IsDead() && player->GetDeathTimer() > 5.0f) {
            RespawnPlayer(player.get());
        }
    }

    // Update leaderboard entries
    for (auto& player : players_) {
        auto it = scoreMap_.find(player->GetPlayerId());
        if (it != scoreMap_.end()) {
            it->second->kills = player->GetKills();
            it->second->deaths = player->GetDeaths();
            it->second->score = player->GetKills() * 100 - player->GetDeaths() * 50;
        }
    }
}

void Game::UpdateMenu(float deltaTime) {
    // Menu logic handled by MainMenu UI class
}

void Game::UpdateLobby(float deltaTime) {
    eosManager_->Tick();
    networkManager_->ProcessMessages();
}

void Game::UpdateLoading(float deltaTime) {
    // Simulate loading progress
    if (stateTimer_ > 2.0f) {
        SetState(GameState::PLAYING);
    }
}

void Game::UpdateGameOver(float deltaTime) {
    // Show final scores, wait for input to return to menu
}

void Game::UpdateSpectating(float deltaTime) {
    if (localPlayer_) {
        // Camera follows other players
        cameraController_->Update(deltaTime);
    }
}

// ============================================================================
// Render
// ============================================================================

void Game::Render() {
    BeginDrawing();
    ClearBackground(BLACK);

    switch (currentState_) {
    case GameState::MENU:       RenderMenu(); break;
    case GameState::LOBBY:      RenderLobby(); break;
    case GameState::LOADING:    RenderLoading(); break;
    case GameState::PLAYING:    RenderPlaying(); break;
    case GameState::PAUSED:     RenderPlaying(); /* Render pause overlay */ break;
    case GameState::GAME_OVER:  RenderGameOver(); break;
    case GameState::SPECTATING: RenderSpectating(); break;
    }

    EndDrawing();
}

void Game::RenderPlaying() {
    // Begin 3D camera mode
    Camera3D camera = cameraController_->GetRaylibCamera();
    BeginMode3D(camera);

    // Render map
    map_->Render();

    // Render enemies
    for (const auto& enemy : enemies_) {
        if (!enemy->IsDead()) {
            enemy->Render();
        }
    }

    // Render other players
    for (const auto& player : players_) {
        if (!player->IsLocal() && !player->IsDead()) {
            player->Render();
        }
    }

    // Render bullets
    for (const auto& bullet : bullets_) {
        bullet.Render();
    }

    // Render particles
    particleSystem_->Render();

    // Render pickups
    map_->RenderPickups();

    EndMode3D();

    // 2D HUD overlay
    RenderDebugInfo();

#if defined(PLATFORM_ANDROID)
    // Touch controls overlay
    RenderTouchControls();
#endif
}

void Game::RenderMenu() {
    // Background
    DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.85f));

    // Title
    DrawText("EOS SHOOTER", screenWidth_ / 2 - MeasureText("EOS SHOOTER", 60) / 2, 80, 60, RED);
    DrawText("Multiplayer FPS", screenWidth_ / 2 - MeasureText("Multiplayer FPS", 24) / 2, 150, 24, GRAY);

#if defined(PLATFORM_ANDROID)
    // Touch-friendly buttons for Android
    int btnW = 400;
    int btnH = 60;
    int btnX = screenWidth_ / 2 - btnW / 2;

    // "Start Game" button
    Rectangle startBtn = { (float)btnX, 250.0f, (float)btnW, (float)btnH };
    bool startHover = CheckCollisionPointRec(GetTouchPosition(0), startBtn) || CheckCollisionPointRec(GetMousePosition(), startBtn);
    DrawRectangleRec(startBtn, startHover ? DARKGREEN : GREEN);
    DrawRectangleLinesEx(startBtn, 2, WHITE);
    DrawText("TAP TO START GAME", btnX + MeasureText("TAP TO START GAME", 24) / 2, 262, 24, WHITE);

    // "Multiplayer" button
    Rectangle lobbyBtn = { (float)btnX, 330.0f, (float)btnW, (float)btnH };
    bool lobbyHover = CheckCollisionPointRec(GetTouchPosition(0), lobbyBtn) || CheckCollisionPointRec(GetMousePosition(), lobbyBtn);
    DrawRectangleRec(lobbyBtn, lobbyHover ? DARKPURPLE : PURPLE);
    DrawRectangleLinesEx(lobbyBtn, 2, WHITE);
    DrawText("MULTIPLAYER LOBBY", btnX + MeasureText("MULTIPLAYER LOBBY", 24) / 2, 342, 24, WHITE);

    // Check touch/click for start
    if ((IsKeyPressed(KEY_ENTER)) ||
        (GetGestureDetected() == GESTURE_TAP && startHover) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && startHover)) {
        GameConfig config;
        config.mode = GameMode::COOP_SURVIVAL;
        config.mapName = "urban_warehouse";
        StartGame(config);
    }

    // Check touch/click for lobby
    if ((IsKeyPressed(KEY_L)) ||
        (GetGestureDetected() == GESTURE_TAP && lobbyHover) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && lobbyHover)) {
        SetState(GameState::LOBBY);
    }
#else
    DrawText("Press ENTER to Start", screenWidth_ / 2 - 140, 300, 30, WHITE);
    DrawText("Press L for Multiplayer Lobby", screenWidth_ / 2 - 200, 350, 30, WHITE);

    if (IsKeyPressed(KEY_ENTER)) {
        GameConfig config;
        config.mode = GameMode::COOP_SURVIVAL;
        config.mapName = "urban_warehouse";
        StartGame(config);
    }
    if (IsKeyPressed(KEY_L)) {
        SetState(GameState::LOBBY);
    }
#endif
}

void Game::RenderLobby() {
    DrawText("MULTIPLAYER LOBBY", screenWidth_ / 2 - 200, 50, 40, YELLOW);
    DrawText("Press C to Create Session", screenWidth_ / 2 - 180, 200, 25, WHITE);
    DrawText("Press J to Join Session", screenWidth_ / 2 - 170, 250, 25, WHITE);
    DrawText("Press ESC to go Back", screenWidth_ / 2 - 140, 300, 25, GRAY);

    if (IsKeyPressed(KEY_C)) {
        if (eosManager_->CreateSession("EOS Shooter Room", config_.maxPlayers)) {
            TraceLog(LOG_INFO, "Lobby: Session created successfully");
        }
    }
    if (IsKeyPressed(KEY_J)) {
        eosManager_->FindSessions();
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        SetState(GameState::MENU);
    }

    // Show found sessions
    auto sessions = eosManager_->GetFoundSessions();
    int y = 400;
    for (const auto& session : sessions) {
        DrawText(TextFormat("%s (%d/%d)", session.name.c_str(), session.currentPlayers, session.maxPlayers),
                 200, y, 20, GREEN);
        y += 30;
    }
}

void Game::RenderLoading() {
    float progress = std::min(stateTimer_ / 2.0f, 1.0f);
    DrawText("LOADING...", screenWidth_ / 2 - 80, screenHeight_ / 2 - 20, 30, WHITE);
    DrawRectangle(screenWidth_ / 2 - 200, screenHeight_ / 2 + 30, (int)(400 * progress), 20, BLUE);
    DrawRectangleLines(screenWidth_ / 2 - 200, screenHeight_ / 2 + 30, 400, 20, WHITE);
}

void Game::RenderGameOver() {
    DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, 0.7f));
    DrawText("GAME OVER", screenWidth_ / 2 - 160, 80, 50, RED);

    // Display leaderboard
    SortLeaderboard();
    int y = 200;
    DrawText("RANK  PLAYER          KILLS  DEATHS  SCORE", 100, y, 20, YELLOW);
    y += 35;
    for (size_t i = 0; i < leaderboard_.size() && i < 10; i++) {
        const auto& entry = leaderboard_[i];
        Color color = (i == 0) ? GOLD : WHITE;
        DrawText(TextFormat("#%-3d  %-15s %5d  %5d  %6d",
            (int)(i + 1), entry.playerName.c_str(), entry.kills, entry.deaths, entry.score),
            100, y, 18, color);
        y += 28;
    }

    DrawText("Press ENTER to Play Again | ESC for Menu", screenWidth_ / 2 - 260, screenHeight_ - 80, 22, GRAY);

#if defined(PLATFORM_ANDROID)
    // Touch buttons for Game Over
    Rectangle retryBtn = { (float)(screenWidth_ / 2 - 200), (float)(screenHeight_ - 120), 180.0f, 50.0f };
    Rectangle menuBtn = { (float)(screenWidth_ / 2 + 20), (float)(screenHeight_ - 120), 180.0f, 50.0f };

    DrawRectangleRec(retryBtn, DARKGREEN);
    DrawText("PLAY AGAIN", (int)retryBtn.x + 20, (int)retryBtn.y + 14, 20, WHITE);
    DrawRectangleRec(menuBtn, DARKGRAY);
    DrawText("MENU", (int)menuBtn.x + 55, (int)menuBtn.y + 14, 20, WHITE);

    if (IsKeyPressed(KEY_ENTER) ||
        (GetGestureDetected() == GESTURE_TAP && CheckCollisionPointRec(GetTouchPosition(0), retryBtn))) {
        RestartGame();
    }
    if (IsKeyPressed(KEY_ESCAPE) ||
        (GetGestureDetected() == GESTURE_TAP && CheckCollisionPointRec(GetTouchPosition(0), menuBtn))) {
        SetState(GameState::MENU);
    }
#else
    if (IsKeyPressed(KEY_ENTER)) RestartGame();
    if (IsKeyPressed(KEY_ESCAPE)) SetState(GameState::MENU);
#endif
}

void Game::RenderSpectating() {
    Camera3D camera = cameraController_->GetRaylibCamera();
    BeginMode3D(camera);
    map_->Render();
    for (const auto& player : players_) {
        if (!player->IsDead()) player->Render();
    }
    for (const auto& enemy : enemies_) {
        if (!enemy->IsDead()) enemy->Render();
    }
    EndMode3D();

    DrawText("SPECTATING", screenWidth_ / 2 - 80, 50, 30, YELLOW);
}

void Game::RenderDebugInfo() {
    DrawFPS(10, 10);
    DrawText(TextFormat("Game Time: %.1f", gameTime_), 10, 30, 16, WHITE);
    DrawText(TextFormat("Players: %d", (int)players_.size()), 10, 50, 16, WHITE);
    DrawText(TextFormat("Enemies: %d", (int)enemies_.size()), 10, 70, 16, WHITE);
    DrawText(TextFormat("Bullets: %d", (int)bullets_.size()), 10, 90, 16, WHITE);

    if (config_.mode == GameMode::COOP_SURVIVAL) {
        DrawText(TextFormat("Wave: %d | Enemies: %d/%d",
            waveInfo_.waveNumber, waveInfo_.enemiesAlive, waveInfo_.enemiesTotal),
            10, 110, 16, ORANGE);
    }

    if (localPlayer_) {
        Vector3 pos = localPlayer_->GetPosition();
        DrawText(TextFormat("Pos: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z), 10, 130, 16, GREEN);
        DrawText(TextFormat("HP: %.0f | Armor: %.0f", localPlayer_->GetHealth(), localPlayer_->GetArmor()), 10, 150, 16, GREEN);
    }

    // EOS status
    DrawText(TextFormat("EOS: %s", eosManager_->IsLoggedIn() ? "Connected" : "Offline"),
             10, screenHeight_ - 30, 16, eosManager_->IsLoggedIn() ? GREEN : RED);
}

// ============================================================================
// Collision Detection
// ============================================================================

void Game::CheckBulletCollisions() {
    for (auto& bullet : bullets_) {
        if (bullet.HasHit()) continue;

        // Check against map geometry
        Ray ray = { bullet.position, bullet.direction };
        RayCollision mapHit = map_->Raycast(ray, bullet.range - bullet.distance);

        if (mapHit.hit) {
            bullet.SetHit(true);
            particleSystem_->Emit(ParticleEmitterType::SPARK, mapHit.point, {0,1,0}, 8);
            continue;
        }

        // Check against enemies (for coop/PvE)
        for (auto& enemy : enemies_) {
            if (enemy->IsDead()) continue;
            BoundingBox enemyBox = enemy->GetBoundingBox();
            RayCollision enemyHit = GetRayCollisionBox(ray, enemyBox);
            if (enemyHit.hit) {
                bullet.SetHit(true);
                enemy->TakeDamage(bullet.damage, bullet.ownerId);
                particleSystem_->Emit(ParticleEmitterType::BLOOD, bullet.position, bullet.direction, 10);

                if (enemy->IsDead()) {
                    RegisterKill(bullet.ownerId, enemy->GetUniqueId());
                    particleSystem_->Emit(ParticleEmitterType::EXPLOSION, enemy->GetPosition(), {0,1,0}, 20);
                }
                break;
            }
        }

        // Check against other players (PvP modes)
        if (config_.mode == GameMode::DEATHMATCH || config_.mode == GameMode::TEAM_DEATHMATCH) {
            for (auto& player : players_) {
                if (player->GetPlayerId() == bullet.ownerId || player->IsDead()) continue;
                if (!config_.friendlyFire && player->GetTeamId() == TeamId::NONE) continue;

                BoundingBox playerBox = player->GetBoundingBox();
                RayCollision playerHit = GetRayCollisionBox(ray, playerBox);
                if (playerHit.hit) {
                    bullet.SetHit(true);
                    player->TakeDamage(bullet.damage);
                    particleSystem_->Emit(ParticleEmitterType::BLOOD, bullet.position, bullet.direction, 10);
                    networkManager_->SendPlayerHit(player->GetPlayerId(), bullet.damage, bullet.ownerId);

                    if (player->IsDead()) {
                        RegisterKill(bullet.ownerId, player->GetPlayerId());
                    }
                    break;
                }
            }
        }
    }
}

void Game::CheckPlayerEnemyCollisions() {
    // Melee range detection and similar
}

void Game::CheckPickupCollisions() {
    if (!localPlayer_) return;
    Vector3 playerPos = localPlayer_->GetPosition();

    auto& pickups = map_->GetPickups();
    for (auto& pickup : pickups) {
        if (!pickup.active) continue;
        float dist = Math::Distance3D(playerPos, pickup.position);
        if (dist < 2.0f) {
            switch (pickup.type) {
            case PickupType::HEALTH:
                localPlayer_->Heal(pickup.value);
                break;
            case PickupType::ARMOR:
                localPlayer_->AddArmor(pickup.value);
                break;
            case PickupType::AMMO:
                localPlayer_->RefillAmmo((int)pickup.value);
                break;
            case PickupType::WEAPON:
                localPlayer_->PickupWeapon(pickup.weaponType);
                break;
            }
            pickup.active = false;
            pickup.respawnTimer = 30.0f; // Respawn after 30 seconds
        }
    }
}

// ============================================================================
// Entity Management
// ============================================================================

Player* Game::GetPlayerById(uint64_t playerId) {
    auto it = playerMap_.find(playerId);
    return (it != playerMap_.end()) ? it->second : nullptr;
}

void Game::AddRemotePlayer(uint64_t playerId, const std::string& name) {
    auto player = std::make_unique<Player>(playerId, name, GetRandomSpawnPoint());
    player->SetIsLocal(false);
    playerMap_[playerId] = player.get();
    players_.push_back(std::move(player));

    ScoreEntry entry;
    entry.playerId = playerId;
    entry.playerName = name;
    leaderboard_.push_back(entry);

    TraceLog(LOG_INFO, "Game: Remote player '%s' (ID: %llu) added", name.c_str(), playerId);
}

void Game::RemoveRemotePlayer(uint64_t playerId) {
    auto it = playerMap_.find(playerId);
    if (it != playerMap_.end()) {
        players_.erase(
            std::remove_if(players_.begin(), players_.end(),
                [playerId](const std::unique_ptr<Player>& p) { return p->GetPlayerId() == playerId; }),
            players_.end());
        playerMap_.erase(it);
    }

    leaderboard_.erase(
        std::remove_if(leaderboard_.begin(), leaderboard_.end(),
            [playerId](const ScoreEntry& e) { return e.playerId == playerId; }),
        leaderboard_.end());

    TraceLog(LOG_INFO, "Game: Player (ID: %llu) removed", playerId);
}

void Game::SpawnEnemy(const Vector3& position, float difficulty) {
    auto enemy = std::make_unique<Enemy>(enemies_.size(), position, difficulty);
    enemies_.push_back(std::move(enemy));
}

// ============================================================================
// Scoring
// ============================================================================

void Game::RegisterKill(uint64_t killerId, uint64_t victimId) {
    Player* killer = GetPlayerById(killerId);
    if (killer && killer->IsLocal()) {
        killer->AddKill();
    }

    auto it = scoreMap_.find(killerId);
    if (it != scoreMap_.end()) {
        it->second->kills++;
        it->second->score += 100;
    }

    // Check score limit
    if (config_.scoreLimit > 0 && it != scoreMap_.end()) {
        if (it->second->score >= config_.scoreLimit) {
            EndGame();
        }
    }
}

void Game::RegisterDamage(uint64_t attackerId, uint64_t victimId, float damage) {
    // Track assist damage
}

void Game::SortLeaderboard() {
    std::sort(leaderboard_.begin(), leaderboard_.end(),
        [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });

    // Rebuild score map
    scoreMap_.clear();
    for (auto& entry : leaderboard_) {
        scoreMap_[entry.playerId] = &entry;
    }
}

// ============================================================================
// Wave System
// ============================================================================

void Game::StartNextWave() {
    waveInfo_.waveNumber++;
    waveInfo_.enemiesTotal = 5 + waveInfo_.waveNumber * 3;
    waveInfo_.enemiesAlive = waveInfo_.enemiesTotal;
    waveInfo_.waveActive = true;
    waveInfo_.waveComplete = false;
    waveInfo_.waveTimer = 0.0f;

    // Spawn enemies for this wave
    float difficulty = config_.difficulty * (1.0f + waveInfo_.waveNumber * 0.15f);
    for (int i = 0; i < waveInfo_.enemiesTotal; i++) {
        Vector3 pos = map_->GetRandomEnemySpawnPoint();
        SpawnEnemy(pos, difficulty);
    }

    TraceLog(LOG_INFO, "Wave %d started: %d enemies (difficulty %.1f)",
        waveInfo_.waveNumber, waveInfo_.enemiesTotal, difficulty);
}

void Game::UpdateWaveSystem(float deltaTime) {
    if (!waveInfo_.waveActive) return;

    // Count alive enemies
    waveInfo_.enemiesAlive = 0;
    for (const auto& enemy : enemies_) {
        if (!enemy->IsDead()) waveInfo_.enemiesAlive++;
    }

    // Check if wave is complete
    if (waveInfo_.enemiesAlive == 0 && !waveInfo_.waveComplete) {
        waveInfo_.waveComplete = true;
        waveInfo_.waveTimer = 0.0f;
        TraceLog(LOG_INFO, "Wave %d complete!", waveInfo_.waveNumber);
    }

    // Wait between waves
    if (waveInfo_.waveComplete) {
        waveInfo_.waveTimer += deltaTime;
        if (waveInfo_.waveTimer >= waveInfo_.timeBetweenWaves) {
            StartNextWave();
        }
    }
}

// ============================================================================
// Spawn System
// ============================================================================

Vector3 Game::GetRandomSpawnPoint() {
    auto& spawns = map_->GetSpawnPoints();
    if (spawns.empty()) return {0, 1, 0};
    int idx = rand() % spawns.size();
    return spawns[idx];
}

void Game::RespawnPlayer(Player* player) {
    Vector3 spawnPos = GetRandomSpawnPoint();
    player->Respawn(spawnPos);

    if (player->IsLocal()) {
        cameraController_->SetPosition(spawnPos);
    }
}

// ============================================================================
// Network Callbacks
// ============================================================================

void Game::OnPlayerJoined(uint64_t playerId, const std::string& name) {
    AddRemotePlayer(playerId, name);
}

void Game::OnPlayerLeft(uint64_t playerId) {
    RemoveRemotePlayer(playerId);
}

void Game::OnPlayerUpdate(uint64_t playerId, const Vector3& pos, const Vector3& rot) {
    Player* player = GetPlayerById(playerId);
    if (player && !player->IsLocal()) {
        player->SetTargetPosition(pos);
        player->SetTargetRotation(rot);
    }
}

void Game::OnPlayerShot(uint64_t playerId, const Vector3& origin, const Vector3& direction) {
    Player* player = GetPlayerById(playerId);
    if (player && !player->IsLocal()) {
        Bullet bullet;
        bullet.position = origin;
        bullet.direction = direction;
        bullet.velocity = { direction.x * 300.0f, direction.y * 300.0f, direction.z * 300.0f };
        bullet.damage = 25.0f;
        bullet.ownerId = playerId;
        bullet.lifetime = 0.0f;
        bullet.maxLifetime = 2.0f;
        bullets_.push_back(bullet);

        particleSystem_->Emit(ParticleEmitterType::MUZZLE_FLASH, origin, direction, 5);
    }
}

void Game::OnPlayerHit(uint64_t playerId, float damage, uint64_t attackerId) {
    Player* player = GetPlayerById(playerId);
    if (player && player->IsLocal()) {
        player->TakeDamage(damage);
        cameraController_->AddShake(damage * 0.01f);
    }
}

// ============================================================================
// Android Touch Controls
// ============================================================================

#if defined(PLATFORM_ANDROID)
void Game::HandleTouchInput(float deltaTime) {
    // FIX #12: Guard against null localPlayer_ in touch input handling.
    if (!localPlayer_ || localPlayer_->IsDead()) return;

    int touchCount = GetTouchPointCount();

    // Reset per-frame state
    joystickDir_ = {0, 0};
    isShootingTouch_ = false;
    touchLookX_ = 0.0f;
    touchLookY_ = 0.0f;

    float joystickRadius = 80.0f;
    float joystickZoneLeft = screenWidth_ * 0.35f;  // Left 35% for joystick
    float shootZoneRight = screenWidth_ * 0.85f;    // Right 15% for shoot button

    for (int i = 0; i < touchCount; i++) {
        Vector2 pos = GetTouchPosition(i);

        if (pos.x < joystickZoneLeft) {
            // Left side: virtual joystick
            if (!joystickActive_) {
                joystickOrigin_ = pos;
                joystickActive_ = true;
                joystickTouchId_ = i;
            }
            if (i == joystickTouchId_) {
                float dx = pos.x - joystickOrigin_.x;
                float dy = pos.y - joystickOrigin_.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > 0.01f) {
                    joystickDir_.x = dx / joystickRadius;
                    joystickDir_.y = dy / joystickRadius;
                    if (dist > joystickRadius) {
                        joystickDir_.x = dx / dist;
                        joystickDir_.y = dy / dist;
                    }
                }
            }
        } else if (pos.x > shootZoneRight && pos.y > screenHeight_ * 0.5f) {
            // Right side bottom: shoot button
            isShootingTouch_ = true;
        } else if (pos.x > joystickZoneLeft && pos.x <= shootZoneRight) {
            // Middle area: look control
            if (lookTouchId_ < 0) {
                lookTouchId_ = i;
                lastLookPos_ = pos;
            }
            if (i == lookTouchId_) {
                touchLookX_ = (pos.x - lastLookPos_.x) * 0.005f;
                touchLookY_ = (pos.y - lastLookPos_.y) * 0.005f;
                lastLookPos_ = pos;
            }
        }
    }

    if (touchCount == 0) {
        joystickActive_ = false;
        joystickTouchId_ = -1;
        lookTouchId_ = -1;
    }

    // Apply joystick to player movement
    Vector3 moveDir = {joystickDir_.x, 0, joystickDir_.y};
    if (moveDir.x != 0 || moveDir.z != 0) {
        float sinYaw = sinf(localPlayer_->GetRotation().y);
        float cosYaw = cosf(localPlayer_->GetRotation().y);
        Vector3 rotatedDir = {
            moveDir.x * cosYaw - moveDir.z * sinYaw,
            0,
            moveDir.x * sinYaw + moveDir.z * cosYaw
        };
        localPlayer_->Move(rotatedDir, deltaTime);
    } else {
        localPlayer_->SetVelocity({0, localPlayer_->GetVelocity().y, 0});
    }

    // Apply look rotation
    if (touchLookX_ != 0 || touchLookY_ != 0) {
        Vector3 rot = localPlayer_->GetRotation();
        rot.y -= touchLookX_;
        rot.x -= touchLookY_;
        rot.x = std::clamp(rot.x, -1.5f, 1.5f);
        localPlayer_->SetRotation(rot);
    }

    // Shooting
    if (isShootingTouch_) {
        localPlayer_->Shoot();
    } else {
        localPlayer_->StopShooting();
    }

    // Reload: double tap right side
    static float lastTapTime = 0;
    static Vector2 lastTapPos = {0, 0};
    for (int i = 0; i < touchCount; i++) {
        Vector2 pos = GetTouchPosition(i);
        if (GetGestureDetected() == GESTURE_DOUBLETAP) {
            localPlayer_->Reload();
        }
    }
}

void Game::RenderTouchControls() {
    // FIX #11: Guard against null localPlayer_ in touch controls.
    // RenderTouchControls is only called during PLAYING state, but
    // localPlayer_ could theoretically be null if the game enters an
    // inconsistent state (e.g., during a restart or transition).
    if (!localPlayer_) return;

    // === Left Virtual Joystick ===
    float joystickRadius = 80.0f;
    Vector2 joyCenter = {150.0f, screenHeight_ - 150.0f};

    // Joystick base
    DrawCircleV(joyCenter, joystickRadius, Fade(GRAY, 0.3f));
    DrawCircleLines((int)joyCenter.x, (int)joyCenter.y, (int)joystickRadius, Fade(WHITE, 0.5f));

    // Joystick thumb
    if (joystickActive_) {
        Vector2 thumbPos = {
            joyCenter.x + joystickDir_.x * joystickRadius * 0.7f,
            joyCenter.y + joystickDir_.y * joystickRadius * 0.7f
        };
        DrawCircleV(thumbPos, 25, Fade(WHITE, 0.6f));
    } else {
        DrawCircleV(joyCenter, 25, Fade(WHITE, 0.3f));
    }

    // === Right Side: Shoot Button ===
    Vector2 shootCenter = {screenWidth_ - 120.0f, screenHeight_ - 150.0f};
    float shootRadius = 55.0f;
    DrawCircleV(shootCenter, shootRadius, isShootingTouch_ ? Fade(RED, 0.6f) : Fade(RED, 0.3f));
    DrawCircleLines((int)shootCenter.x, (int)shootCenter.y, (int)shootRadius, Fade(WHITE, 0.5f));
    DrawText("FIRE", (int)shootCenter.x - MeasureText("FIRE", 16) / 2, (int)shootCenter.y - 8, 16, WHITE);

    // === Reload Button ===
    Vector2 reloadCenter = {screenWidth_ - 120.0f, screenHeight_ - 280.0f};
    float reloadRadius = 35.0f;
    DrawCircleV(reloadCenter, reloadRadius, Fade(BLUE, 0.3f));
    DrawCircleLines((int)reloadCenter.x, (int)reloadCenter.y, (int)reloadRadius, Fade(WHITE, 0.5f));
    DrawText("R", (int)reloadCenter.x - 5, (int)reloadCenter.y - 8, 16, WHITE);

    // Check reload touch
    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 pos = GetTouchPosition(i);
        float dx = pos.x - reloadCenter.x;
        float dy = pos.y - reloadCenter.y;
        if (dx * dx + dy * dy < reloadRadius * reloadRadius) {
            localPlayer_->Reload();
        }
    }

    // === Jump Button ===
    Vector2 jumpCenter = {screenWidth_ - 220.0f, screenHeight_ - 200.0f};
    float jumpRadius = 30.0f;
    DrawCircleV(jumpCenter, jumpRadius, Fade(GREEN, 0.3f));
    DrawCircleLines((int)jumpCenter.x, (int)jumpCenter.y, (int)jumpRadius, Fade(WHITE, 0.5f));
    DrawText("J", (int)jumpCenter.x - 4, (int)jumpCenter.y - 8, 16, WHITE);

    // Check jump touch
    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 pos = GetTouchPosition(i);
        float dx = pos.x - jumpCenter.x;
        float dy = pos.y - jumpCenter.y;
        if (dx * dx + dy * dy < jumpRadius * jumpRadius) {
            localPlayer_->Jump();
        }
    }

    // === Weapon Switch ===
    for (int w = 0; w < 3; w++) {
        Vector2 wCenter = {screenWidth_ - 60.0f - w * 55.0f, 60.0f};
        DrawCircleV(wCenter, 22, (localPlayer_->GetCurrentWeapon() && w == 0) ? Fade(YELLOW, 0.5f) : Fade(GRAY, 0.3f));
        DrawText(TextFormat("%d", w + 1), (int)wCenter.x - 4, (int)wCenter.y - 8, 16, WHITE);

        for (int i = 0; i < GetTouchPointCount(); i++) {
            Vector2 pos = GetTouchPosition(i);
            float dx = pos.x - wCenter.x;
            float dy = pos.y - wCenter.y;
            if (dx * dx + dy * dy < 22 * 22) {
                localPlayer_->SwitchWeapon(w);
            }
        }
    }
}
#endif

} // namespace EOSShooter
