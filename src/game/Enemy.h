#pragma once
// ============================================================================
// EOS Shooter - Enemy.h
// AI enemy with state machine, pathfinding, and squad behavior.
// ============================================================================

#include "raylib.h"
#include "Weapon.h"
#include <vector>
#include <cstdint>

namespace EOSShooter {

class ModelManager;  // Forward declaration

enum class AIState {
    IDLE,
    PATROL,
    CHASE,
    ATTACK,
    TAKE_COVER,
    FLANK,
    SEARCH,
    DEAD
};

enum class EnemyClass {
    GRUNT,          // Basic infantry
    HEAVY,          // Slow, armored, LMG
    SNIPER,         // Long range, high damage
    SHOTGUNNER,     // Close range ambush
    ROCKETEER       // Explosive, dangerous
};

class Enemy {
public:
    Enemy(uint64_t uniqueId, const Vector3& position, float difficulty);
    ~Enemy() = default;

    void Update(float deltaTime, class Player* player);
    void Render();
    void TakeDamage(float damage, uint64_t attackerId);
    void Die();

    // === AI State Machine ===
    void ChangeState(AIState newState);
    void UpdateAI(float deltaTime, class Player* player);
    void UpdateIdle(float deltaTime);
    void UpdatePatrol(float deltaTime);
    void UpdateChase(float deltaTime, class Player* player);
    void UpdateAttack(float deltaTime, class Player* player);
    void UpdateCover(float deltaTime);
    void UpdateFlank(float deltaTime, class Player* player);
    void UpdateSearch(float deltaTime);

    // === Detection ===
    bool CanSeePlayer(class Player* player) const;
    float GetDistanceToPlayer(class Player* player) const;
    void AlertNearbyEnemies();

    // === Getters ===
    uint64_t GetUniqueId() const { return uniqueId_; }
    Vector3 GetPosition() const { return position_; }
    float GetHealth() const { return health_; }
    bool IsDead() const { return aiState_ == AIState::DEAD; }
    float GetDeathTimer() const { return deathTimer_; }
    AIState GetAIState() const { return aiState_; }
    EnemyClass GetEnemyClass() const { return enemyClass_; }
    BoundingBox GetBoundingBox() const;

    // === Static Model Manager ===
    static ModelManager* s_modelManager;
    static void SetModelManager(ModelManager* mgr) { s_modelManager = mgr; }

    // === Static Map Pointer (for collision) ===
    static class Map* s_map;
    static void SetMap(class Map* m) { s_map = m; }

    // === Weapon Type for Model ===
    WeaponType GetWeaponTypeForClass() const;

private:
    // === Identity ===
    uint64_t uniqueId_;
    EnemyClass enemyClass_ = EnemyClass::GRUNT;

    // === Transform ===
    Vector3 position_;
    Vector3 rotation_;      // Euler angles
    Vector3 velocity_;

    // === Stats (scaled by difficulty) ===
    float health_ = 100.0f;
    float maxHealth_ = 100.0f;
    float armor_ = 0.0f;
    float moveSpeed_ = 3.0f;
    float damage_ = 15.0f;
    float fireRate_ = 2.0f;        // Shots per second
    float accuracy_ = 0.7f;        // 0-1, higher = more accurate
    float detectionRange_ = 30.0f;
    float attackRange_ = 25.0f;
    float fieldOfView_ = 1.2f;     // ~70 degrees in radians

    // === AI State ===
    AIState aiState_ = AIState::IDLE;
    float stateTimer_ = 0.0f;
    float alertLevel_ = 0.0f;      // 0-1, increases when hearing sounds
    Vector3 lastKnownPlayerPos_ = {0, 0, 0};
    bool hasPlayerLOS_ = false;

    // === Patrol ===
    std::vector<Vector3> patrolPoints_;
    int currentPatrolIndex_ = 0;
    float patrolWaitTime_ = 3.0f;
    bool waitingAtPatrolPoint_ = false;

    // === Combat ===
    float fireTimer_ = 0.0f;
    float burstTimer_ = 0.0f;
    int burstShotsRemaining_ = 0;
    float coverTimer_ = 0.0f;
    Vector3 coverPosition_ = {0, 0, 0};

    // === Death ===
    float deathTimer_ = 0.0f;

    // === Difficulty Scaling ===
    float difficulty_ = 1.0f;

    // === Helpers ===
    void MoveTowards(const Vector3& target, float speed, float deltaTime);
    void LookAt(const Vector3& target);
    void ShootAtPlayer(class Player* player, float deltaTime);
    Vector3 FindCoverPosition() const;
    Vector3 FindFlankPosition(class Player* player) const;
};

} // namespace EOSShooter
