#pragma once
// ============================================================================
// EOS Shooter - Bullet.h
// Bullet entity with physics, trails, and hit detection.
// ============================================================================

#include "raylib.h"
#include "Weapon.h"
#include <vector>
#include <cstdint>

namespace EOSShooter {

struct Bullet {
    // === Transform ===
    Vector3 position  = {0, 0, 0};
    Vector3 direction = {0, 0, 1};
    Vector3 velocity  = {0, 0, 0};

    // === Stats ===
    float damage       = 25.0f;
    float range        = 100.0f;
    float distance     = 0.0f;   // Distance traveled so far
    float penetration  = 0.0f;   // How many surfaces it can pass through

    // === Type ===
    BulletType type = BulletType::NORMAL;
    uint64_t ownerId = 0;        // Who fired this bullet

    // === Lifetime ===
    float lifetime    = 0.0f;
    float maxLifetime = 2.0f;
    bool hasHit_      = false;

    // === Trail ===
    struct TrailPoint {
        Vector3 position;
        float alpha;
    };
    std::vector<TrailPoint> trail;
    float trailTimer = 0.0f;

    // === Methods ===
    void Update(float deltaTime);
    void Render() const;
    bool IsExpired() const;
    bool HasHit() const { return hasHit_; }
    void SetHit(bool hit) { hasHit_ = hit; }
};

} // namespace EOSShooter
