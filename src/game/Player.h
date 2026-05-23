#pragma once
// ============================================================================
// EOS Shooter - Player.h
// Player entity with movement, combat, inventory, and networking support.
// Enhanced with 3D weapon model rendering support.
// ============================================================================

#include "raylib.h"
#include "Weapon.h"
#include <string>
#include <array>
#include <vector>
#include <cstdint>
#include <memory>

namespace EOSShooter {

class ModelManager;  // Forward declaration

// Movement states for animation and physics
enum class MovementState {
    IDLE,
    WALKING,
    RUNNING,
    CROUCHING,
    JUMPING,
    FALLING,
    SLIDING
};

// Team identifiers for team-based modes
enum class TeamId : uint8_t {
    NONE = 0,
    TEAM_ALPHA = 1,
    TEAM_BRAVO = 2
};

// Player stats for end-of-match
struct PlayerStats {
    int kills = 0;
    int deaths = 0;
    int assists = 0;
    float damageDealt = 0.0f;
    float damageTaken = 0.0f;
    int headshots = 0;
    float playTime = 0.0f;
    int score = 0;
};

class Player {
public:
    Player(uint64_t playerId, const std::string& name, const Vector3& spawnPosition);
    ~Player() = default;

    // === Lifecycle ===
    void Update(float deltaTime);
    void Render();
    void Respawn(const Vector3& position);
    void Die();

    // === Input (local player only) ===
    void HandleInput(float deltaTime);

    // === Movement ===
    void Move(const Vector3& direction, float deltaTime);
    void Jump();
    void StartSprint();
    void StopSprint();
    void StartCrouch();
    void StopCrouch();
    void SetVelocity(const Vector3& vel) { velocity_ = vel; }
    void SetPosition(const Vector3& pos) { position_ = pos; }
    void SetRotation(const Vector3& rot) { rotation_ = rot; }

    // === Combat ===
    void Shoot();
    void StopShooting();
    void Reload();
    void SwitchWeapon(int slot);
    void TakeDamage(float damage);
    void Heal(float amount);
    void AddArmor(float amount);
    void RefillAmmo(int amount);
    void PickupWeapon(WeaponType type);
    bool IsShooting() const { return isShooting_; }
    bool IsDead() const { return isDead_; }
    bool IsReloading() const;
    bool IsAiming() const { return isAiming_; }

    // === Network Interpolation ===
    void SetTargetPosition(const Vector3& target) { targetPosition_ = target; }
    void SetTargetRotation(const Vector3& target) { targetRotation_ = target; }
    void InterpolatePosition(float deltaTime);
    void InterpolateRotation(float deltaTime);

    // === Network Serialization ===
    struct NetData {
        Vector3 position;
        Vector3 rotation;
        Vector3 velocity;
        MovementState moveState;
        float health;
        float armor;
        int currentWeaponSlot;
        bool isShooting;
        bool isAiming;
    };
    NetData Serialize() const;
    void Deserialize(const NetData& data);

    // === Getters ===
    uint64_t GetPlayerId() const { return playerId_; }
    const std::string& GetName() const { return name_; }
    Vector3 GetPosition() const { return position_; }
    Vector3 GetRotation() const { return rotation_; }
    Vector3 GetVelocity() const { return velocity_; }
    float GetHealth() const { return health_; }
    float GetArmor() const { return armor_; }
    float GetStamina() const { return stamina_; }
    float GetDeathTimer() const { return deathTimer_; }
    MovementState GetMovementState() const { return moveState_; }
    TeamId GetTeamId() const { return teamId_; }
    bool IsLocal() const { return isLocal_; }
    bool IsCrouching() const { return isCrouching_; }
    Weapon* GetCurrentWeapon();
    const Weapon* GetCurrentWeapon() const;
    Weapon* GetWeapon(int slot);
    BoundingBox GetBoundingBox() const;
    int GetKills() const { return stats_.kills; }
    int GetDeaths() const { return stats_.deaths; }
    int GetScore() const { return stats_.score; }
    const PlayerStats& GetStats() const { return stats_; }

    // === Setters ===
    void SetIsLocal(bool local) { isLocal_ = local; }
    void SetTeamId(TeamId team) { teamId_ = team; }
    void SetGroundLevel(float level) { groundLevel_ = level; }
    void AddKill() { stats_.kills++; stats_.score += 100; }
    void AddDeath() { stats_.deaths++; }
    void AddAssist() { stats_.assists++; stats_.score += 50; }

    // === Static Model Manager ===
    static ModelManager* s_modelManager;
    static void SetModelManager(ModelManager* mgr) { s_modelManager = mgr; }

private:
    // === Identity ===
    uint64_t playerId_;
    std::string name_;
    bool isLocal_ = false;
    TeamId teamId_ = TeamId::NONE;

    // === Transform ===
    Vector3 position_;
    Vector3 rotation_;          // Euler angles (pitch, yaw, roll)
    Vector3 velocity_;
    Vector3 targetPosition_;    // For network interpolation
    Vector3 targetRotation_;    // For network interpolation

    // === Movement ===
    MovementState moveState_ = MovementState::IDLE;
    bool isGrounded_ = true;
    bool isSprinting_ = false;
    bool isCrouching_ = false;
    float moveSpeed_ = 6.0f;
    float sprintSpeed_ = 10.0f;
    float crouchSpeed_ = 3.0f;
    float jumpForce_ = 8.0f;
    float gravity_ = -20.0f;
    float groundLevel_ = 0.0f;

    // === Health System ===
    float health_ = 100.0f;
    float maxHealth_ = 100.0f;
    float armor_ = 0.0f;
    float maxArmor_ = 100.0f;
    float stamina_ = 100.0f;
    float maxStamina_ = 100.0f;
    float staminaRegenRate_ = 20.0f;
    float staminaDrainRate_ = 25.0f;
    float healthRegenDelay_ = 5.0f;
    float healthRegenRate_ = 5.0f;
    float healthRegenTimer_ = 0.0f;
    bool isDead_ = false;
    float deathTimer_ = 0.0f;

    // === Combat ===
    bool isShooting_ = false;
    bool isAiming_ = false;     // ADS
    float aimFOV_ = 45.0f;
    float normalFOV_ = 70.0f;
    int currentWeaponSlot_ = 0;

    // === Weapon Inventory ===
    static constexpr int MAX_WEAPON_SLOTS = 3;
    std::array<std::unique_ptr<Weapon>, MAX_WEAPON_SLOTS> weapons_;

    // === Stats ===
    PlayerStats stats_;

    // === Rendering ===
    float modelHeight_ = 1.8f;
    float modelRadius_ = 0.4f;
    Color playerColor_ = BLUE;

    // === Internal Methods ===
    void UpdateMovement(float deltaTime);
    void UpdateStamina(float deltaTime);
    void UpdateHealthRegen(float deltaTime);
    void ApplyGravity(float deltaTime);
    void UpdateWeapon(float deltaTime);

    // === 3D Weapon Model Rendering ===
    void RenderWeaponModel3D(WeaponType weaponType, Vector3 weaponOffset,
                              float yaw, Color tint) const;
};

} // namespace EOSShooter
