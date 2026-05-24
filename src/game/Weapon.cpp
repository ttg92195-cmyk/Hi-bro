// ============================================================================
// EOS Shooter - Weapon.cpp
// Full implementation of the weapon system.
// ============================================================================

#include "Weapon.h"
#include <cmath>
#include <algorithm>

namespace EOSShooter {

// ============================================================================
// Construction & Initialization
// ============================================================================

Weapon::Weapon(WeaponType type) : type_(type) {
    InitializeStats();
    currentMagazine_ = effectiveMagSize_;
    RecalculateStats();
}

void Weapon::InitializeStats() {
    switch (type_) {
    case WeaponType::ASSAULT_RIFLE:
        name_ = "AR-7 Phantom";
        fireMode_ = FireMode::AUTO;
        baseDamage_ = 28.0f;
        baseRange_ = 120.0f;
        bulletSpeed_ = 600.0f;
        fireRate_ = 11.0f;
        reloadTime_ = 2.2f;
        baseSpread_ = 0.018f;
        baseMagSize_ = 30;
        reserveAmmo_ = 150;
        bulletType_ = BulletType::NORMAL;
        spreadIncrement_ = 0.004f;
        maxSpread_ = 0.10f;
        break;

    case WeaponType::SMG:
        name_ = "Viper-9";
        fireMode_ = FireMode::AUTO;
        baseDamage_ = 20.0f;
        baseRange_ = 60.0f;
        bulletSpeed_ = 450.0f;
        fireRate_ = 15.0f;
        reloadTime_ = 1.6f;
        baseSpread_ = 0.025f;
        baseMagSize_ = 35;
        reserveAmmo_ = 175;
        bulletType_ = BulletType::NORMAL;
        spreadIncrement_ = 0.006f;
        maxSpread_ = 0.14f;
        break;

    case WeaponType::SHOTGUN:
        name_ = "Thunder-12";
        fireMode_ = FireMode::SEMI;
        baseDamage_ = 15.0f;    // Per pellet, 8 pellets
        baseRange_ = 25.0f;
        bulletSpeed_ = 350.0f;
        fireRate_ = 1.5f;
        reloadTime_ = 3.5f;
        baseSpread_ = 0.08f;
        baseMagSize_ = 8;
        reserveAmmo_ = 32;
        bulletType_ = BulletType::NORMAL;
        spreadIncrement_ = 0.02f;
        maxSpread_ = 0.20f;
        break;

    case WeaponType::SNIPER_RIFLE:
        name_ = "Spectre-50";
        fireMode_ = FireMode::SEMI;
        baseDamage_ = 95.0f;
        baseRange_ = 300.0f;
        bulletSpeed_ = 900.0f;
        fireRate_ = 1.0f;
        reloadTime_ = 3.0f;
        baseSpread_ = 0.003f;
        baseMagSize_ = 5;
        reserveAmmo_ = 25;
        bulletType_ = BulletType::ARMOR_PIERCING;
        spreadIncrement_ = 0.001f;
        maxSpread_ = 0.04f;
        break;

    case WeaponType::PISTOL:
        name_ = "Sidearm-19";
        fireMode_ = FireMode::SEMI;
        baseDamage_ = 22.0f;
        baseRange_ = 50.0f;
        bulletSpeed_ = 400.0f;
        fireRate_ = 6.0f;
        reloadTime_ = 1.5f;
        baseSpread_ = 0.015f;
        baseMagSize_ = 12;
        reserveAmmo_ = 60;
        bulletType_ = BulletType::NORMAL;
        spreadIncrement_ = 0.005f;
        maxSpread_ = 0.08f;
        break;

    case WeaponType::RPG:
        name_ = "Devastator";
        fireMode_ = FireMode::SEMI;
        baseDamage_ = 150.0f;
        baseRange_ = 200.0f;
        bulletSpeed_ = 200.0f;
        fireRate_ = 0.5f;
        reloadTime_ = 4.0f;
        baseSpread_ = 0.01f;
        baseMagSize_ = 1;
        reserveAmmo_ = 3;
        bulletType_ = BulletType::EXPLOSIVE;
        spreadIncrement_ = 0.001f;
        maxSpread_ = 0.03f;
        break;

    case WeaponType::MELEE:
        name_ = "Combat Knife";
        fireMode_ = FireMode::SEMI;
        baseDamage_ = 55.0f;
        baseRange_ = 2.5f;
        bulletSpeed_ = 0.0f;
        fireRate_ = 2.0f;
        reloadTime_ = 0.0f;
        baseSpread_ = 0.0f;
        baseMagSize_ = 1;
        reserveAmmo_ = 1;
        bulletType_ = BulletType::NORMAL;
        spreadIncrement_ = 0.0f;
        maxSpread_ = 0.0f;
        break;
    }

    // Initialize recoil pattern based on weapon type
    recoil_.recoverySpeed = 5.0f;
    recoil_.firstShotMultiplier = 1.5f;
    recoil_.currentVerticalRecoil = 0.0f;
    recoil_.currentHorizontalRecoil = 0.0f;

    // Generate recoil pattern (8-10 points that cycle)
    int patternLength = (type_ == WeaponType::SHOTGUN) ? 2 : 8;
    float vertKick = 0.0f;
    float horizKick = 0.0f;
    for (int i = 0; i < patternLength; i++) {
        switch (type_) {
        case WeaponType::ASSAULT_RIFLE:
            vertKick = 0.3f + (float)(rand() % 10) / 100.0f;
            horizKick = ((float)(rand() % 20) - 10.0f) / 100.0f;
            break;
        case WeaponType::SMG:
            vertKick = 0.2f + (float)(rand() % 8) / 100.0f;
            horizKick = ((float)(rand() % 25) - 12.5f) / 100.0f;
            break;
        case WeaponType::SHOTGUN:
            vertKick = 1.0f;
            horizKick = 0.0f;
            break;
        case WeaponType::SNIPER_RIFLE:
            vertKick = 1.5f;
            horizKick = 0.0f;
            break;
        default:
            vertKick = 0.2f;
            horizKick = 0.0f;
            break;
        }
        recoil_.kickValues.push_back({vertKick, horizKick});
    }
}

// ============================================================================
// Update
// ============================================================================

void Weapon::Update(float deltaTime) {
    switch (state_) {
    case WeaponState::READY:
        UpdateSpread(deltaTime);
        UpdateRecoil(deltaTime);
        break;
    case WeaponState::FIRING:
        UpdateSpread(deltaTime);
        UpdateRecoil(deltaTime);
        fireTimer_ -= deltaTime;
        if (fireTimer_ <= 0 && state_ == WeaponState::FIRING) {
            state_ = WeaponState::READY;
        }
        break;
    case WeaponState::RELOADING:
        UpdateReload(deltaTime);
        break;
    case WeaponState::SWITCHING:
        UpdateSwitch(deltaTime);
        break;
    case WeaponState::EMPTY:
        break;
    }
}

void Weapon::UpdateSpread(float deltaTime) {
    // ADS recovers spread faster and reduces minimum spread
    float effectiveBaseSpread = baseSpread_;
    float effectiveRecoverySpeed = spreadRecoverySpeed_;

    if (isAiming_) {
        effectiveBaseSpread *= 0.35f;    // ADS minimum spread is 35% of hip-fire
        effectiveRecoverySpeed *= 2.0f;  // ADS recovers spread twice as fast
    }

    // Recover spread over time
    if (currentSpread_ > effectiveBaseSpread) {
        currentSpread_ -= effectiveRecoverySpeed * deltaTime;
        if (currentSpread_ < effectiveBaseSpread) currentSpread_ = effectiveBaseSpread;
    }
}

void Weapon::UpdateRecoil(float deltaTime) {
    if (recoil_.currentVerticalRecoil > 0) {
        recoil_.currentVerticalRecoil -= recoil_.recoverySpeed * deltaTime;
        if (recoil_.currentVerticalRecoil < 0) recoil_.currentVerticalRecoil = 0;
    }
    if (recoil_.currentHorizontalRecoil != 0) {
        recoil_.currentHorizontalRecoil *= (1.0f - recoil_.recoverySpeed * deltaTime);
        if (fabsf(recoil_.currentHorizontalRecoil) < 0.001f) recoil_.currentHorizontalRecoil = 0;
    }
}

void Weapon::UpdateReload(float deltaTime) {
    reloadTimer_ += deltaTime;
    if (reloadTimer_ >= reloadTime_) {
        // Complete reload
        int needed = effectiveMagSize_ - currentMagazine_;
        int available = std::min(needed, reserveAmmo_);
        currentMagazine_ += available;
        reserveAmmo_ -= available;
        reloadTimer_ = 0.0f;
        state_ = WeaponState::READY;
    }
}

void Weapon::UpdateSwitch(float deltaTime) {
    switchTimer_ += deltaTime;
    if (switchTimer_ >= equipTime_) {
        switchTimer_ = 0.0f;
        state_ = WeaponState::READY;
    }
}

// ============================================================================
// Combat
// ============================================================================

bool Weapon::CanFire() const {
    if (state_ != WeaponState::READY) return false;
    if (currentMagazine_ <= 0) return false;
    return true;
}

void Weapon::Fire() {
    if (!CanFire()) return;

    currentMagazine_--;
    fireTimer_ = 1.0f / fireRate_;
    state_ = WeaponState::FIRING;

    // Apply spread increase
    float spreadMult = isFirstShot_ ? recoil_.firstShotMultiplier : 1.0f;
    currentSpread_ += spreadIncrement_ * spreadMult;
    if (currentSpread_ > maxSpread_) currentSpread_ = maxSpread_;
    isFirstShot_ = false;

    // Apply recoil
    ApplyRecoilKick();

    // Check if empty after firing
    if (currentMagazine_ <= 0 && reserveAmmo_ <= 0) {
        state_ = WeaponState::EMPTY;
    }
}

void Weapon::StartFiring() {
    if (type_ == WeaponType::MELEE) {
        if (CanFire()) Fire();
        return;
    }

    switch (fireMode_) {
    case FireMode::AUTO:
        if (CanFire()) Fire();
        break;
    case FireMode::SEMI:
        if (CanFire() && isFirstShot_) Fire();
        break;
    case FireMode::BURST:
        if (CanFire() && burstCount_ == 0) {
            burstCount_ = burstSize_;
            Fire();
            burstCount_--;
        }
        break;
    }
}

void Weapon::StopFiring() {
    isFirstShot_ = true;
    burstCount_ = 0;
}

void Weapon::StartReload() {
    if (state_ == WeaponState::RELOADING) return;
    if (currentMagazine_ >= effectiveMagSize_) return;
    if (reserveAmmo_ <= 0) return;
    if (type_ == WeaponType::MELEE) return;

    state_ = WeaponState::RELOADING;
    reloadTimer_ = 0.0f;
}

void Weapon::CancelReload() {
    if (state_ == WeaponState::RELOADING) {
        state_ = WeaponState::READY;
        reloadTimer_ = 0.0f;
    }
}

void Weapon::Equip() {
    state_ = WeaponState::SWITCHING;
    switchTimer_ = 0.0f;
    isFirstShot_ = true;
}

void Weapon::Unequip() {
    CancelReload();
    StopFiring();
    state_ = WeaponState::READY;
}

void Weapon::ApplyRecoilKick() {
    if (recoil_.kickValues.empty()) return;

    int idx = 0; // Simplified: always use first pattern point
    // In production, track shot count and cycle through pattern
    auto& kick = recoil_.kickValues[idx];
    float mult = isFirstShot_ ? recoil_.firstShotMultiplier : 1.0f;
    recoil_.currentVerticalRecoil += kick.x * mult;
    recoil_.currentHorizontalRecoil += kick.y * mult;
}

// ============================================================================
// Attachments
// ============================================================================

bool Weapon::AttachModifier(AttachmentType type, int slot) {
    if (slot < 0 || slot >= MAX_ATTACHMENTS) return false;

    // Validate slot type mapping
    // Slot 0 = optic, Slot 1 = barrel, Slot 2 = grip/magazine
    switch (slot) {
    case 0: // Optic
        if (type != AttachmentType::RED_DOT && type != AttachmentType::ACOG) return false;
        break;
    case 1: // Barrel
        if (type != AttachmentType::SUPPRESSOR && type != AttachmentType::FLASH_HIDER) return false;
        break;
    case 2: // Grip/Magazine
        if (type != AttachmentType::GRIP && type != AttachmentType::EXTENDED_MAG && type != AttachmentType::LASER) return false;
        break;
    }

    // Create attachment with modifiers
    Attachment att;
    att.type = type;

    switch (type) {
    case AttachmentType::RED_DOT:
        att.name = "Red Dot Sight";
        att.aimSpeedModifier = 0.1f;
        att.spreadModifier = -0.003f;
        break;
    case AttachmentType::ACOG:
        att.name = "ACOG Scope";
        att.aimSpeedModifier = -0.05f;
        att.rangeModifier = 50.0f;
        att.spreadModifier = -0.005f;
        break;
    case AttachmentType::SUPPRESSOR:
        att.name = "Suppressor";
        att.damageModifier = -0.05f;
        att.spreadModifier = -0.002f;
        att.rangeModifier = -10.0f;
        break;
    case AttachmentType::GRIP:
        att.name = "Foregrip";
        att.recoilModifier = -0.15f;
        att.spreadModifier = -0.004f;
        break;
    case AttachmentType::EXTENDED_MAG:
        att.name = "Extended Magazine";
        att.magSizeModifier = 0.5f;
        break;
    case AttachmentType::LASER:
        att.name = "Laser Sight";
        att.spreadModifier = -0.005f;
        break;
    case AttachmentType::FLASH_HIDER:
        att.name = "Flash Hider";
        att.recoilModifier = -0.05f;
        break;
    default:
        break;
    }

    attachments_[slot] = att;
    RecalculateStats();
    return true;
}

void Weapon::RemoveAttachment(int slot) {
    if (slot < 0 || slot >= MAX_ATTACHMENTS) return;
    attachments_[slot] = Attachment{};
    RecalculateStats();
}

void Weapon::RecalculateStats() {
    // Start from base
    effectiveDamage_ = baseDamage_;
    effectiveRange_ = baseRange_;
    effectiveMagSize_ = baseMagSize_;
    float recoilMult = 1.0f;
    float spreadMod = 0.0f;

    for (int i = 0; i < MAX_ATTACHMENTS; i++) {
        const auto& att = attachments_[i];
        if (att.type == AttachmentType::NONE) continue;

        effectiveDamage_ *= (1.0f + att.damageModifier);
        effectiveRange_ += att.rangeModifier;
        effectiveMagSize_ = (int)(baseMagSize_ * (1.0f + att.magSizeModifier));
        recoilMult += att.recoilModifier;
        spreadMod += att.spreadModifier;
    }

    // Clamp values
    if (effectiveDamage_ < 1.0f) effectiveDamage_ = 1.0f;
    if (effectiveRange_ < 10.0f) effectiveRange_ = 10.0f;
    if (effectiveMagSize_ < 1) effectiveMagSize_ = 1;
    if (recoilMult < 0.3f) recoilMult = 0.3f;

    // Update recoil recovery with modifier
    recoil_.recoverySpeed = 5.0f / recoilMult;

    // Update base spread with modifier
    baseSpread_ = 0.018f + spreadMod; // Starting value + mods
    if (baseSpread_ < 0.001f) baseSpread_ = 0.001f;
}

// ============================================================================
// Ammo Management
// ============================================================================

void Weapon::AddReserveAmmo(int amount) {
    reserveAmmo_ += amount;
    // Cap reserve ammo
    int maxReserve = baseMagSize_ * 5;
    if (reserveAmmo_ > maxReserve) reserveAmmo_ = maxReserve;
}

void Weapon::RefillAmmo() {
    currentMagazine_ = effectiveMagSize_;
    reserveAmmo_ = baseMagSize_ * 5;
    if (state_ == WeaponState::EMPTY) state_ = WeaponState::READY;
}

} // namespace EOSShooter
