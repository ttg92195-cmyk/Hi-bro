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
    screenWidth_ = GetScreenWidth();
    screenHeight_ = GetScreenHeight();
    if (screenWidth_ <= 0) screenWidth_ = 1280;
    if (screenHeight_ <= 0) screenHeight_ = 720;
    TraceLog(LOG_INFO, "Game: Android detected - using existing window (%dx%d)", screenWidth_, screenHeight_);
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth_, screenHeight_, title.c_str());
    SetWindowMinSize(800, 600);
#endif

    SetTargetFPS(60);

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

    if (!eosManager_->Initialize()) {
        TraceLog(LOG_WARNING, "EOS: Failed to initialize - multiplayer will be unavailable");
    }

    networkManager_->Initialize(eosManager_.get());

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

    cameraController_->Initialize(screenWidth_, screenHeight_);

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
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        lastTime = currentTime;
        deltaTime_ = std::min(elapsed.count(), 0.05f);

        frameCount_++;
        fpsTimer_ += deltaTime_;
        if (fpsTimer_ >= 1.0f) {
            fps_ = frameCount_;
            frameCount_ = 0;
            fpsTimer_ -= 1.0f;
        }

        fixedAccumulator_ += deltaTime_;
        while (fixedAccumulator_ >= fixedDeltaTime_) {
            fixedAccumulator_ -= fixedDeltaTime_;
        }

        eosManager_->Tick();

        Update(deltaTime_);
        Render();
    }

    Shutdown();
}

void Game::Shutdown() {
    if (!isInitialized_) return;

    TraceLog(LOG_INFO, "Game: Shutting down...");

    networkManager_->Shutdown();
    eosManager_->Shutdown();

    players_.clear();
    enemies_.clear();
    bullets_.clear();
    playerMap_.clear();
    leaderboard_.clear();
    scoreMap_.clear();

    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }

#if !defined(PLATFORM_ANDROID)
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
    screenFade_ = 1.0f;

    TraceLog(LOG_INFO, "Game: State changed from %d to %d", (int)previousState_, (int)currentState_);

    switch (currentState_) {
    case GameState::MENU:
#if !defined(PLATFORM_ANDROID)
        EnableCursor();
#endif
        break;

    case GameState::PLAYING:
#if !defined(PLATFORM_ANDROID)
        DisableCursor();
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

    if (!map_->Load(config_.mapName)) {
        TraceLog(LOG_ERROR, "Game: Failed to load map '%s'", config_.mapName.c_str());
        return false;
    }

    Vector3 spawnPos = GetRandomSpawnPoint();
    localPlayer_ = new Player(0, "LocalPlayer", spawnPos);
    localPlayer_->SetIsLocal(true);
    players_.emplace_back(localPlayer_);
    playerMap_[0] = localPlayer_;

    ScoreEntry entry;
    entry.playerId = 0;
    entry.playerName = "LocalPlayer";
    leaderboard_.push_back(entry);
    scoreMap_[0] = &leaderboard_.back();

    if (config_.mode == GameMode::COOP_SURVIVAL) {
        waveInfo_ = WaveInfo{};
        StartNextWave();
    }

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
    players_.clear();
    enemies_.clear();
    bullets_.clear();
    playerMap_.clear();
    leaderboard_.clear();
    scoreMap_.clear();
    localPlayer_ = nullptr;

    StartGame(config_);
}

// ============================================================================
// Update
// ============================================================================

void Game::Update(float deltaTime) {
    stateTimer_ += deltaTime;
    menuAnimTime_ += deltaTime;

    // Screen fade transition
    if (screenFade_ > 0.0f) {
        screenFade_ -= deltaTime * 2.0f;
        if (screenFade_ < 0.0f) screenFade_ = 0.0f;
    }

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

    // Update crosshair spread (decrease over time)
    crosshairSpread_ -= deltaTime * 80.0f;
    if (crosshairSpread_ < 0.0f) crosshairSpread_ = 0.0f;

    // Update hit marker timer
    if (showHitMarker_) {
        hitMarkerTime_ -= deltaTime;
        if (hitMarkerTime_ <= 0.0f) {
            showHitMarker_ = false;
            hitMarkerTime_ = 0.0f;
        }
    }

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

                direction = Math::ApplySpread(direction, weapon->GetCurrentSpread());

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

                // Increase crosshair spread when shooting
                crosshairSpread_ += 8.0f;
                if (crosshairSpread_ > 30.0f) crosshairSpread_ = 30.0f;

                particleSystem_->Emit(ParticleEmitterType::MUZZLE_FLASH, origin, direction, 5);

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

    // Update map (pickup respawn, animation timers)
    map_->Update(deltaTime);

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
    if (stateTimer_ > 2.0f) {
        SetState(GameState::PLAYING);
    }
}

void Game::UpdateGameOver(float deltaTime) {
    // Show final scores, wait for input to return to menu
}

void Game::UpdateSpectating(float deltaTime) {
    if (localPlayer_) {
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

    // Screen fade transition
    if (screenFade_ > 0.0f) {
        DrawRectangle(0, 0, screenWidth_, screenHeight_, Fade(BLACK, screenFade_));
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

    // 2D HUD overlay (AFTER EndMode3D)
    RenderHUD();
    RenderCrosshair();

#if defined(PLATFORM_ANDROID)
    // Touch controls overlay
    RenderTouchControls();
#endif
}

// ============================================================================
// Professional HUD Rendering
// ============================================================================

void Game::RenderHUD() {
    if (!localPlayer_) return;

    float sw = (float)screenWidth_;
    float sh = (float)screenHeight_;
    float pad = 20.0f;

    // --- Health Bar ---
    float barW = 250.0f;
    float barH = 20.0f;
    float barX = pad;
    float barY = sh - pad - barH;

    float healthPct = localPlayer_->GetHealth() / 100.0f;
    float armorPct = localPlayer_->GetArmor() / 100.0f;
    float staminaPct = localPlayer_->GetStamina() / 100.0f;

    // Health bar background
    DrawRectangle((int)barX - 2, (int)barY - 2, (int)barW + 4, (int)barH + 4, Fade(DARKGRAY, 0.8f));

    // Health bar fill - gradient from green to red
    Color hpColor;
    if (healthPct > 0.5f) {
        hpColor = { 0, 220, 80, 255 };
    } else if (healthPct > 0.25f) {
        hpColor = { 255, 200, 0, 255 };
    } else {
        // Pulse when low health
        float pulse = sinf(menuAnimTime_ * 8.0f) * 0.3f + 0.7f;
        hpColor = { 255, (uint8_t)(50 * pulse), 0, 255 };
    }
    DrawRectangle((int)barX, (int)barY, (int)(barW * healthPct), (int)barH, hpColor);

    // Health bar glow
    DrawRectangle((int)barX, (int)barY, (int)(barW * healthPct), 2, Fade(WHITE, 0.5f));

    // Health text
    DrawText(TextFormat("HP %.0f", localPlayer_->GetHealth()),
             (int)barX + 6, (int)barY + 2, 14, WHITE);

    // --- Armor Bar ---
    float armorBarY = barY - barH - 6;
    DrawRectangle((int)barX - 2, (int)armorBarY - 2, (int)barW + 4, (int)barH + 4, Fade(DARKGRAY, 0.8f));
    Color armorColor = { 40, 120, 255, 255 };
    DrawRectangle((int)barX, (int)armorBarY, (int)(barW * armorPct), (int)barH, armorColor);
    DrawRectangle((int)barX, (int)armorBarY, (int)(barW * armorPct), 2, Fade(WHITE, 0.4f));
    DrawText(TextFormat("ARM %.0f", localPlayer_->GetArmor()),
             (int)barX + 6, (int)armorBarY + 2, 14, WHITE);

    // --- Stamina Bar ---
    float stamBarW = 120.0f;
    float stamBarH = 8.0f;
    float stamBarY = armorBarY - stamBarH - 6;
    DrawRectangle((int)barX - 2, (int)stamBarY - 2, (int)stamBarW + 4, (int)stamBarH + 4, Fade(DARKGRAY, 0.7f));
    Color stamColor = { 255, 200, 40, 255 };
    DrawRectangle((int)barX, (int)stamBarY, (int)(stamBarW * staminaPct), (int)stamBarH, stamColor);

    // --- Ammo Counter ---
    Weapon* weapon = localPlayer_->GetCurrentWeapon();
    if (weapon) {
        int magAmmo = weapon->GetCurrentMagazine();
        int magSize = weapon->GetMagazineSize();
        int reserve = weapon->GetReserveAmmo();

        const char* weaponName = "WEAPON";
        switch (weapon->GetType()) {
        case WeaponType::ASSAULT_RIFLE: weaponName = "ASSAULT RIFLE"; break;
        case WeaponType::SMG:           weaponName = "SMG"; break;
        case WeaponType::SHOTGUN:       weaponName = "SHOTGUN"; break;
        case WeaponType::SNIPER_RIFLE:  weaponName = "SNIPER RIFLE"; break;
        case WeaponType::PISTOL:        weaponName = "PISTOL"; break;
        case WeaponType::RPG:           weaponName = "RPG"; break;
        case WeaponType::MELEE:         weaponName = "MELEE"; break;
        }

        float ammoX = sw - pad - 200;
        float ammoY = sh - pad - 60;

        // Ammo background panel
        DrawRectangle((int)ammoX - 10, (int)ammoY - 5, 210, 65, Fade(DARKGRAY, 0.6f));
        DrawRectangleLinesEx({ (float)ammoX - 10, (float)ammoY - 5, 210, 65 }, 1, Fade((Color){0, 220, 220, 255}, 0.4f));

        // Weapon name
        DrawText(weaponName, (int)ammoX, (int)ammoY, 14, Fade((Color){0, 220, 220, 255}, 0.9f));

        // Current ammo (large)
        Color ammoColor = (magAmmo <= magSize * 0.25f) ? (Color){255, 80, 80, 255} : WHITE;
        const char* ammoText = TextFormat("%d", magAmmo);
        int ammoTextW = MeasureText(ammoText, 36);
        DrawText(ammoText, (int)ammoX, (int)ammoY + 20, 36, ammoColor);

        // Reserve ammo
        const char* reserveText = TextFormat("/ %d", reserve);
        DrawText(reserveText, (int)ammoX + ammoTextW + 4, (int)ammoY + 32, 18, GRAY);

        // Reload indicator
        if (weapon->IsReloading()) {
            float reloadCX = sw * 0.5f;
            float reloadCY = sh * 0.5f + 60;

            // Circular reload progress
            DrawCircleLines((int)reloadCX, (int)reloadCY, 20, Fade(WHITE, 0.3f));
            const char* reloadText = "RELOADING";
            DrawText(reloadText, (int)reloadCX - MeasureText(reloadText, 16) / 2, (int)reloadCY + 25, 16, (Color){255, 200, 40, 255});
        }
    }

    // --- Wave Info Panel (Coop Survival) ---
    if (config_.mode == GameMode::COOP_SURVIVAL) {
        float panelW = 220.0f;
        float panelH = 60.0f;
        float panelX = sw * 0.5f - panelW * 0.5f;
        float panelY = pad;

        DrawRectangle((int)panelX, (int)panelY, (int)panelW, (int)panelH, Fade(DARKGRAY, 0.6f));
        DrawRectangleLinesEx({ panelX, panelY, panelW, panelH }, 1, Fade((Color){255, 100, 0, 255}, 0.6f));

        const char* waveText = TextFormat("WAVE %d", waveInfo_.waveNumber);
        DrawText(waveText, (int)panelX + (int)panelW / 2 - MeasureText(waveText, 22) / 2, (int)panelY + 5, 22, (Color){255, 140, 0, 255});

        const char* enemyText = TextFormat("Enemies: %d / %d", waveInfo_.enemiesAlive, waveInfo_.enemiesTotal);
        DrawText(enemyText, (int)panelX + (int)panelW / 2 - MeasureText(enemyText, 14) / 2, (int)panelY + 34, 14, Fade(WHITE, 0.8f));
    }

    // --- Minimap ---
    {
        float mmSize = 120.0f;
        float mmX = sw - pad - mmSize;
        float mmY = pad;
        float mmCenterX = mmX + mmSize * 0.5f;
        float mmCenterY = mmY + mmSize * 0.5f;
        float mmRadius = mmSize * 0.5f;

        // Minimap background circle
        DrawCircle((int)mmCenterX, (int)mmCenterY, (int)mmRadius, Fade(DARKGRAY, 0.7f));
        DrawCircleLines((int)mmCenterX, (int)mmCenterY, (int)mmRadius, Fade((Color){0, 200, 200, 255}, 0.5f));

        // Crosshair on minimap
        DrawLine((int)mmCenterX - 5, (int)mmCenterY, (int)mmCenterX + 5, (int)mmCenterY, Fade(WHITE, 0.3f));
        DrawLine((int)mmCenterX, (int)mmCenterY - 5, (int)mmCenterX, (int)mmCenterY + 5, Fade(WHITE, 0.3f));

        // Player position on minimap (center)
        DrawCircle((int)mmCenterX, (int)mmCenterY, 3, (Color){0, 220, 220, 255});

        // Enemy dots
        if (localPlayer_) {
            Vector3 playerPos = localPlayer_->GetPosition();
            Vector3 bounds = map_->GetBounds();
            float mapScale = mmRadius / std::max(bounds.x, bounds.z);

            for (const auto& enemy : enemies_) {
                if (enemy->IsDead()) continue;
                Vector3 ePos = enemy->GetPosition();
                float dx = (ePos.x - playerPos.x) * mapScale;
                float dz = (ePos.z - playerPos.z) * mapScale;
                // Rotate by player yaw
                float yaw = localPlayer_->GetRotation().y;
                float rx = dx * cosf(yaw) - dz * sinf(yaw);
                float ry = dx * sinf(yaw) + dz * cosf(yaw);
                int dotX = (int)(mmCenterX + rx);
                int dotY = (int)(mmCenterY + ry);
                // Check if within minimap circle
                float distFromCenter = sqrtf(rx * rx + ry * ry);
                if (distFromCenter < mmRadius - 2) {
                    DrawCircle(dotX, dotY, 2, (Color){255, 60, 60, 255});
                }
            }
        }
    }

    // --- Debug Info (compact) ---
    DrawFPS(10, 10);
    if (localPlayer_) {
        Vector3 pos = localPlayer_->GetPosition();
        DrawText(TextFormat("Pos: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z), 10, 30, 12, Fade(WHITE, 0.5f));
    }

    // EOS status
    DrawText(TextFormat("EOS: %s", eosManager_->IsLoggedIn() ? "Connected" : "Offline"),
             10, screenHeight_ - 20, 12, eosManager_->IsLoggedIn() ? Fade(GREEN, 0.6f) : Fade(RED, 0.6f));
}

// ============================================================================
// Dynamic Crosshair
// ============================================================================

void Game::RenderCrosshair() {
    float cx = (float)screenWidth_ * 0.5f;
    float cy = (float)screenHeight_ * 0.5f;
    float gap = 4.0f + crosshairSpread_;
    float lineLen = 10.0f;
    float thickness = 2.0f;

    Color chColor = showHitMarker_ ?
        Fade((Color){255, 60, 60, 255}, hitMarkerTime_ * 5.0f) :
        Fade(WHITE, 0.85f);

    // Top line
    DrawRectangle((int)(cx - thickness * 0.5f), (int)(cy - gap - lineLen), (int)thickness, (int)lineLen, chColor);
    // Bottom line
    DrawRectangle((int)(cx - thickness * 0.5f), (int)(cy + gap), (int)thickness, (int)lineLen, chColor);
    // Left line
    DrawRectangle((int)(cx - gap - lineLen), (int)(cy - thickness * 0.5f), (int)lineLen, (int)thickness, chColor);
    // Right line
    DrawRectangle((int)(cx + gap), (int)(cy - thickness * 0.5f), (int)lineLen, (int)thickness, chColor);

    // Center dot
    DrawCircle((int)cx, (int)cy, 2, chColor);

    // Hit marker X overlay
    if (showHitMarker_ && hitMarkerTime_ > 0.0f) {
        float alpha = hitMarkerTime_ * 4.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        Color hitColor = { 255, 60, 60, (uint8_t)(255 * alpha) };
        float hmSize = 8.0f;
        DrawLine((int)(cx - hmSize), (int)(cy - hmSize), (int)(cx - 3), (int)(cy - 3), hitColor);
        DrawLine((int)(cx + hmSize), (int)(cy - hmSize), (int)(cx + 3), (int)(cy - 3), hitColor);
        DrawLine((int)(cx - hmSize), (int)(cy + hmSize), (int)(cx - 3), (int)(cy + 3), hitColor);
        DrawLine((int)(cx + hmSize), (int)(cy + hmSize), (int)(cx + 3), (int)(cy + 3), hitColor);
    }
}

// ============================================================================
// AAA Menu Rendering
// ============================================================================

void Game::RenderMenu() {
    // === Animated Starfield Background ===
    for (int i = 0; i < 120; i++) {
        // Use deterministic math based on index + time for star positions
        float seed = (float)i * 7.31f;
        float sx = sinf(seed * 3.71f + menuAnimTime_ * (0.02f + fmodf(seed, 0.05f))) * 0.5f + 0.5f;
        float sy = cosf(seed * 2.33f + menuAnimTime_ * (0.01f + fmodf(seed, 0.03f))) * 0.5f + 0.5f;
        float brightness = sinf(menuAnimTime_ * 2.0f + seed) * 0.3f + 0.7f;
        int px = (int)(sx * screenWidth_);
        int py = (int)(sy * screenHeight_);
        uint8_t c = (uint8_t)(brightness * 200);
        DrawPixel(px, py, {c, c, (uint8_t)(c + 55 > 255 ? 255 : c + 55), 255});
        // Larger bright stars
        if (i % 7 == 0) {
            DrawCircle(px, py, 1, Fade((Color){180, 180, 255, 255}, brightness * 0.5f));
        }
    }

    // Dark gradient overlay
    for (int y = 0; y < screenHeight_; y += 4) {
        float t = (float)y / screenHeight_;
        uint8_t a = (uint8_t)(60 + t * 120);
        DrawRectangle(0, y, screenWidth_, 4, {5, 5, 15, a});
    }

    // === Scanline Overlay ===
    for (int y = 0; y < screenHeight_; y += 3) {
        DrawRectangle(0, y, screenWidth_, 1, Fade(BLACK, 0.08f));
    }

    // === Glowing Title "EOS SHOOTER" ===
    {
        const char* title = "EOS SHOOTER";
        int titleSize = 64;
        int titleW = MeasureText(title, titleSize);
        int titleX = screenWidth_ / 2 - titleW / 2;
        int titleY = 70;

        // Pulsing glow effect
        float pulse = sinf(menuAnimTime_ * 2.0f) * 0.3f + 0.7f;
        float glowPulse = sinf(menuAnimTime_ * 3.0f) * 0.2f + 0.8f;

        // Multiple layers of glow
        for (int layer = 3; layer >= 0; layer--) {
            Color glowColor;
            switch (layer) {
            case 3: glowColor = Fade((Color){0, 200, 200, 255}, 0.08f * glowPulse); break;
            case 2: glowColor = Fade((Color){0, 220, 220, 255}, 0.15f * glowPulse); break;
            case 1: glowColor = Fade((Color){100, 255, 255, 255}, 0.3f * pulse); break;
            case 0: glowColor = (Color){220, 255, 255, 255}; break;
            }
            int offset = layer * 2;
            DrawText(title, titleX - offset, titleY - offset, titleSize, glowColor);
            DrawText(title, titleX + offset, titleY + offset, titleSize, glowColor);
            DrawText(title, titleX, titleY, titleSize, glowColor);
        }

        // Main title text
        DrawText(title, titleX, titleY, titleSize, (Color){220, 255, 255, 255});

        // Underline glow
        float lineW = titleW * (0.8f + sinf(menuAnimTime_ * 1.5f) * 0.2f);
        int lineX = screenWidth_ / 2 - (int)lineW / 2;
        DrawRectangle(lineX, titleY + titleSize + 5, (int)lineW, 2, Fade((Color){0, 220, 220, 255}, pulse * 0.6f));
    }

    // === Subtitle with typewriter/fade effect ===
    {
        const char* subtitle = "MULTIPLAYER FPS";
        int subW = MeasureText(subtitle, 22);
        float subAlpha = std::min(stateTimer_ * 0.5f, 1.0f);
        DrawText(subtitle, screenWidth_ / 2 - subW / 2, 150, 22,
                 Fade((Color){0, 200, 200, 255}, subAlpha * 0.7f));
    }

#if defined(PLATFORM_ANDROID)
    // === Touch-friendly Gradient Buttons for Android ===
    int btnW = 400;
    int btnH = 60;
    int btnX = screenWidth_ / 2 - btnW / 2;

    // "Start Game" button
    {
        Rectangle startBtn = { (float)btnX, 250.0f, (float)btnW, (float)btnH };
        bool startHover = CheckCollisionPointRec(GetTouchPosition(0), startBtn) || CheckCollisionPointRec(GetMousePosition(), startBtn);

        // Button background gradient
        for (int i = 0; i < btnH; i++) {
            float t = (float)i / btnH;
            Color c = startHover ?
                (Color){ 0, (uint8_t)(180 + t * 40), (uint8_t)(80 + t * 40), 255 } :
                (Color){ 0, (uint8_t)(120 + t * 40), (uint8_t)(50 + t * 30), 220 };
            DrawRectangle(btnX, 250 + i, btnW, 1, c);
        }

        // Glow on hover
        if (startHover) {
            DrawRectangle(btnX - 3, 247, btnW + 6, btnH + 6, Fade((Color){0, 255, 128, 255}, 0.15f));
        }

        DrawRectangleLinesEx(startBtn, 2, startHover ? (Color){0, 255, 128, 255} : Fade(WHITE, 0.5f));
        const char* startText = "TAP TO START GAME";
        DrawText(startText, btnX + btnW / 2 - MeasureText(startText, 24) / 2, 262, 24, WHITE);
    }

    // "Multiplayer" button
    {
        Rectangle lobbyBtn = { (float)btnX, 330.0f, (float)btnW, (float)btnH };
        bool lobbyHover = CheckCollisionPointRec(GetTouchPosition(0), lobbyBtn) || CheckCollisionPointRec(GetMousePosition(), lobbyBtn);

        for (int i = 0; i < btnH; i++) {
            float t = (float)i / btnH;
            Color c = lobbyHover ?
                (Color){ (uint8_t)(140 + t * 40), 0, (uint8_t)(180 + t * 40), 255 } :
                (Color){ (uint8_t)(80 + t * 30), 0, (uint8_t)(120 + t * 30), 220 };
            DrawRectangle(btnX, 330 + i, btnW, 1, c);
        }

        if (lobbyHover) {
            DrawRectangle(btnX - 3, 327, btnW + 6, btnH + 6, Fade((Color){180, 0, 255, 255}, 0.15f));
        }

        DrawRectangleLinesEx(lobbyBtn, 2, lobbyHover ? (Color){180, 0, 255, 255} : Fade(WHITE, 0.5f));
        const char* lobbyText = "MULTIPLAYER LOBBY";
        DrawText(lobbyText, btnX + btnW / 2 - MeasureText(lobbyText, 24) / 2, 342, 24, WHITE);
    }

    // Check touch/click for start
    if ((IsKeyPressed(KEY_ENTER)) ||
        (GetGestureDetected() == GESTURE_TAP && CheckCollisionPointRec(GetTouchPosition(0), { (float)btnX, 250.0f, (float)btnW, (float)btnH })) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), { (float)btnX, 250.0f, (float)btnW, (float)btnH }))) {
        GameConfig config;
        config.mode = GameMode::COOP_SURVIVAL;
        config.mapName = "urban_warehouse";
        StartGame(config);
    }

    // Check touch/click for lobby
    if ((IsKeyPressed(KEY_L)) ||
        (GetGestureDetected() == GESTURE_TAP && CheckCollisionPointRec(GetTouchPosition(0), { (float)btnX, 330.0f, (float)btnW, (float)btnH })) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), { (float)btnX, 330.0f, (float)btnW, (float)btnH }))) {
        SetState(GameState::LOBBY);
    }
#else
    // === Desktop Gradient Buttons ===
    int btnW = 360;
    int btnH = 50;
    int btnX = screenWidth_ / 2 - btnW / 2;

    // "Start Game" button
    {
        Rectangle startBtn = { (float)btnX, 260.0f, (float)btnW, (float)btnH };
        bool startHover = CheckCollisionPointRec(GetMousePosition(), startBtn);

        for (int i = 0; i < btnH; i++) {
            float t = (float)i / btnH;
            Color c = startHover ?
                (Color){ 0, (uint8_t)(180 + t * 40), (uint8_t)(80 + t * 40), 255 } :
                (Color){ 0, (uint8_t)(100 + t * 40), (uint8_t)(40 + t * 30), 200 };
            DrawRectangle(btnX, 260 + i, btnW, 1, c);
        }

        if (startHover) {
            DrawRectangle(btnX - 3, 257, btnW + 6, btnH + 6, Fade((Color){0, 255, 128, 255}, 0.12f));
        }

        DrawRectangleLinesEx(startBtn, 2, startHover ? (Color){0, 255, 128, 255} : Fade(WHITE, 0.4f));
        const char* startText = "PRESS ENTER TO START";
        DrawText(startText, btnX + btnW / 2 - MeasureText(startText, 22) / 2, 272, 22, WHITE);

        if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && startHover)) {
            GameConfig config;
            config.mode = GameMode::COOP_SURVIVAL;
            config.mapName = "urban_warehouse";
            StartGame(config);
        }
    }

    // "Multiplayer" button
    {
        Rectangle lobbyBtn = { (float)btnX, 330.0f, (float)btnW, (float)btnH };
        bool lobbyHover = CheckCollisionPointRec(GetMousePosition(), lobbyBtn);

        for (int i = 0; i < btnH; i++) {
            float t = (float)i / btnH;
            Color c = lobbyHover ?
                (Color){ (uint8_t)(140 + t * 40), 0, (uint8_t)(180 + t * 40), 255 } :
                (Color){ (uint8_t)(70 + t * 30), 0, (uint8_t)(100 + t * 30), 200 };
            DrawRectangle(btnX, 330 + i, btnW, 1, c);
        }

        if (lobbyHover) {
            DrawRectangle(btnX - 3, 327, btnW + 6, btnH + 6, Fade((Color){180, 0, 255, 255}, 0.12f));
        }

        DrawRectangleLinesEx(lobbyBtn, 2, lobbyHover ? (Color){180, 0, 255, 255} : Fade(WHITE, 0.4f));
        const char* lobbyText = "PRESS L FOR MULTIPLAYER";
        DrawText(lobbyText, btnX + btnW / 2 - MeasureText(lobbyText, 22) / 2, 342, 22, WHITE);

        if (IsKeyPressed(KEY_L) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && lobbyHover)) {
            SetState(GameState::LOBBY);
        }
    }
#endif

    // === Version & Credits ===
    const char* verText = "v0.1.0 ALPHA";
    DrawText(verText, screenWidth_ / 2 - MeasureText(verText, 14) / 2, screenHeight_ - 50, 14, Fade(GRAY, 0.5f));
    const char* engineText = "Powered by Raylib";
    DrawText(engineText, screenWidth_ / 2 - MeasureText(engineText, 12) / 2, screenHeight_ - 30, 12, Fade(GRAY, 0.3f));
}

// ============================================================================
// Better Lobby Rendering
// ============================================================================

void Game::RenderLobby() {
    // Dark gradient background
    for (int y = 0; y < screenHeight_; y += 4) {
        float t = (float)y / screenHeight_;
        DrawRectangle(0, y, screenWidth_, 4, {(uint8_t)(10 + t * 10), (uint8_t)(5 + t * 5), (uint8_t)(20 + t * 15), 255});
    }

    // Title with glow
    const char* title = "MULTIPLAYER LOBBY";
    int titleW = MeasureText(title, 40);
    int titleX = screenWidth_ / 2 - titleW / 2;
    DrawText(title, titleX + 2, 52, 40, Fade((Color){255, 200, 0, 255}, 0.2f));
    DrawText(title, titleX - 2, 48, 40, Fade((Color){255, 200, 0, 255}, 0.2f));
    DrawText(title, titleX, 50, 40, (Color){255, 220, 50, 255});

    // Semi-transparent panel background
    DrawRectangle(screenWidth_ / 2 - 280, 120, 560, 300, Fade(DARKGRAY, 0.5f));
    DrawRectangleLinesEx({ (float)(screenWidth_ / 2 - 280), 120.0f, 560.0f, 300.0f }, 1, Fade((Color){255, 200, 0, 255}, 0.3f));

    // Instructions
    const char* instr1 = "Press C to Create Session";
    const char* instr2 = "Press J to Join Session";
    const char* instr3 = "Press ESC to go Back";

    DrawText(instr1, screenWidth_ / 2 - MeasureText(instr1, 22) / 2, 160, 22, WHITE);
    DrawText(instr2, screenWidth_ / 2 - MeasureText(instr2, 22) / 2, 200, 22, WHITE);
    DrawText(instr3, screenWidth_ / 2 - MeasureText(instr3, 20) / 2, 250, 20, Fade(GRAY, 0.7f));

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

    // Show found sessions as card-style entries
    auto sessions = eosManager_->GetFoundSessions();
    int y = 440;
    for (const auto& session : sessions) {
        // Session card
        DrawRectangle(screenWidth_ / 2 - 250, y - 4, 500, 30, Fade(DARKGRAY, 0.6f));
        DrawRectangleLinesEx({ (float)(screenWidth_ / 2 - 250), (float)(y - 4), 500.0f, 30.0f }, 1, Fade((Color){0, 220, 220, 255}, 0.3f));

        const char* sessionText = TextFormat("%s (%d/%d)", session.name.c_str(), session.currentPlayers, session.maxPlayers);
        DrawText(sessionText, screenWidth_ / 2 - 240, y, 18, (Color){0, 220, 220, 255});
        y += 40;
    }
}

// ============================================================================
// Better Loading Screen
// ============================================================================

void Game::RenderLoading() {
    float progress = std::min(stateTimer_ / 2.0f, 1.0f);

    // Dark background
    DrawRectangle(0, 0, screenWidth_, screenHeight_, (Color){8, 8, 20, 255});

    // Animated spinner dots
    {
        float cx = (float)screenWidth_ * 0.5f;
        float cy = (float)screenHeight_ * 0.5f - 40;
        for (int i = 0; i < 8; i++) {
            float angle = (float)i * PI * 0.25f + menuAnimTime_ * 3.0f;
            float radius = 20.0f;
            int dx = (int)(cx + cosf(angle) * radius);
            int dy = (int)(cy + sinf(angle) * radius);
            float dotAlpha = sinf(menuAnimTime_ * 6.0f + (float)i * 0.8f) * 0.3f + 0.7f;
            DrawCircle(dx, dy, 3, Fade((Color){0, 220, 220, 255}, dotAlpha));
        }
    }

    // "Loading..." text with fade
    const char* loadingText = "LOADING...";
    int textW = MeasureText(loadingText, 30);
    float textAlpha = sinf(menuAnimTime_ * 2.0f) * 0.2f + 0.8f;
    DrawText(loadingText, screenWidth_ / 2 - textW / 2, screenHeight_ / 2 - 20, 30,
             Fade(WHITE, textAlpha));

    // Progress bar background
    int barW = 400;
    int barH = 16;
    int barX = screenWidth_ / 2 - barW / 2;
    int barY = screenHeight_ / 2 + 30;

    DrawRectangle(barX, barY, barW, barH, Fade(DARKGRAY, 0.8f));

    // Progress bar gradient fill
    int fillW = (int)(barW * progress);
    for (int i = 0; i < fillW; i++) {
        float t = (float)i / barW;
        Color c = {
            (uint8_t)(0 + t * 200),
            (uint8_t)(180 + t * 40),
            (uint8_t)(220 - t * 100),
            255
        };
        DrawRectangle(barX + i, barY, 1, barH, c);
    }

    // Progress bar glow on top edge
    DrawRectangle(barX, barY, fillW, 2, Fade(WHITE, 0.5f));

    // Border
    DrawRectangleLines(barX, barY, barW, barH, Fade((Color){0, 220, 220, 255}, 0.6f));

    // Percentage text
    const char* pctText = TextFormat("%d%%", (int)(progress * 100));
    DrawText(pctText, screenWidth_ / 2 - MeasureText(pctText, 14) / 2, barY + barH + 5, 14,
             Fade(WHITE, 0.7f));
}

// ============================================================================
// Epic Game Over Screen
// ============================================================================

void Game::RenderGameOver() {
    // Dark overlay with red tint
    DrawRectangle(0, 0, screenWidth_, screenHeight_, {30, 0, 0, 220});

    // Vignette effect
    for (int i = 0; i < 40; i++) {
        DrawRectangle(0, 0, screenWidth_, i, Fade(BLACK, (float)(40 - i) / 40.0f * 0.5f));
        DrawRectangle(0, screenHeight_ - i, screenWidth_, i, Fade(BLACK, (float)(40 - i) / 40.0f * 0.5f));
    }

    // "GAME OVER" with glow effect
    {
        const char* goText = "GAME OVER";
        int goSize = 56;
        int goW = MeasureText(goText, goSize);
        int goX = screenWidth_ / 2 - goW / 2;
        int goY = 60;

        float pulse = sinf(menuAnimTime_ * 2.0f) * 0.3f + 0.7f;

        // Glow layers
        DrawText(goText, goX - 3, goY - 3, goSize, Fade((Color){255, 0, 0, 255}, 0.15f * pulse));
        DrawText(goText, goX + 3, goY + 3, goSize, Fade((Color){255, 0, 0, 255}, 0.15f * pulse));
        DrawText(goText, goX - 1, goY - 1, goSize, Fade((Color){255, 50, 50, 255}, 0.3f * pulse));
        DrawText(goText, goX + 1, goY + 1, goSize, Fade((Color){255, 50, 50, 255}, 0.3f * pulse));

        // Main text
        DrawText(goText, goX, goY, goSize, (Color){255, 80, 80, 255});

        // Underline
        DrawRectangle(goX, goY + goSize + 5, goW, 2, Fade((Color){255, 50, 50, 255}, pulse * 0.5f));
    }

    // Leaderboard panel
    SortLeaderboard();
    int panelX = screenWidth_ / 2 - 300;
    int panelW = 600;
    int panelY = 150;

    DrawRectangle(panelX, panelY, panelW, 50, Fade(DARKGRAY, 0.7f));
    DrawRectangleLinesEx({ (float)panelX, (float)panelY, (float)panelW, 50.0f }, 1, Fade((Color){255, 200, 0, 255}, 0.4f));

    const char* headerText = "RANK   PLAYER            KILLS  DEATHS  SCORE";
    DrawText(headerText, panelX + 20, panelY + 14, 18, (Color){255, 200, 0, 255});

    int y = panelY + 55;
    for (size_t i = 0; i < leaderboard_.size() && i < 10; i++) {
        const auto& entry = leaderboard_[i];

        // Card background for each entry
        Color rowBg = (i % 2 == 0) ? Fade(DARKGRAY, 0.4f) : Fade(DARKGRAY, 0.25f);
        DrawRectangle(panelX, y - 2, panelW, 30, rowBg);

        Color textColor;
        Color rankColor;
        switch (i) {
        case 0: rankColor = GOLD; textColor = GOLD; break;
        case 1: rankColor = LIGHTGRAY; textColor = LIGHTGRAY; break;
        case 2: rankColor = (Color){205, 127, 50, 255}; textColor = (Color){205, 127, 50, 255}; break;
        default: rankColor = WHITE; textColor = WHITE; break;
        }

        const char* rankStr = TextFormat("#%-3d", (int)(i + 1));
        DrawText(rankStr, panelX + 20, y + 2, 16, rankColor);

        const char* lineText = TextFormat("%-15s  %5d  %5d  %6d",
            entry.playerName.c_str(), entry.kills, entry.deaths, entry.score);
        DrawText(lineText, panelX + 70, y + 2, 16, textColor);

        y += 32;
    }

    // Bottom instructions
    const char* instrText = "Press ENTER to Play Again | ESC for Menu";
    DrawText(instrText, screenWidth_ / 2 - MeasureText(instrText, 20) / 2, screenHeight_ - 80, 20, GRAY);

#if defined(PLATFORM_ANDROID)
    Rectangle retryBtn = { (float)(screenWidth_ / 2 - 200), (float)(screenHeight_ - 120), 180.0f, 50.0f };
    Rectangle menuBtn = { (float)(screenWidth_ / 2 + 20), (float)(screenHeight_ - 120), 180.0f, 50.0f };

    // Gradient retry button
    for (int i = 0; i < 50; i++) {
        float t = (float)i / 50.0f;
        DrawRectangle((int)retryBtn.x, (int)retryBtn.y + i, 180, 1, (Color){0, (uint8_t)(140 + t * 40), (uint8_t)(60 + t * 30), 255});
    }
    DrawRectangleLinesEx(retryBtn, 2, Fade(WHITE, 0.5f));
    DrawText("PLAY AGAIN", (int)retryBtn.x + 25, (int)retryBtn.y + 14, 20, WHITE);

    DrawRectangleRec(menuBtn, Fade(DARKGRAY, 0.7f));
    DrawRectangleLinesEx(menuBtn, 2, Fade(WHITE, 0.4f));
    DrawText("MENU", (int)menuBtn.x + 60, (int)menuBtn.y + 14, 20, WHITE);

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

// ============================================================================
// Spectating Screen
// ============================================================================

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

    // Spectating overlay
    const char* specText = "SPECTATING";
    int specW = MeasureText(specText, 30);
    DrawText(specText, screenWidth_ / 2 - specW / 2, 50, 30, (Color){255, 200, 0, 255});
}

// ============================================================================
// Legacy Debug Info (kept for compatibility, HUD is the new default)
// ============================================================================

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

    DrawText(TextFormat("EOS: %s", eosManager_->IsLoggedIn() ? "Connected" : "Offline"),
             10, screenHeight_ - 30, 16, eosManager_->IsLoggedIn() ? GREEN : RED);
}

// ============================================================================
// Collision Detection
// ============================================================================

void Game::CheckBulletCollisions() {
    for (auto& bullet : bullets_) {
        if (bullet.HasHit()) continue;

        Ray ray = { bullet.position, bullet.direction };
        RayCollision mapHit = map_->Raycast(ray, bullet.range - bullet.distance);

        if (mapHit.hit) {
            bullet.SetHit(true);
            particleSystem_->Emit(ParticleEmitterType::SPARK, mapHit.point, {0,1,0}, 8);
            continue;
        }

        for (auto& enemy : enemies_) {
            if (enemy->IsDead()) continue;
            BoundingBox enemyBox = enemy->GetBoundingBox();
            RayCollision enemyHit = GetRayCollisionBox(ray, enemyBox);
            if (enemyHit.hit) {
                bullet.SetHit(true);
                enemy->TakeDamage(bullet.damage, bullet.ownerId);
                particleSystem_->Emit(ParticleEmitterType::BLOOD, bullet.position, bullet.direction, 10);

                // Show hit marker
                showHitMarker_ = true;
                hitMarkerTime_ = 0.3f;

                if (enemy->IsDead()) {
                    RegisterKill(bullet.ownerId, enemy->GetUniqueId());
                    particleSystem_->Emit(ParticleEmitterType::EXPLOSION, enemy->GetPosition(), {0,1,0}, 20);
                }
                break;
            }
        }

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

                    showHitMarker_ = true;
                    hitMarkerTime_ = 0.3f;

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
            pickup.respawnTimer = 30.0f;
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

    if (config_.scoreLimit > 0 && it != scoreMap_.end()) {
        if (it->second->score >= config_.scoreLimit) {
            EndGame();
        }
    }
}

void Game::RegisterDamage(uint64_t attackerId, uint64_t victimId, float damage) {
}

void Game::SortLeaderboard() {
    std::sort(leaderboard_.begin(), leaderboard_.end(),
        [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });

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

    waveInfo_.enemiesAlive = 0;
    for (const auto& enemy : enemies_) {
        if (!enemy->IsDead()) waveInfo_.enemiesAlive++;
    }

    if (waveInfo_.enemiesAlive == 0 && !waveInfo_.waveComplete) {
        waveInfo_.waveComplete = true;
        waveInfo_.waveTimer = 0.0f;
        TraceLog(LOG_INFO, "Wave %d complete!", waveInfo_.waveNumber);
    }

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
    if (!localPlayer_ || localPlayer_->IsDead()) return;

    int touchCount = GetTouchPointCount();

    joystickDir_ = {0, 0};
    isShootingTouch_ = false;
    touchLookX_ = 0.0f;
    touchLookY_ = 0.0f;

    float joystickRadius = 80.0f;
    float joystickZoneLeft = screenWidth_ * 0.35f;
    float shootZoneRight = screenWidth_ * 0.85f;

    for (int i = 0; i < touchCount; i++) {
        Vector2 pos = GetTouchPosition(i);

        if (pos.x < joystickZoneLeft) {
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
            isShootingTouch_ = true;
        } else if (pos.x > joystickZoneLeft && pos.x <= shootZoneRight) {
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

    if (touchLookX_ != 0 || touchLookY_ != 0) {
        Vector3 rot = localPlayer_->GetRotation();
        rot.y -= touchLookX_;
        rot.x -= touchLookY_;
        rot.x = std::clamp(rot.x, -1.5f, 1.5f);
        localPlayer_->SetRotation(rot);
    }

    if (isShootingTouch_) {
        localPlayer_->Shoot();
    } else {
        localPlayer_->StopShooting();
    }

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
    if (!localPlayer_) return;

    // === Left Virtual Joystick ===
    float joystickRadius = 80.0f;
    Vector2 joyCenter = {150.0f, (float)screenHeight_ - 150.0f};

    // Joystick base - gradient circle
    DrawCircle((int)joyCenter.x, (int)joyCenter.y, (int)(joystickRadius + 4), Fade((Color){0, 100, 100, 255}, 0.08f));
    DrawCircle((int)joyCenter.x, (int)joyCenter.y, (int)joystickRadius, Fade((Color){0, 60, 60, 255}, 0.25f));
    DrawCircleLines((int)joyCenter.x, (int)joyCenter.y, (int)joystickRadius, Fade((Color){0, 200, 200, 255}, 0.35f));

    // Inner ring
    DrawCircleLines((int)joyCenter.x, (int)joyCenter.y, (int)(joystickRadius * 0.5f), Fade((Color){0, 200, 200, 255}, 0.15f));

    // Joystick thumb
    if (joystickActive_) {
        Vector2 thumbPos = {
            joyCenter.x + joystickDir_.x * joystickRadius * 0.7f,
            joyCenter.y + joystickDir_.y * joystickRadius * 0.7f
        };
        DrawCircle((int)thumbPos.x, (int)thumbPos.y, 22, Fade((Color){0, 200, 200, 255}, 0.5f));
        DrawCircle((int)thumbPos.x, (int)thumbPos.y, 18, Fade((Color){0, 255, 255, 255}, 0.3f));
    } else {
        DrawCircle((int)joyCenter.x, (int)joyCenter.y, 18, Fade(WHITE, 0.15f));
    }

    // === Right Side: Shoot Button with pulsing glow ===
    Vector2 shootCenter = {(float)screenWidth_ - 120.0f, (float)screenHeight_ - 150.0f};
    float shootRadius = 55.0f;
    float shootPulse = sinf(menuAnimTime_ * 4.0f) * 0.1f + 0.9f;

    if (isShootingTouch_) {
        // Active glow
        DrawCircle((int)shootCenter.x, (int)shootCenter.y, (int)(shootRadius + 10), Fade(RED, 0.12f));
        DrawCircle((int)shootCenter.x, (int)shootCenter.y, (int)shootRadius, Fade(RED, 0.55f));
    } else {
        DrawCircle((int)shootCenter.x, (int)shootCenter.y, (int)(shootRadius + 6), Fade(RED, 0.06f * shootPulse));
        DrawCircle((int)shootCenter.x, (int)shootCenter.y, (int)shootRadius, Fade(RED, 0.25f));
    }
    DrawCircleLines((int)shootCenter.x, (int)shootCenter.y, (int)shootRadius, Fade(WHITE, 0.4f));
    const char* fireText = "FIRE";
    DrawText(fireText, (int)shootCenter.x - MeasureText(fireText, 16) / 2, (int)shootCenter.y - 8, 16, WHITE);

    // === Reload Button ===
    Vector2 reloadCenter = {(float)screenWidth_ - 120.0f, (float)screenHeight_ - 280.0f};
    float reloadRadius = 35.0f;
    DrawCircle((int)reloadCenter.x, (int)reloadCenter.y, (int)reloadRadius, Fade((Color){40, 80, 200, 255}, 0.25f));
    DrawCircleLines((int)reloadCenter.x, (int)reloadCenter.y, (int)reloadRadius, Fade((Color){80, 140, 255, 255}, 0.4f));
    DrawText("R", (int)reloadCenter.x - 5, (int)reloadCenter.y - 8, 16, (Color){120, 180, 255, 255});

    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 pos = GetTouchPosition(i);
        float dx = pos.x - reloadCenter.x;
        float dy = pos.y - reloadCenter.y;
        if (dx * dx + dy * dy < reloadRadius * reloadRadius) {
            localPlayer_->Reload();
        }
    }

    // === Jump Button ===
    Vector2 jumpCenter = {(float)screenWidth_ - 220.0f, (float)screenHeight_ - 200.0f};
    float jumpRadius = 30.0f;
    DrawCircle((int)jumpCenter.x, (int)jumpCenter.y, (int)jumpRadius, Fade((Color){0, 150, 80, 255}, 0.25f));
    DrawCircleLines((int)jumpCenter.x, (int)jumpCenter.y, (int)jumpRadius, Fade((Color){0, 220, 120, 255}, 0.4f));
    DrawText("J", (int)jumpCenter.x - 4, (int)jumpCenter.y - 8, 16, (Color){0, 220, 120, 255});

    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 pos = GetTouchPosition(i);
        float dx = pos.x - jumpCenter.x;
        float dy = pos.y - jumpCenter.y;
        if (dx * dx + dy * dy < jumpRadius * jumpRadius) {
            if (localPlayer_) localPlayer_->Jump();
        }
    }
}
#endif

} // namespace EOSShooter
