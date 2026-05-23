// ============================================================================
// EOS Shooter - Enemy.cpp
// Full AI implementation with state machine, detection, and squad behavior.
// ============================================================================

#include "Enemy.h"
#include "Player.h"
#include "Map.h"
#include "Game.h"
#include "ModelManager.h"
#include "../utils/Math.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace EOSShooter {

// Static model manager pointer
ModelManager* Enemy::s_modelManager = nullptr;

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

    // === Colors per enemy class with sci-fi theme ===
    Color bodyColor;
    Color accentColor;
    Color eyeColor = {255, 0, 0, 255}; // Glowing red eyes
    float height = 1.8f;
    float bodyWidth = 0.4f;

    switch (enemyClass_) {
    case EnemyClass::GRUNT:
        bodyColor = (Color){40, 100, 40, 255};        // Military green
        accentColor = (Color){80, 200, 80, 255};      // Bright green accent
        break;
    case EnemyClass::HEAVY:
        bodyColor = (Color){80, 80, 90, 255};         // Dark steel
        accentColor = (Color){255, 160, 0, 255};      // Orange accent
        height = 2.0f;
        bodyWidth = 0.55f;
        break;
    case EnemyClass::SNIPER:
        bodyColor = (Color){100, 30, 40, 255};        // Dark maroon
        accentColor = (Color){255, 80, 120, 255};     // Pink accent
        height = 1.9f;
        bodyWidth = 0.35f;
        break;
    case EnemyClass::SHOTGUNNER:
        bodyColor = (Color){120, 70, 30, 255};        // Brown/tan
        accentColor = (Color){255, 200, 0, 255};      // Yellow accent
        break;
    case EnemyClass::ROCKETEER:
        bodyColor = (Color){140, 60, 20, 255};        // Dark orange
        accentColor = (Color){255, 60, 60, 255};      // Red accent
        height = 1.85f;
        bodyWidth = 0.45f;
        break;
    default:
        bodyColor = DARKGREEN;
        accentColor = GREEN;
        break;
    }

    // === Torso ===
    float torsoTop = position_.y + height * 0.95f;
    float torsoBot = position_.y + height * 0.45f;
    DrawCapsule(
        {position_.x, torsoBot, position_.z},
        {position_.x, torsoTop, position_.z},
        bodyWidth, 8, 8, bodyColor);

    // === Accent stripe on torso (chest plate) ===
    float chestY = position_.y + height * 0.75f;
    DrawSphere({position_.x, chestY, position_.z}, bodyWidth * 0.5f, Fade(accentColor, 0.3f));

    // === Legs ===
    float legTop = position_.y + height * 0.45f;
    float legBot = position_.y + 0.05f;
    float legRadius = bodyWidth * 0.35f;
    float legSpread = bodyWidth * 0.5f;

    float sinR = sinf(rotation_.y + PI * 0.5f);
    float cosR = cosf(rotation_.y + PI * 0.5f);

    // Left leg
    DrawCapsule(
        {position_.x + sinR * legSpread, legTop, position_.z + cosR * legSpread},
        {position_.x + sinR * legSpread, legBot, position_.z + cosR * legSpread},
        legRadius, 6, 6, bodyColor);
    // Right leg
    DrawCapsule(
        {position_.x - sinR * legSpread, legTop, position_.z - cosR * legSpread},
        {position_.x - sinR * legSpread, legBot, position_.z - cosR * legSpread},
        legRadius, 6, 6, bodyColor);

    // === Head ===
    float headY = position_.y + height + 0.15f;
    float headRadius = (enemyClass_ == EnemyClass::HEAVY) ? 0.28f : 0.22f;
    DrawSphere({position_.x, headY, position_.z}, headRadius, (Color){60, 60, 70, 255});

    // === Glowing Red Eyes ===
    float eyeFwd = headRadius * 0.7f;
    float fwdX = sinf(rotation_.y) * eyeFwd;
    float fwdZ = cosf(rotation_.y) * eyeFwd;
    float eyeSpread = headRadius * 0.35f;
    float eyeY = headY + 0.02f;

    // Pulsing glow
    float eyePulse = sinf(GetTime() * 5.0f + uniqueId_) * 0.2f + 0.8f;

    // Left eye
    Vector3 leftEyePos = {
        position_.x + fwdX + sinR * eyeSpread,
        eyeY,
        position_.z + fwdZ + cosR * eyeSpread
    };
    DrawSphere(leftEyePos, 0.04f, eyeColor);
    DrawSphere(leftEyePos, 0.07f, Fade(eyeColor, 0.2f * eyePulse));

    // Right eye
    Vector3 rightEyePos = {
        position_.x + fwdX - sinR * eyeSpread,
        eyeY,
        position_.z + fwdZ - cosR * eyeSpread
    };
    DrawSphere(rightEyePos, 0.04f, eyeColor);
    DrawSphere(rightEyePos, 0.07f, Fade(eyeColor, 0.2f * eyePulse));

    // === Health Bar (proper background + fill + border) ===
    float healthPercent = health_ / maxHealth_;
    float barY = headY + headRadius + 0.3f;
    float barWidth = 0.8f;
    float barHeight = 0.06f;

    // Background bar (full width, dark)
    Vector3 barBgStart = {position_.x - barWidth * 0.5f, barY - barHeight, position_.z};
    Vector3 barBgEnd = {position_.x + barWidth * 0.5f, barY - barHeight, position_.z};
    DrawLine3D(barBgStart, barBgEnd, Fade(DARKGRAY, 0.9f));

    // Fill bar
    Color hpColor;
    if (healthPercent > 0.5f) {
        hpColor = (Color){0, 220, 80, 255};
    } else if (healthPercent > 0.25f) {
        hpColor = (Color){255, 200, 0, 255};
    } else {
        float hpPulse = sinf(GetTime() * 6.0f) * 0.3f + 0.7f;
        hpColor = (Color){255, (uint8_t)(50 * hpPulse), 0, 255};
    }
    Vector3 barFillEnd = {
        position_.x - barWidth * 0.5f + barWidth * healthPercent,
        barY - barHeight,
        position_.z
    };
    DrawLine3D(barBgStart, barFillEnd, hpColor);

    // Border lines (top and bottom of health bar)
    DrawLine3D({barBgStart.x, barY, barBgStart.z}, {barBgEnd.x, barY, barBgEnd.z}, Fade((Color){100, 100, 100, 255}, 0.5f));
    DrawLine3D({barBgStart.x, barY - barHeight * 2, barBgStart.z}, {barBgEnd.x, barY - barHeight * 2, barBgEnd.z}, Fade((Color){100, 100, 100, 255}, 0.5f));

    // === Weapon Visual (3D model or fallback primitive) ===
    float wepFwd = 0.5f;
    Vector3 weaponOffset = {
        position_.x + sinf(rotation_.y) * wepFwd,
        position_.y + height * 0.7f,
        position_.z + cosf(rotation_.y) * wepFwd
    };

    // Default weapon color
    Color weaponColor = (Color){50, 50, 55, 255};

    // Try 3D model rendering first
    WeaponType enemyWeaponType = GetWeaponTypeForClass();
    bool modelRendered = false;

    if (s_modelManager && s_modelManager->HasWeaponModel(enemyWeaponType)) {
        Model* model = s_modelManager->GetWeaponModel(enemyWeaponType);
        if (model) {
            const WeaponModelConfig& cfg = s_modelManager->GetWeaponConfig(enemyWeaponType);

            // Calculate weapon position with offset rotated by enemy yaw
            Vector3 finalPos = {
                weaponOffset.x + cfg.positionOffset.x * cosf(rotation_.y) - cfg.positionOffset.z * sinf(rotation_.y),
                weaponOffset.y + cfg.positionOffset.y,
                weaponOffset.z + cfg.positionOffset.x * sinf(rotation_.y) + cfg.positionOffset.z * cosf(rotation_.y)
            };

            float totalAngle = cfg.rotationAngle + rotation_.y * RAD2DEG;

            // Scale adjustments for specific enemy classes
            Vector3 scale = cfg.scale;
            if (enemyClass_ == EnemyClass::HEAVY) {
                scale.x *= 1.3f;
                scale.y *= 1.3f;
                scale.z *= 1.3f;
            }

            DrawModelEx(*model, finalPos, cfg.rotationAxis, totalAngle, scale, weaponColor);
            modelRendered = true;
        }
    }

    // Fallback: primitive rendering
    if (!modelRendered) {
        float weaponLen = 0.4f;
        float weaponW = 0.06f;

        switch (enemyClass_) {
        case EnemyClass::HEAVY:
            weaponLen = 0.6f;
            weaponW = 0.1f;
            weaponColor = (Color){70, 70, 80, 255};
            break;
        case EnemyClass::SNIPER:
            weaponLen = 0.7f;
            weaponW = 0.04f;
            weaponColor = (Color){40, 50, 60, 255};
            break;
        case EnemyClass::SHOTGUNNER:
            weaponLen = 0.45f;
            weaponW = 0.1f;
            weaponColor = (Color){90, 60, 30, 255};
            break;
        case EnemyClass::ROCKETEER:
            weaponLen = 0.6f;
            weaponW = 0.12f;
            weaponColor = (Color){80, 40, 30, 255};
            break;
        default:
            break;
        }

        DrawCube(weaponOffset, weaponW, weaponW, weaponLen, weaponColor);
    }

    // Weapon glow for special enemies
    if (enemyClass_ == EnemyClass::ROCKETEER) {
        DrawSphere(weaponOffset, 0.12f * 2.0f, Fade((Color){255, 60, 0, 255}, 0.08f));
    } else if (enemyClass_ == EnemyClass::SNIPER) {
        // Laser sight dot
        float laserLen = 2.0f;
        Vector3 laserEnd = {
            position_.x + sinf(rotation_.y) * (wepFwd + laserLen),
            position_.y + height * 0.7f,
            position_.z + cosf(rotation_.y) * (wepFwd + laserLen)
        };
        DrawLine3D(weaponOffset, laserEnd, Fade((Color){255, 0, 0, 255}, 0.15f));
    } else if (enemyClass_ == EnemyClass::HEAVY) {
        // Muzzle glow hint
        DrawSphere(weaponOffset, 0.12f * 1.5f, Fade(accentColor, 0.06f));
    }

    // === Alert indicator when in CHASE/ATTACK state ===
    if (aiState_ == AIState::CHASE || aiState_ == AIState::ATTACK) {
        float alertY = barY + 0.15f;
        float alertPulse = sinf(GetTime() * 8.0f) * 0.3f + 0.7f;
        DrawSphere({position_.x, alertY, position_.z}, 0.03f, Fade((Color){255, 50, 50, 255}, alertPulse));
    }
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

WeaponType Enemy::GetWeaponTypeForClass() const {
    switch (enemyClass_) {
    case EnemyClass::GRUNT:      return WeaponType::ASSAULT_RIFLE;
    case EnemyClass::HEAVY:      return WeaponType::ASSAULT_RIFLE;  // Uses AR model, bigger scale
    case EnemyClass::SNIPER:     return WeaponType::SNIPER_RIFLE;
    case EnemyClass::SHOTGUNNER: return WeaponType::SHOTGUN;
    case EnemyClass::ROCKETEER:  return WeaponType::RPG;
    default:                     return WeaponType::ASSAULT_RIFLE;
    }
}

} // namespace EOSShooter
