// ============================================================================
// EOS Shooter - Player.cpp
// Full implementation of the Player entity.
// ============================================================================

#include "Player.h"
#include "../utils/Math.h"
#include <algorithm>
#include <cmath>
#include <memory>

namespace EOSShooter {

// ============================================================================
// Construction
// ============================================================================

Player::Player(uint64_t playerId, const std::string& name, const Vector3& spawnPosition)
    : playerId_(playerId)
    , name_(name)
    , position_(spawnPosition)
    , rotation_({0.0f, 0.0f, 0.0f})
    , velocity_({0.0f, 0.0f, 0.0f})
    , targetPosition_(spawnPosition)
    , targetRotation_({0.0f, 0.0f, 0.0f})
{
    // Initialize default weapon loadout
    weapons_[0] = std::make_unique<Weapon>(WeaponType::ASSAULT_RIFLE);  // Primary
    weapons_[1] = std::make_unique<Weapon>(WeaponType::PISTOL);         // Secondary
    weapons_[2] = std::make_unique<Weapon>(WeaponType::MELEE);          // Melee
}

// ============================================================================
// Lifecycle
// ============================================================================

void Player::Update(float deltaTime) {
    if (isDead_) {
        deathTimer_ += deltaTime;
        return;
    }

    if (isLocal_) {
#if !defined(PLATFORM_ANDROID)
        HandleInput(deltaTime);
#endif
    } else {
        InterpolatePosition(deltaTime);
        InterpolateRotation(deltaTime);
    }

    UpdateMovement(deltaTime);
    UpdateStamina(deltaTime);
    UpdateHealthRegen(deltaTime);
    ApplyGravity(deltaTime);
    UpdateWeapon(deltaTime);

    stats_.playTime += deltaTime;
}

void Player::Render() {
    if (isDead_) return;

    Vector3 pos = position_;
    float height = isCrouching_ ? modelHeight_ * 0.6f : modelHeight_;
    Color bodyColor = isLocal_ ? playerColor_ :
                  (teamId_ == TeamId::TEAM_ALPHA ? BLUE : RED);

    // === Torso (upper body) ===
    float torsoTop = pos.y + height * 0.95f;
    float torsoBot = pos.y + height * 0.45f;
    float torsoRadius = modelRadius_ * 0.95f;
    DrawCapsule(
        {pos.x, torsoBot, pos.z},
        {pos.x, torsoTop, pos.z},
        torsoRadius, 8, 8, bodyColor);

    // === Legs (lower body) ===
    float legTop = pos.y + height * 0.45f;
    float legBot = pos.y + 0.05f;
    float legRadius = modelRadius_ * 0.35f;
    float legSpread = modelRadius_ * 0.45f;

    // Left leg
    float sinR = sinf(rotation_.y + PI * 0.5f);
    float cosR = cosf(rotation_.y + PI * 0.5f);
    DrawCapsule(
        {pos.x + sinR * legSpread, legTop, pos.z + cosR * legSpread},
        {pos.x + sinR * legSpread, legBot, pos.z + cosR * legSpread},
        legRadius, 6, 6, bodyColor);

    // Right leg
    DrawCapsule(
        {pos.x - sinR * legSpread, legTop, pos.z - cosR * legSpread},
        {pos.x - sinR * legSpread, legBot, pos.z - cosR * legSpread},
        legRadius, 6, 6, bodyColor);

    // === Head ===
    float headY = pos.y + height + 0.15f;
    float headRadius = 0.22f;
    DrawSphere({pos.x, headY, pos.z}, headRadius, (Color){200, 200, 210, 255});

    // === Visor (helmet face shield) ===
    float visorFwd = 0.12f;
    float fwdX = sinf(rotation_.y) * visorFwd;
    float fwdZ = cosf(rotation_.y) * visorFwd;
    // Visor - cyan tinted glass
    DrawSphere({pos.x + fwdX, headY - 0.02f, pos.z + fwdZ}, headRadius * 0.7f,
               (Color){0, 180, 220, 180});

    // === Team Color Indicator (shoulder pads) ===
    Color teamColor;
    if (teamId_ == TeamId::TEAM_ALPHA) {
        teamColor = (Color){60, 120, 255, 255};
    } else if (teamId_ == TeamId::TEAM_BRAVO) {
        teamColor = (Color){255, 60, 60, 255};
    } else {
        teamColor = (Color){0, 200, 200, 255};
    }

    // Left shoulder pad
    float shoulderY = pos.y + height * 0.85f;
    DrawSphere({pos.x + sinR * (torsoRadius + 0.08f), shoulderY, pos.z + cosR * (torsoRadius + 0.08f)},
               0.1f, teamColor);
    // Right shoulder pad
    DrawSphere({pos.x - sinR * (torsoRadius + 0.08f), shoulderY, pos.z - cosR * (torsoRadius + 0.08f)},
               0.1f, teamColor);

    // === Name tag (above head) ===
    if (!isLocal_) {
        // Name tag drawn as a 3D text indicator
        // Simple 3D position above head - will need proper screen projection from Game
        // For now, use a floating health bar style indicator
        float barY = headY + headRadius + 0.2f;
        float barWidth = 0.6f;
        float healthPct = health_ / maxHealth_;
        Color hpBarColor = (healthPct > 0.5f) ? (Color){0, 220, 80, 255} :
                          (healthPct > 0.25f) ? (Color){255, 200, 0, 255} :
                          (Color){255, 50, 50, 255};

        // Health bar background
        DrawLine3D({pos.x - barWidth * 0.5f, barY, pos.z},
                   {pos.x + barWidth * 0.5f, barY, pos.z}, Fade(DARKGRAY, 0.8f));
        // Health bar fill
        DrawLine3D({pos.x - barWidth * 0.5f, barY, pos.z},
                   {pos.x - barWidth * 0.5f + barWidth * healthPct, barY, pos.z}, hpBarColor);

        // Team indicator dot above health bar
        DrawSphere({pos.x, barY + 0.12f, pos.z}, 0.04f, teamColor);
    }

    // === Weapon visual ===
    Weapon* weapon = GetCurrentWeapon();
    if (weapon) {
        float wepFwd = 0.6f;
        float wepX = sinf(rotation_.y) * wepFwd;
        float wepZ = cosf(rotation_.y) * wepFwd;
        Vector3 weaponOffset = {
            pos.x + wepX,
            pos.y + height * 0.7f,
            pos.z + wepZ
        };

        Color weaponColor = DARKGRAY;
        // Weapon glow based on type
        switch (weapon->GetType()) {
        case WeaponType::SHOTGUN:       weaponColor = (Color){120, 80, 40, 255}; break;
        case WeaponType::SNIPER_RIFLE:  weaponColor = (Color){50, 70, 90, 255}; break;
        case WeaponType::RPG:           weaponColor = (Color){80, 60, 40, 255}; break;
        default: break;
        }

        DrawCube(weaponOffset, 0.08f, 0.08f, 0.5f, weaponColor);

        // Weapon muzzle glow
        float muzzleFwd = wepFwd + 0.3f;
        Vector3 muzzlePos = {
            pos.x + sinf(rotation_.y) * muzzleFwd,
            pos.y + height * 0.7f,
            pos.z + cosf(rotation_.y) * muzzleFwd
        };
        if (isShooting_) {
            DrawSphere(muzzlePos, 0.06f, Fade((Color){255, 200, 50, 255}, 0.5f));
        }
    }
}

void Player::Respawn(const Vector3& position) {
    position_ = position;
    velocity_ = {0, 0, 0};
    health_ = maxHealth_;
    armor_ = 0.0f;
    stamina_ = maxStamina_;
    isDead_ = false;
    deathTimer_ = 0.0f;
    moveState_ = MovementState::IDLE;
    isShooting_ = false;
    isSprinting_ = false;
    isCrouching_ = false;
    healthRegenTimer_ = 0.0f;

    // Refill ammo for all weapons
    for (auto& weapon : weapons_) {
        if (weapon) weapon->RefillAmmo();
    }

    TraceLog(LOG_INFO, "Player '%s' respawned at (%.1f, %.1f, %.1f)",
        name_.c_str(), position.x, position.y, position.z);
}

void Player::Die() {
    isDead_ = true;
    deathTimer_ = 0.0f;
    isShooting_ = false;
    isSprinting_ = false;
    stats_.deaths++;

    TraceLog(LOG_INFO, "Player '%s' died", name_.c_str());
}

// ============================================================================
// Input Handling (Local Player Only)
// ============================================================================

void Player::HandleInput(float deltaTime) {
    if (isDead_) return;

    // === Mouse Look ===
    Vector2 mouseDelta = GetMouseDelta();
    rotation_.y -= mouseDelta.x * 0.003f;     // Yaw
    rotation_.x -= mouseDelta.y * 0.003f;     // Pitch
    rotation_.x = std::clamp(rotation_.x, -1.5f, 1.5f); // Limit pitch

    // === Movement (WASD) ===
    Vector3 moveDir = {0, 0, 0};
    if (IsKeyDown(KEY_W)) moveDir.z += 1.0f;
    if (IsKeyDown(KEY_S)) moveDir.z -= 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveDir.x += 1.0f;

    // Rotate movement direction by yaw
    if (moveDir.x != 0 || moveDir.z != 0) {
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
        moveDir.x /= len;
        moveDir.z /= len;

        float sinYaw = sinf(rotation_.y);
        float cosYaw = cosf(rotation_.y);
        Vector3 rotatedDir = {
            moveDir.x * cosYaw - moveDir.z * sinYaw,
            0,
            moveDir.x * sinYaw + moveDir.z * cosYaw
        };
        Move(rotatedDir, deltaTime);
    } else {
        // Decelerate when no input
        velocity_.x *= 0.85f;
        velocity_.z *= 0.85f;
        moveState_ = MovementState::IDLE;
    }

    // === Jump ===
    if (IsKeyPressed(KEY_SPACE) && isGrounded_) {
        Jump();
    }

    // === Sprint ===
    if (IsKeyDown(KEY_LEFT_SHIFT) && moveDir.z > 0 && stamina_ > 0) {
        StartSprint();
    } else if (IsKeyReleased(KEY_LEFT_SHIFT) || stamina_ <= 0) {
        StopSprint();
    }

    // === Crouch ===
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        StartCrouch();
    } else {
        StopCrouch();
    }

    // === Shooting ===
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Shoot();
    } else {
        StopShooting();
    }

    // === ADS (Right Click) ===
    isAiming_ = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);

    // === Reload ===
    if (IsKeyPressed(KEY_R)) {
        Reload();
    }

    // === Weapon Switch ===
    if (IsKeyPressed(KEY_ONE)) SwitchWeapon(0);
    if (IsKeyPressed(KEY_TWO)) SwitchWeapon(1);
    if (IsKeyPressed(KEY_THREE)) SwitchWeapon(2);

    // Mouse wheel for weapon switch
    float wheel = GetMouseWheelMove();
    if (wheel > 0) SwitchWeapon((currentWeaponSlot_ + 1) % MAX_WEAPON_SLOTS);
    if (wheel < 0) SwitchWeapon((currentWeaponSlot_ - 1 + MAX_WEAPON_SLOTS) % MAX_WEAPON_SLOTS);
}

// ============================================================================
// Movement
// ============================================================================

void Player::Move(const Vector3& direction, float deltaTime) {
    float speed = moveSpeed_;
    if (isSprinting_) speed = sprintSpeed_;
    if (isCrouching_) speed = crouchSpeed_;

    velocity_.x = direction.x * speed;
    velocity_.z = direction.z * speed;

    position_.x += velocity_.x * deltaTime;
    position_.z += velocity_.z * deltaTime;

    // Update movement state
    if (isSprinting_) {
        moveState_ = MovementState::RUNNING;
    } else if (isCrouching_) {
        moveState_ = MovementState::CROUCHING;
    } else {
        moveState_ = MovementState::WALKING;
    }
}

void Player::Jump() {
    if (!isGrounded_) return;
    velocity_.y = jumpForce_;
    isGrounded_ = false;
    moveState_ = MovementState::JUMPING;
}

void Player::StartSprint() {
    if (isCrouching_ || stamina_ <= 0) return;
    isSprinting_ = true;
}

void Player::StopSprint() {
    isSprinting_ = false;
}

void Player::StartCrouch() {
    isCrouching_ = true;
    isSprinting_ = false;
}

void Player::StopCrouch() {
    isCrouching_ = false;
}

void Player::UpdateMovement(float deltaTime) {
    // Apply friction when grounded and no input
    if (isGrounded_ && moveState_ == MovementState::IDLE) {
        velocity_.x *= 0.9f;
        velocity_.z *= 0.9f;
    }
}

void Player::ApplyGravity(float deltaTime) {
    if (!isGrounded_) {
        velocity_.y += gravity_ * deltaTime;
        position_.y += velocity_.y * deltaTime;
    }

    // Ground collision
    if (position_.y <= groundLevel_) {
        position_.y = groundLevel_;
        velocity_.y = 0;
        isGrounded_ = true;
        if (moveState_ == MovementState::JUMPING || moveState_ == MovementState::FALLING) {
            moveState_ = MovementState::IDLE;
        }
    } else {
        isGrounded_ = false;
        if (velocity_.y < 0 && moveState_ == MovementState::JUMPING) {
            moveState_ = MovementState::FALLING;
        }
    }
}

void Player::UpdateStamina(float deltaTime) {
    if (isSprinting_) {
        stamina_ -= staminaDrainRate_ * deltaTime;
        if (stamina_ <= 0) {
            stamina_ = 0;
            StopSprint();
        }
    } else {
        stamina_ += staminaRegenRate_ * deltaTime;
        if (stamina_ > maxStamina_) stamina_ = maxStamina_;
    }
}

void Player::UpdateHealthRegen(float deltaTime) {
    if (isDead_) return;

    if (healthRegenTimer_ > 0) {
        healthRegenTimer_ -= deltaTime;
    } else if (health_ < maxHealth_) {
        health_ += healthRegenRate_ * deltaTime;
        if (health_ > maxHealth_) health_ = maxHealth_;
    }
}

// ============================================================================
// Combat
// ============================================================================

void Player::Shoot() {
    isShooting_ = true;
    Weapon* weapon = GetCurrentWeapon();
    if (weapon) {
        weapon->StartFiring();
    }
}

void Player::StopShooting() {
    isShooting_ = false;
    Weapon* weapon = GetCurrentWeapon();
    if (weapon) {
        weapon->StopFiring();
    }
}

void Player::Reload() {
    Weapon* weapon = GetCurrentWeapon();
    if (weapon) {
        weapon->StartReload();
    }
}

void Player::SwitchWeapon(int slot) {
    if (slot < 0 || slot >= MAX_WEAPON_SLOTS) return;
    if (slot == currentWeaponSlot_) return;
    if (!weapons_[slot]) return;

    // Stop current weapon actions
    if (weapons_[currentWeaponSlot_]) {
        weapons_[currentWeaponSlot_]->StopFiring();
    }

    currentWeaponSlot_ = slot;
    weapons_[slot]->Equip();
}

void Player::TakeDamage(float damage) {
    if (isDead_) return;

    // Armor absorbs a percentage of damage
    float armorDamage = damage * 0.6f;  // 60% to armor
    float healthDamage = damage * 0.4f;  // 40% to health

    if (armor_ > 0) {
        float armorAbsorbed = std::min(armor_, armorDamage);
        armor_ -= armorAbsorbed;
        float remainingArmorDamage = armorDamage - armorAbsorbed;
        healthDamage += remainingArmorDamage * 0.5f; // Overflow damage reduced
    }

    health_ -= healthDamage;
    healthRegenTimer_ = healthRegenDelay_; // Reset regen timer
    stats_.damageTaken += damage;

    if (health_ <= 0) {
        health_ = 0;
        Die();
    }
}

void Player::Heal(float amount) {
    health_ += amount;
    if (health_ > maxHealth_) health_ = maxHealth_;
}

void Player::AddArmor(float amount) {
    armor_ += amount;
    if (armor_ > maxArmor_) armor_ = maxArmor_;
}

void Player::RefillAmmo(int amount) {
    for (auto& weapon : weapons_) {
        if (weapon) weapon->AddReserveAmmo(amount);
    }
}

void Player::PickupWeapon(WeaponType type) {
    // Replace current weapon slot with new weapon
    weapons_[currentWeaponSlot_] = std::make_unique<Weapon>(type);
    weapons_[currentWeaponSlot_]->Equip();
}

bool Player::IsReloading() const {
    const Weapon* weapon = GetCurrentWeapon();
    return weapon ? weapon->IsReloading() : false;
}

// ============================================================================
// Network Interpolation
// ============================================================================

void Player::InterpolatePosition(float deltaTime) {
    float lerpSpeed = 15.0f * deltaTime;
    position_.x += (targetPosition_.x - position_.x) * lerpSpeed;
    position_.y += (targetPosition_.y - position_.y) * lerpSpeed;
    position_.z += (targetPosition_.z - position_.z) * lerpSpeed;
}

void Player::InterpolateRotation(float deltaTime) {
    float lerpSpeed = 20.0f * deltaTime;
    rotation_.x += (targetRotation_.x - rotation_.x) * lerpSpeed;
    rotation_.y += (targetRotation_.y - rotation_.y) * lerpSpeed;
}

// ============================================================================
// Network Serialization
// ============================================================================

Player::NetData Player::Serialize() const {
    NetData data;
    data.position = position_;
    data.rotation = rotation_;
    data.velocity = velocity_;
    data.moveState = moveState_;
    data.health = health_;
    data.armor = armor_;
    data.currentWeaponSlot = currentWeaponSlot_;
    data.isShooting = isShooting_;
    data.isAiming = isAiming_;
    return data;
}

void Player::Deserialize(const NetData& data) {
    targetPosition_ = data.position;
    targetRotation_ = data.rotation;
    velocity_ = data.velocity;
    moveState_ = data.moveState;
    health_ = data.health;
    armor_ = data.armor;
    currentWeaponSlot_ = data.currentWeaponSlot;
    isShooting_ = data.isShooting;
    isAiming_ = data.isAiming;
}

// ============================================================================
// Weapon Management
// ============================================================================

Weapon* Player::GetCurrentWeapon() {
    if (currentWeaponSlot_ >= 0 && currentWeaponSlot_ < MAX_WEAPON_SLOTS) {
        return weapons_[currentWeaponSlot_].get();
    }
    return nullptr;
}

const Weapon* Player::GetCurrentWeapon() const {
    if (currentWeaponSlot_ >= 0 && currentWeaponSlot_ < MAX_WEAPON_SLOTS) {
        return weapons_[currentWeaponSlot_].get();
    }
    return nullptr;
}

Weapon* Player::GetWeapon(int slot) {
    if (slot >= 0 && slot < MAX_WEAPON_SLOTS) {
        return weapons_[slot].get();
    }
    return nullptr;
}

void Player::UpdateWeapon(float deltaTime) {
    Weapon* weapon = GetCurrentWeapon();
    if (weapon) {
        weapon->Update(deltaTime);

        // Auto-reload when empty
        if (weapon->IsMagazineEmpty() && !weapon->IsReloading() && isShooting_) {
            weapon->StartReload();
        }
    }
}

// ============================================================================
// Bounding Box
// ============================================================================

BoundingBox Player::GetBoundingBox() const {
    float height = isCrouching_ ? modelHeight_ * 0.6f : modelHeight_;
    return {
        {position_.x - modelRadius_, position_.y, position_.z - modelRadius_},
        {position_.x + modelRadius_, position_.y + height, position_.z + modelRadius_}
    };
}

} // namespace EOSShooter
