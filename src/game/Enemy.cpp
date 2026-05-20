// ============================================================================
// EOS Shooter - Enemy.cpp
// Full AI implementation with state machine, detection, and squad behavior.
// ============================================================================

#include "Enemy.h"
#include "Player.h"
#include "Map.h"
#include "Game.h"
#include "../utils/Math.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace EOSShooter {

// ============================================================================
// Construction
// ============================================================================

Enemy::Enemy(uint64_t uniqueId, const Vector3& position, float difficulty)
    : uniqueId_(uniqueId)
    , position_(position)
    , rotation_({0, 0, 0})
    , velocity_({0, 0, 0})
    , difficulty_(difficulty)
{
    // Determine enemy class randomly (weighted)
    float roll = (float)(rand() % 100) / 100.0f;
    if (roll < 0.50f)      enemyClass_ = EnemyClass::GRUNT;
    else if (roll < 0.70f) enemyClass_ = EnemyClass::HEAVY;
    else if (roll < 0.85f) enemyClass_ = EnemyClass::SNIPER;
    else if (roll < 0.95f) enemyClass_ = EnemyClass::SHOTGUNNER;
    else                    enemyClass_ = EnemyClass::ROCKETEER;

    // Initialize stats based on class and difficulty
    switch (enemyClass_) {
    case EnemyClass::GRUNT:
        health_ = 80.0f * difficulty;
        moveSpeed_ = 3.5f * difficulty;
        damage_ = 18.0f * difficulty;
        fireRate_ = 3.0f * difficulty;
        accuracy_ = 0.5f + difficulty * 0.15f;
        detectionRange_ = 30.0f;
        attackRange_ = 25.0f;
        break;
    case EnemyClass::HEAVY:
        health_ = 200.0f * difficulty;
        armor_ = 50.0f * difficulty;
        moveSpeed_ = 2.0f;
        damage_ = 12.0f * difficulty;
        fireRate_ = 8.0f * difficulty;
        accuracy_ = 0.4f + difficulty * 0.1f;
        detectionRange_ = 25.0f;
        attackRange_ = 20.0f;
        break;
    case EnemyClass::SNIPER:
        health_ = 60.0f * difficulty;
        moveSpeed_ = 2.5f;
        damage_ = 70.0f * difficulty;
        fireRate_ = 0.8f * difficulty;
        accuracy_ = 0.8f + difficulty * 0.1f;
        detectionRange_ = 60.0f;
        attackRange_ = 55.0f;
        break;
    case EnemyClass::SHOTGUNNER:
        health_ = 100.0f * difficulty;
        moveSpeed_ = 4.5f * difficulty;
        damage_ = 40.0f * difficulty;  // Per shot, multiple pellets
        fireRate_ = 1.5f;
        accuracy_ = 0.3f;
        detectionRange_ = 15.0f;
        attackRange_ = 10.0f;
        break;
    case EnemyClass::ROCKETEER:
        health_ = 90.0f * difficulty;
        moveSpeed_ = 2.5f;
        damage_ = 120.0f * difficulty;
        fireRate_ = 0.4f;
        accuracy_ = 0.5f + difficulty * 0.15f;
        detectionRange_ = 40.0f;
        attackRange_ = 35.0f;
        break;
    }

    maxHealth_ = health_;

    // Generate patrol points around spawn position
    for (int i = 0; i < 4; i++) {
        float angle = (float)i * PI * 0.5f;
        Vector3 point = {
            position_.x + cosf(angle) * 8.0f,
            position_.y,
            position_.z + sinf(angle) * 8.0f
        };
        patrolPoints_.push_back(point);
    }
}

// ============================================================================
// Lifecycle
// ============================================================================

void Enemy::Update(float deltaTime, Player* player) {
    if (aiState_ == AIState::DEAD) {
        deathTimer_ += deltaTime;
        return;
    }

    stateTimer_ += deltaTime;
    UpdateAI(deltaTime, player);

    // Apply gravity
    if (position_.y > 0) {
        velocity_.y -= 20.0f * deltaTime;
        position_.y += velocity_.y * deltaTime;
    }
    if (position_.y < 0) {
        position_.y = 0;
        velocity_.y = 0;
    }
}

void Enemy::Render() {
    if (aiState_ == AIState::DEAD) return;

    Color bodyColor;
    switch (enemyClass_) {
    case EnemyClass::GRUNT:     bodyColor = DARKGREEN; break;
    case EnemyClass::HEAVY:     bodyColor = DARKGRAY; break;
    case EnemyClass::SNIPER:    bodyColor = MAROON; break;
    case EnemyClass::SHOTGUNNER: bodyColor = BROWN; break;
    case EnemyClass::ROCKETEER: bodyColor = ORANGE; break;
    default: bodyColor = DARKGREEN; break;
    }

    // Body
    float height = (enemyClass_ == EnemyClass::HEAVY) ? 2.0f : 1.8f;
    DrawCapsule(position_, {position_.x, position_.y + height, position_.z},
                0.4f, 8, 8, bodyColor);

    // Head
    DrawSphere({position_.x, position_.y + height + 0.2f, position_.z}, 0.25f, SKYBLUE);

    // Health bar above head
    float healthPercent = health_ / maxHealth_;
    Vector3 barPos = {position_.x, position_.y + height + 0.7f, position_.z};
    DrawBillboard({0}, Rectangle{0, 0, 1, 0.1f}, barPos, 1.0f, WHITE); // Simplified

    // Simple health bar using 3D lines
    Vector3 barStart = {position_.x - 0.5f, position_.y + height + 0.6f, position_.z};
    Vector3 barEnd = {position_.x + 0.5f, position_.y + height + 0.6f, position_.z};
    Vector3 healthEnd = {position_.x - 0.5f + healthPercent, position_.y + height + 0.6f, position_.z};
    Color hpColor = (healthPercent > 0.5f) ? GREEN : (healthPercent > 0.25f) ? YELLOW : RED;
    DrawLine3D(barStart, healthEnd, hpColor);
    DrawLine3D(barStart, barEnd, DARKGRAY);

    // Weapon visual
    Vector3 weaponOffset = {
        position_.x + sinf(rotation_.y) * 0.5f,
        position_.y + height * 0.7f,
        position_.z + cosf(rotation_.y) * 0.5f
    };
    DrawCube(weaponOffset, 0.06f, 0.06f, 0.4f, DARKGRAY);
}

void Enemy::TakeDamage(float damage, uint64_t attackerId) {
    if (aiState_ == AIState::DEAD) return;

    // Apply armor reduction
    float effectiveDamage = damage;
    if (armor_ > 0) {
        float absorbed = std::min(armor_, damage * 0.5f);
        armor_ -= absorbed;
        effectiveDamage -= absorbed * 0.5f;
    }

    health_ -= effectiveDamage;
    alertLevel_ = 1.0f; // Maximum alert when hit

    if (health_ <= 0) {
        health_ = 0;
        Die();
    } else {
        // Flinch: switch to chase if idle/patrol
        if (aiState_ == AIState::IDLE || aiState_ == AIState::PATROL) {
            ChangeState(AIState::CHASE);
        }
    }
}

void Enemy::Die() {
    ChangeState(AIState::DEAD);
    deathTimer_ = 0.0f;
}

// ============================================================================
// AI State Machine
// ============================================================================

void Enemy::ChangeState(AIState newState) {
    if (aiState_ == newState) return;
    aiState_ = newState;
    stateTimer_ = 0.0f;
}

void Enemy::UpdateAI(float deltaTime, Player* player) {
    if (!player || player->IsDead()) {
        if (aiState_ != AIState::IDLE && aiState_ != AIState::PATROL) {
            ChangeState(AIState::SEARCH);
        }
        return;
    }

    // Check line of sight to player
    hasPlayerLOS_ = CanSeePlayer(player);
    float distToPlayer = GetDistanceToPlayer(player);

    if (hasPlayerLOS_) {
        lastKnownPlayerPos_ = player->GetPosition();
        alertLevel_ = std::min(alertLevel_ + deltaTime * 2.0f, 1.0f);
    } else {
        alertLevel_ = std::max(alertLevel_ - deltaTime * 0.3f, 0.0f);
    }

    // State machine
    switch (aiState_) {
    case AIState::IDLE:     UpdateIdle(deltaTime); break;
    case AIState::PATROL:   UpdatePatrol(deltaTime); break;
    case AIState::CHASE:    UpdateChase(deltaTime, player); break;
    case AIState::ATTACK:   UpdateAttack(deltaTime, player); break;
    case AIState::TAKE_COVER: UpdateCover(deltaTime); break;
    case AIState::FLANK:    UpdateFlank(deltaTime, player); break;
    case AIState::SEARCH:   UpdateSearch(deltaTime); break;
    default: break;
    }
}

void Enemy::UpdateIdle(float deltaTime) {
    // Look around slowly
    rotation_.y += deltaTime * 0.5f;

    // Transition to patrol after idle time
    if (stateTimer_ > 5.0f) {
        ChangeState(AIState::PATROL);
    }

    // If alert or can see player, switch to chase
    if (alertLevel_ > 0.5f || hasPlayerLOS_) {
        ChangeState(AIState::CHASE);
    }
}

void Enemy::UpdatePatrol(float deltaTime) {
    if (patrolPoints_.empty()) {
        ChangeState(AIState::IDLE);
        return;
    }

    // Move to current patrol point
    Vector3 target = patrolPoints_[currentPatrolIndex_];
    float dist = Math::Distance3D(position_, target);

    if (dist < 1.0f) {
        // Reached patrol point
        if (!waitingAtPatrolPoint_) {
            waitingAtPatrolPoint_ = true;
            stateTimer_ = 0.0f;
        }

        // Wait at patrol point
        if (stateTimer_ >= patrolWaitTime_) {
            currentPatrolIndex_ = (currentPatrolIndex_ + 1) % patrolPoints_.size();
            waitingAtPatrolPoint_ = false;
        }
    } else {
        MoveTowards(target, moveSpeed_ * 0.6f, deltaTime);
        LookAt(target);
    }

    // React to player
    if (hasPlayerLOS_ && dist < detectionRange_) {
        ChangeState(AIState::CHASE);
        AlertNearbyEnemies();
    }
}

void Enemy::UpdateChase(float deltaTime, Player* player) {
    if (!player) return;

    float dist = GetDistanceToPlayer(player);

    // Look at player while chasing
    LookAt(player->GetPosition());

    if (hasPlayerLOS_) {
        // In attack range - switch to attack
        if (dist <= attackRange_) {
            ChangeState(AIState::ATTACK);
        } else {
            // Move towards player
            MoveTowards(player->GetPosition(), moveSpeed_, deltaTime);
        }
    } else {
        // Lost sight - move to last known position
        float distToLast = Math::Distance3D(position_, lastKnownPlayerPos_);
        if (distToLast > 2.0f) {
            MoveTowards(lastKnownPlayerPos_, moveSpeed_, deltaTime);
        } else {
            // Reached last known position, search area
            ChangeState(AIState::SEARCH);
        }
    }

    // If health is low, take cover
    if (health_ < maxHealth_ * 0.3f) {
        ChangeState(AIState::TAKE_COVER);
    }
}

void Enemy::UpdateAttack(float deltaTime, Player* player) {
    if (!player) {
        ChangeState(AIState::SEARCH);
        return;
    }

    float dist = GetDistanceToPlayer(player);
    LookAt(player->GetPosition());

    // Shoot at player
    ShootAtPlayer(player);

    // If player moved out of attack range, chase
    if (dist > attackRange_ * 1.2f) {
        ChangeState(AIState::CHASE);
    }

    // Strafe while attacking (move side to side)
    if (stateTimer_ > 1.0f) {
        float strafeDir = ((int)(stateTimer_ * 0.5f) % 2 == 0) ? 1.0f : -1.0f;
        Vector3 strafe = {
            cosf(rotation_.y + PI * 0.5f) * strafeDir * moveSpeed_ * 0.3f * deltaTime,
            0,
            sinf(rotation_.y + PI * 0.5f) * strafeDir * moveSpeed_ * 0.3f * deltaTime
        };
        position_.x += strafe.x;
        position_.z += strafe.z;
    }

    // Take cover if health low
    if (health_ < maxHealth_ * 0.25f) {
        ChangeState(AIState::TAKE_COVER);
    }
}

void Enemy::UpdateCover(float deltaTime) {
    coverTimer_ += deltaTime;

    if (coverTimer_ < 0.5f) {
        // Move to cover position
        MoveTowards(coverPosition_, moveSpeed_ * 1.2f, deltaTime);
    }

    // Stay in cover for a while to "heal"
    if (coverTimer_ > 3.0f) {
        // Pop out and re-engage
        coverTimer_ = 0.0f;
        ChangeState(AIState::CHASE);
    }
}

void Enemy::UpdateFlank(float deltaTime, Player* player) {
    if (!player) {
        ChangeState(AIState::SEARCH);
        return;
    }

    Vector3 flankPos = FindFlankPosition(player);
    MoveTowards(flankPos, moveSpeed_, deltaTime);

    // If close enough to flank position or can see player from flank, attack
    float dist = Math::Distance3D(position_, flankPos);
    if (dist < 2.0f || (hasPlayerLOS_ && GetDistanceToPlayer(player) <= attackRange_)) {
        ChangeState(AIState::ATTACK);
    }
}

void Enemy::UpdateSearch(float deltaTime) {
    // Look around at last known position
    rotation_.y += deltaTime * 1.5f;

    // Give up after searching
    if (stateTimer_ > 8.0f) {
        alertLevel_ = 0.0f;
        ChangeState(AIState::PATROL);
    }

    // Re-detect player
    if (hasPlayerLOS_) {
        ChangeState(AIState::CHASE);
    }
}

// ============================================================================
// Detection
// ============================================================================

bool Enemy::CanSeePlayer(Player* player) const {
    if (!player || player->IsDead()) return false;

    Vector3 toPlayer = {
        player->GetPosition().x - position_.x,
        player->GetPosition().y - position_.y,
        player->GetPosition().z - position_.z
    };
    float distance = Math::Magnitude3D(toPlayer);

    // Out of detection range
    if (distance > detectionRange_) return false;

    // Check field of view
    Vector3 forward = {sinf(rotation_.y), 0, cosf(rotation_.y)};
    Vector3 dirToPlayer = Math::Normalize3D(toPlayer);
    float dot = forward.x * dirToPlayer.x + forward.z * dirToPlayer.z;

    if (dot < cosf(fieldOfView_ * 0.5f)) return false;

    // TODO: Raycast against map geometry for true LOS check
    // For now, simplified LOS check
    return true;
}

float Enemy::GetDistanceToPlayer(Player* player) const {
    if (!player) return FLT_MAX;
    return Math::Distance3D(position_, player->GetPosition());
}

void Enemy::AlertNearbyEnemies() {
    // In a full implementation, this would notify the Game class
    // to alert other enemies within a certain radius
    // For now, we just mark this enemy as alert
    alertLevel_ = 1.0f;
}

// ============================================================================
// Helpers
// ============================================================================

void Enemy::MoveTowards(const Vector3& target, float speed, float deltaTime) {
    Vector3 dir = {
        target.x - position_.x,
        0,
        target.z - position_.z
    };
    float dist = sqrtf(dir.x * dir.x + dir.z * dir.z);
    if (dist < 0.1f) return;

    dir.x /= dist;
    dir.z /= dist;

    position_.x += dir.x * speed * deltaTime;
    position_.z += dir.z * speed * deltaTime;
}

void Enemy::LookAt(const Vector3& target) {
    Vector3 dir = {
        target.x - position_.x,
        0,
        target.z - position_.z
    };
    rotation_.y = atan2f(dir.x, dir.z);
}

void Enemy::ShootAtPlayer(Player* player) {
    fireTimer_ += 1.0f / 60.0f; // Assuming 60fps update rate

    if (fireTimer_ >= 1.0f / fireRate_) {
        fireTimer_ = 0.0f;

        // Accuracy determines hit chance
        float hitChance = accuracy_;
        float distance = GetDistanceToPlayer(player);

        // Reduce accuracy at long range
        if (distance > attackRange_ * 0.7f) {
            hitChance *= 0.7f;
        }

        // Random hit check
        float roll = (float)(rand() % 100) / 100.0f;
        if (roll < hitChance) {
            player->TakeDamage(damage_);
        }
    }
}

Vector3 Enemy::FindCoverPosition() const {
    // Simple cover finding: move perpendicular to player direction
    Vector3 toPlayer = Math::Normalize3D({
        lastKnownPlayerPos_.x - position_.x,
        0,
        lastKnownPlayerPos_.z - position_.z
    });

    // Try positions perpendicular to player
    Vector3 coverDir = {-toPlayer.z, 0, toPlayer.x};
    return {
        position_.x + coverDir.x * 5.0f,
        position_.y,
        position_.z + coverDir.z * 5.0f
    };
}

Vector3 Enemy::FindFlankPosition(Player* player) const {
    if (!player) return position_;

    Vector3 toPlayer = {
        player->GetPosition().x - position_.x,
        0,
        player->GetPosition().z - position_.z
    };
    float dist = sqrtf(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
    if (dist < 0.1f) return position_;

    toPlayer.x /= dist;
    toPlayer.z /= dist;

    // Flank from the side
    Vector3 flankDir = {-toPlayer.z, 0, toPlayer.x};
    float flankDist = 8.0f;
    if ((int)uniqueId_ % 2 == 0) flankDir = {toPlayer.z, 0, -toPlayer.x};

    return {
        player->GetPosition().x - toPlayer.x * 5.0f + flankDir.x * flankDist,
        player->GetPosition().y,
        player->GetPosition().z - toPlayer.z * 5.0f + flankDir.z * flankDist
    };
}

BoundingBox Enemy::GetBoundingBox() const {
    float height = (enemyClass_ == EnemyClass::HEAVY) ? 2.0f : 1.8f;
    float radius = (enemyClass_ == EnemyClass::HEAVY) ? 0.5f : 0.4f;
    return {
        {position_.x - radius, position_.y, position_.z - radius},
        {position_.x + radius, position_.y + height, position_.z + radius}
    };
}

} // namespace EOSShooter
