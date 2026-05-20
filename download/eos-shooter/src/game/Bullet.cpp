// ============================================================================
// EOS Shooter - Bullet.cpp
// ============================================================================

#include "Bullet.h"
#include "../utils/Math.h"

namespace EOSShooter {

void Bullet::Update(float deltaTime) {
    if (hasHit_) return;

    lifetime += deltaTime;
    if (IsExpired()) return;

    // Apply gravity drop for long range
    if (type != BulletType::EXPLOSIVE) {
        velocity.y -= 9.81f * deltaTime * 0.05f; // Reduced gravity for gameplay
    }

    // Move bullet
    Vector3 displacement = {
        velocity.x * deltaTime,
        velocity.y * deltaTime,
        velocity.z * deltaTime
    };
    position.x += displacement.x;
    position.y += displacement.y;
    position.z += displacement.z;

    // Track distance
    distance += Math::Magnitude3D(displacement);

    // Update trail
    trailTimer += deltaTime;
    if (trailTimer >= 0.005f) {  // Add trail point every 5ms
        TrailPoint point;
        point.position = position;
        point.alpha = 1.0f;
        trail.push_back(point);
        trailTimer = 0.0f;

        // Limit trail length
        if (trail.size() > 30) {
            trail.erase(trail.begin());
        }
    }

    // Fade trail
    for (auto& tp : trail) {
        tp.alpha -= deltaTime * 3.0f;
    }
    trail.erase(
        std::remove_if(trail.begin(), trail.end(),
            [](const TrailPoint& tp) { return tp.alpha <= 0; }),
        trail.end());
}

void Bullet::Render() const {
    // Draw trail
    if (!trail.empty()) {
        for (size_t i = 1; i < trail.size(); i++) {
            Color trailColor = YELLOW;
            if (type == BulletType::TRACER) trailColor = RED;
            if (type == BulletType::EXPLOSIVE) trailColor = ORANGE;

            uint8_t alpha = (uint8_t)(trail[i].alpha * 200);
            DrawLine3D(trail[i - 1].position, trail[i].position,
                       {trailColor.r, trailColor.g, trailColor.b, alpha});
        }
    }

    // Draw bullet point
    if (!hasHit_) {
        Color bulletColor = YELLOW;
        float size = 0.03f;

        switch (type) {
        case BulletType::TRACER:
            bulletColor = RED;
            size = 0.04f;
            break;
        case BulletType::EXPLOSIVE:
            bulletColor = ORANGE;
            size = 0.06f;
            break;
        case BulletType::ARMOR_PIERCING:
            bulletColor = WHITE;
            size = 0.025f;
            break;
        default:
            break;
        }

        DrawSphere(position, size, bulletColor);
    }
}

bool Bullet::IsExpired() const {
    return hasHit_ || lifetime >= maxLifetime || distance >= range;
}

} // namespace EOSShooter
