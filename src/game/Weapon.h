#pragma once
// ============================================================================
// EOS Shooter - Weapon.h
// Comprehensive weapon system with attachments, fire modes, and recoil.
// ============================================================================

#include "raylib.h"
#include <string>
#include <vector>

namespace EOSShooter {

// Weapon classification
enum class WeaponType {
    ASSAULT_RIFLE,
    SMG,
    SHOTGUN,
    SNIPER_RIFLE,
    PISTOL,
    RPG,
    MELEE
};

// Fire mode options
enum class FireMode {
    SEMI,       // Single shot per click
    AUTO,       // Hold to fire
    BURST       // 3-round burst
};

// Weapon operational state
enum class WeaponState {
    READY,
    FIRING,
    RELOADING,
    SWITCHING,
    EMPTY
};

// Attachment types that modify weapon behavior
enum class AttachmentType {
    NONE,
    RED_DOT,       // +10% aim speed
    ACOG,          // +2x zoom, -5% aim speed
    SUPPRESSOR,    // -20% sound, -5% damage
    GRIP,          // -15% recoil
    EXTENDED_MAG,  // +50% magazine size
    LASER,         // -10% hip spread
    FLASH_HIDER    // -30% muzzle flash
};

struct Attachment {
    AttachmentType type = AttachmentType::NONE;
    std::string name;
    float damageModifier = 0.0f;
    float recoilModifier = 0.0f;
    float spreadModifier = 0.0f;
    float magSizeModifier = 0.0f;
    float aimSpeedModifier = 0.0f;
    float rangeModifier = 0.0f;
};

// Recoil pattern for predictable weapon behavior
struct RecoilPattern {
    std::vector<Vector2> kickValues;    // Vertical/horizontal kick per shot
    float recoverySpeed = 5.0f;         // How fast recoil returns
    float firstShotMultiplier = 1.5f;   // Extra kick on first shot
    float currentVerticalRecoil = 0.0f;
    float currentHorizontalRecoil = 0.0f;
};

// Bullet trail type for visual effect
enum class BulletType {
    NORMAL,
    TRACER,
    EXPLOSIVE,
    ARMOR_PIERCING
};

class Weapon {
public:
    explicit Weapon(WeaponType type);
    ~Weapon() = default;

    // === Lifecycle ===
    void Update(float deltaTime);
    void Equip();
    void Unequip();

    // === Combat ===
    void Fire();
    void StartFiring();
    void StopFiring();
    void StartReload();
    void CancelReload();

    // === Attachment System ===
    bool AttachModifier(AttachmentType type, int slot);
    void RemoveAttachment(int slot);
    void RecalculateStats();

    // === Getters ===
    WeaponType GetType() const { return type_; }
    WeaponState GetState() const { return state_; }
    FireMode GetFireMode() const { return fireMode_; }
    bool CanFire() const;
    bool IsFiring() const { return state_ == WeaponState::FIRING; }
    bool IsReloading() const { return state_ == WeaponState::RELOADING; }
    bool IsMagazineEmpty() const { return currentMagazine_ <= 0; }
    bool IsAmmoEmpty() const { return currentMagazine_ <= 0 && reserveAmmo_ <= 0; }

    float GetDamage() const { return effectiveDamage_; }
    float GetRange() const { return effectiveRange_; }
    float GetBulletSpeed() const { return bulletSpeed_; }
    float GetFireRate() const { return fireRate_; }          // Rounds per second
    float GetReloadTime() const { return reloadTime_; }
    float GetCurrentSpread() const { return currentSpread_; }
    float GetBaseSpread() const { return baseSpread_; }
    int GetMagazineSize() const { return effectiveMagSize_; }
    int GetCurrentMagazine() const { return currentMagazine_; }
    int GetReserveAmmo() const { return reserveAmmo_; }
    BulletType GetBulletType() const { return bulletType_; }
    const std::string& GetName() const { return name_; }
    const RecoilPattern& GetRecoilPattern() const { return recoil_; }

    // === Setters ===
    void AddReserveAmmo(int amount);
    void RefillAmmo();
    void SetDamage(float dmg) { baseDamage_ = dmg; RecalculateStats(); }

    // === ADS State ===
    void SetAiming(bool aiming) { isAiming_ = aiming; }
    bool IsAiming() const { return isAiming_; }

private:
    // === Identity ===
    WeaponType type_;
    std::string name_;
    FireMode fireMode_;
    WeaponState state_ = WeaponState::READY;
    BulletType bulletType_ = BulletType::NORMAL;

    // === Base Stats (before attachment modifiers) ===
    float baseDamage_ = 25.0f;
    float baseRange_ = 100.0f;
    float bulletSpeed_ = 500.0f;
    float fireRate_ = 10.0f;            // Rounds per second
    float reloadTime_ = 2.0f;
    float baseSpread_ = 0.02f;          // Radians
    int baseMagSize_ = 30;
    int reserveAmmo_ = 120;

    // === Effective Stats (after attachments) ===
    float effectiveDamage_ = 25.0f;
    float effectiveRange_ = 100.0f;
    int effectiveMagSize_ = 30;

    // === Runtime State ===
    int currentMagazine_ = 30;
    float fireTimer_ = 0.0f;
    float reloadTimer_ = 0.0f;
    float switchTimer_ = 0.0f;
    float equipTime_ = 0.3f;
    float currentSpread_ = 0.02f;
    float spreadRecoverySpeed_ = 3.0f;
    float maxSpread_ = 0.15f;
    float spreadIncrement_ = 0.005f;
    bool isFirstShot_ = true;
    int burstCount_ = 0;
    int burstSize_ = 3;
    bool isAiming_ = false;     // ADS state for spread reduction

    // === Recoil ===
    RecoilPattern recoil_;
    float recoilRecoveryTimer_ = 0.0f;

    // === Attachments (3 slots: optic, barrel, grip) ===
    static constexpr int MAX_ATTACHMENTS = 3;
    Attachment attachments_[3] = {};

    // === Internal ===
    void InitializeStats();
    void UpdateSpread(float deltaTime);
    void UpdateRecoil(float deltaTime);
    void UpdateReload(float deltaTime);
    void UpdateSwitch(float deltaTime);
    void ApplyRecoilKick();
};

} // namespace EOSShooter
