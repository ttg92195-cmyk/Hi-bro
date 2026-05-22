#pragma once
// ============================================================================
// EOS Shooter - Game.h
// Main game class managing the game lifecycle, states, and entity systems.
// ============================================================================

#include "raylib.h"
#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "Map.h"
#include "ParticleSystem.h"
#include "CameraController.h"
#include "../multiplayer/EOSManager.h"
#include "../multiplayer/NetworkManager.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace EOSShooter {

// Game states that determine what's currently active
enum class GameState {
    MENU,           // Main menu screen
    LOBBY,          // Multiplayer lobby - finding/joining session
    LOADING,        // Loading map and assets
    PLAYING,        // Active gameplay
    PAUSED,         // Game paused (single-player only)
    GAME_OVER,      // Round/match ended
    SPECTATING      // Dead player spectating
};

// Game mode types
enum class GameMode {
    DEATHMATCH,         // Free-for-all
    TEAM_DEATHMATCH,    // Team vs Team
    BATTLE_ROYALE,      // Last player standing
    COOP_SURVIVAL,      // PvE waves
    CAPTURE_THE_FLAG    // CTF objective
};

// Configuration for a game session
struct GameConfig {
    GameMode mode = GameMode::DEATHMATCH;
    int maxPlayers = 16;
    int scoreLimit = 50;
    float timeLimit = 600.0f;      // 10 minutes
    bool friendlyFire = false;
    bool autoBalance = true;
    std::string mapName = "urban_warehouse";
    float difficulty = 1.0f;
};

// Score entry for leaderboard
struct ScoreEntry {
    uint64_t playerId = 0;
    std::string playerName;
    int kills = 0;
    int deaths = 0;
    int assists = 0;
    int score = 0;
    float playTime = 0.0f;
};

// Wave info for coop survival mode
struct WaveInfo {
    int waveNumber = 0;
    int enemiesTotal = 0;
    int enemiesAlive = 0;
    float waveTimer = 0.0f;
    float timeBetweenWaves = 10.0f;
    bool waveActive = false;
    bool waveComplete = false;
};

class Game {
public:
    Game();
    ~Game();

    // === Lifecycle ===
    bool Initialize(int screenWidth, int screenHeight, const std::string& title);
    void Run();
    void Shutdown();

    // === State Management ===
    void SetState(GameState newState);
    GameState GetState() const { return currentState_; }
    bool IsRunning() const { return isRunning_; }
    void RequestQuit() { isRunning_ = false; }

    // === Game Session ===
    bool StartGame(const GameConfig& config);
    void EndGame();
    void PauseGame();
    void ResumeGame();
    void RestartGame();

    // === Update/Render ===
    void Update(float deltaTime);
    void Render();

    // === Entity Management ===
    Player* GetLocalPlayer() { return localPlayer_; }
    Player* GetPlayerById(uint64_t playerId);
    const std::vector<std::unique_ptr<Player>>& GetAllPlayers() const { return players_; }
    const std::vector<std::unique_ptr<Enemy>>& GetAllEnemies() const { return enemies_; }
    const std::vector<Bullet>& GetAllBullets() const { return bullets_; }

    void AddRemotePlayer(uint64_t playerId, const std::string& name);
    void RemoveRemotePlayer(uint64_t playerId);
    void SpawnEnemy(const Vector3& position, float difficulty);

    // === Scoring ===
    void RegisterKill(uint64_t killerId, uint64_t victimId);
    void RegisterDamage(uint64_t attackerId, uint64_t victimId, float damage);
    const std::vector<ScoreEntry>& GetLeaderboard() const { return leaderboard_; }
    void SortLeaderboard();

    // === Wave System (Coop Survival) ===
    void StartNextWave();
    void UpdateWaveSystem(float deltaTime);
    const WaveInfo& GetWaveInfo() const { return waveInfo_; }

    // === Accessors ===
    GameConfig& GetConfig() { return config_; }
    const GameConfig& GetConfig() const { return config_; }
    Map* GetMap() { return map_.get(); }
    ParticleSystem* GetParticles() { return particleSystem_.get(); }
    CameraController* GetCamera() { return cameraController_.get(); }
    EOSManager* GetEOSManager() { return eosManager_.get(); }
    NetworkManager* GetNetworkManager() { return networkManager_.get(); }

    float GetDeltaTime() const { return deltaTime_; }
    float GetGameTime() const { return gameTime_; }
    int GetFPS() const { return fps_; }

private:
    // === Internal Update Methods ===
    void UpdatePlaying(float deltaTime);
    void UpdateMenu(float deltaTime);
    void UpdateLobby(float deltaTime);
    void UpdateLoading(float deltaTime);
    void UpdateGameOver(float deltaTime);
    void UpdateSpectating(float deltaTime);

    // === Internal Render Methods ===
    void RenderPlaying();
    void RenderMenu();
    void RenderLobby();
    void RenderLoading();
    void RenderGameOver();
    void RenderSpectating();
    void RenderDebugInfo();
    void RenderHUD();
    void RenderCrosshair();
    void RenderTouchControls(); // Android touch overlay
    void HandleTouchInput(float deltaTime); // Android touch input

    // === Collision ===
    void CheckBulletCollisions();
    void CheckPlayerEnemyCollisions();
    void CheckPickupCollisions();

    // === Spawn System ===
    Vector3 GetRandomSpawnPoint();
    void RespawnPlayer(Player* player);

    // === Network Callbacks ===
    void OnPlayerJoined(uint64_t playerId, const std::string& name);
    void OnPlayerLeft(uint64_t playerId);
    void OnPlayerUpdate(uint64_t playerId, const Vector3& pos, const Vector3& rot);
    void OnPlayerShot(uint64_t playerId, const Vector3& origin, const Vector3& direction);
    void OnPlayerHit(uint64_t playerId, float damage, uint64_t attackerId);

    // === Core State ===
    GameState currentState_ = GameState::MENU;
    GameState previousState_ = GameState::MENU;
    bool isRunning_ = false;
    bool isInitialized_ = false;
    bool isPaused_ = false;

    // === Timing ===
    float deltaTime_ = 0.0f;
    float gameTime_ = 0.0f;
    float stateTimer_ = 0.0f;
    float fixedAccumulator_ = 0.0f;
    const float fixedDeltaTime_ = 1.0f / 60.0f;
    int fps_ = 0;
    float fpsTimer_ = 0.0f;
    int frameCount_ = 0;

    // === Game Config ===
    GameConfig config_;
    WaveInfo waveInfo_;
    float matchTimer_ = 0.0f;
    bool matchEnded_ = false;

    // === Entities ===
    Player* localPlayer_ = nullptr;
    std::vector<std::unique_ptr<Player>> players_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<Bullet> bullets_;
    std::unordered_map<uint64_t, Player*> playerMap_;

    // === Systems ===
    std::unique_ptr<Map> map_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::unique_ptr<CameraController> cameraController_;
    std::unique_ptr<EOSManager> eosManager_;
    std::unique_ptr<NetworkManager> networkManager_;

    // === Scoring ===
    std::vector<ScoreEntry> leaderboard_;
    std::unordered_map<uint64_t, ScoreEntry*> scoreMap_;

    // === Touch Controls (Android) ===
#if defined(PLATFORM_ANDROID)
    Vector2 joystickOrigin_ = {0, 0};    // Left joystick center
    Vector2 joystickDir_ = {0, 0};       // Left joystick direction
    bool joystickActive_ = false;
    int joystickTouchId_ = -1;
    bool isShootingTouch_ = false;
    int shootTouchId_ = -1;
    float touchLookX_ = 0.0f;
    float touchLookY_ = 0.0f;
    int lookTouchId_ = -1;
    Vector2 lastLookPos_ = {0, 0};
#endif

    // === Screen ===
    int screenWidth_ = 1280;
    int screenHeight_ = 720;

    // === Visual Effects ===
    float menuAnimTime_ = 0.0f;
    float screenFade_ = 1.0f;
    float crosshairSpread_ = 0.0f;
    float hitMarkerTime_ = 0.0f;
    bool showHitMarker_ = false;
};

} // namespace EOSShooter
