// ============================================================================
// EOS Shooter - ParticleSystem.cpp
// ============================================================================

#include "ParticleSystem.h"
#include <cmath>
#include <cstdlib>

namespace EOSShooter {

ParticleSystem::ParticleSystem(int maxParticles)
    : maxParticles_(maxParticles)
{
    particles_.resize(maxParticles);
}

void ParticleSystem::Update(float deltaTime) {
    for (auto& p : particles_) {
        if (!p.active) continue;

        p.lifetime -= deltaTime;
        if (p.lifetime <= 0) {
            p.active = false;
            continue;
        }

        // Apply gravity
        p.velocity.y += p.gravity * deltaTime;

        // Apply drag
        p.velocity.x *= p.drag;
        p.velocity.y *= p.drag;
        p.velocity.z *= p.drag;

        // Update position
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.position.z += p.velocity.z * deltaTime;

        // Floor collision
        if (p.position.y < 0) {
            p.position.y = 0;
            p.velocity.y *= -0.3f;
            p.velocity.x *= 0.7f;
            p.velocity.z *= 0.7f;
        }
    }
}

void ParticleSystem::Render() const {
    for (const auto& p : particles_) {
        if (!p.active) continue;

        float t = 1.0f - (p.lifetime / p.maxLifetime);
        Color color = LerpColor(p.startColor, p.endColor, t);
        float size = p.startSize + (p.endSize - p.startSize) * t;

        if (size > 0.005f) {
            DrawSphere(p.position, size, color);
        }
    }
}

void ParticleSystem::Emit(ParticleEmitterType type, const Vector3& position,
                           const Vector3& direction, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        switch (type) {
        case ParticleEmitterType::MUZZLE_FLASH:  p = GenerateMuzzleFlash(position, direction); break;
        case ParticleEmitterType::BLOOD:          p = GenerateBlood(position, direction); break;
        case ParticleEmitterType::SMOKE:          p = GenerateSmoke(position, direction); break;
        case ParticleEmitterType::EXPLOSION:      p = GenerateExplosion(position, direction); break;
        case ParticleEmitterType::SPARK:          p = GenerateSpark(position, direction); break;
        case ParticleEmitterType::SHELL_EJECT:    p = GenerateShellEject(position, direction); break;
        case ParticleEmitterType::DUST:           p = GenerateDust(position, direction); break;
        case ParticleEmitterType::FIRE:           p = GenerateFire(position, direction); break;
        }
        EmitCustom(p);
    }
}

void ParticleSystem::EmitCustom(const Particle& particle) {
    int idx = FindNextInactive();
    if (idx >= 0) {
        particles_[idx] = particle;
        particles_[idx].active = true;
    }
}

int ParticleSystem::FindNextInactive() {
    // Search from next index for an inactive particle (pool allocation)
    for (int i = 0; i < maxParticles_; i++) {
        int idx = (nextIndex_ + i) % maxParticles_;
        if (!particles_[idx].active) {
            nextIndex_ = (idx + 1) % maxParticles_;
            return idx;
        }
    }
    return -1; // Pool full
}

int ParticleSystem::GetActiveCount() const {
    int count = 0;
    for (const auto& p : particles_) {
        if (p.active) count++;
    }
    return count;
}

void ParticleSystem::Clear() {
    for (auto& p : particles_) {
        p.active = false;
    }
}

// ============================================================================
// Emitter Generators
// ============================================================================

Particle ParticleSystem::GenerateMuzzleFlash(const Vector3& pos, const Vector3& dir) {
    Particle p;
    p.position = pos;
    p.velocity = {
        dir.x * 10.0f + ((float)(rand() % 100) - 50) * 0.1f,
        dir.y * 10.0f + ((float)(rand() % 100) - 50) * 0.1f,
        dir.z * 10.0f + ((float)(rand() % 100) - 50) * 0.1f
    };
    p.startColor = YELLOW;
    p.endColor = ORANGE;
    p.startSize = 0.08f + (float)(rand() % 10) / 100.0f;
    p.endSize = 0.01f;
    p.lifetime = 0.05f + (float)(rand() % 10) / 100.0f;
    p.maxLifetime = p.lifetime;
    p.gravity = 0.0f;
    p.drag = 0.95f;
    return p;
}

Particle ParticleSystem::GenerateBlood(const Vector3& pos, const Vector3& dir) {
    Particle p;
    p.position = pos;
    p.velocity = {
        dir.x * 5.0f + ((float)(rand() % 60) - 30) * 0.1f,
        (float)(rand() % 30) * 0.1f,
        dir.z * 5.0f + ((float)(rand() % 60) - 30) * 0.1f
    };
    p.startColor = RED;
    p.endColor = MAROON;
    p.startSize = 0.04f + (float)(rand() % 5) / 100.0f;
    p.endSize = 0.01f;
    p.lifetime = 0.3f + (float)(rand() % 20) / 10.0f;
    p.maxLifetime = p.lifetime;
    p.gravity = -9.81f;
    p.drag = 0.97f;
    return p;
}

Particle ParticleSystem::GenerateSmoke(const Vector3& pos, const Vector3& dir) {
    Particle p;
    p.position = pos;
    p.velocity = {
        ((float)(rand() % 40) - 20) * 0.1f,
        1.0f + (float)(rand() % 10) / 10.0f,
        ((float)(rand() % 40) - 20) * 0.1f
    };
    p.startColor = {80, 80, 80, 200};
    p.endColor = {40, 40, 40, 0};
    p.startSize = 0.15f;
    p.endSize = 0.5f;
    p.lifetime = 1.0f + (float)(rand() % 20) / 10.0f;
    p.maxLifetime = p.lifetime;
    p.gravity = 0.5f;
    p.drag = 0.99f;
    return p;
}

Particle ParticleSystem::GenerateExplosion(const Vector3& pos, const Vector3& dir) {
    float angle1 = (float)(rand() % 360) * DEG2RAD;
    float angle2 = (float)(rand() % 180 - 90) * DEG2RAD;
    float speed = 5.0f + (float)(rand() % 150) / 10.0f;

    Particle p;
    p.position = pos;
    p.velocity = {
        cosf(angle1) * cosf(angle2) * speed,
        sinf(angle2) * speed + 3.0f,
        sinf(angle1) * cosf(angle2) * speed
    };

    float colorRoll = (float)(rand() % 100) / 100.0f;
    if (colorRoll < 0.3f) {
        p.startColor = YELLOW;
        p.endColor = RED;
    } else if (colorRoll < 0.6f) {
        p.startColor = ORANGE;
        p.endColor = MAROON;
    } else {
        p.startColor = RED;
        p.endColor = DARKGRAY;
    }

    p.startSize = 0.1f + (float)(rand() % 15) / 100.0f;
    p.endSize = 0.02f;
    p.lifetime = 0.3f + (float)(rand() % 10) / 10.0f;
    p.maxLifetime = p.lifetime;
    p.gravity = -5.0f;
    p.drag = 0.96f;
    return p;
}

Particle ParticleSystem::GenerateSpark(const Vector3& pos, const Vector3& dir) {
    float speed = 8.0f + (float)(rand() % 100) / 10.0f;
    Particle p;
    p.position = pos;
    p.velocity = {
        dir.x * speed + ((float)(rand() % 60) - 30) * 0.2f,
        2.0f + (float)(rand() % 30) / 10.0f,
        dir.z * speed + ((float)(rand() % 60) - 30) * 0.2f
    };
    p.startColor = YELLOW;
    p.endColor = WHITE;
    p.startSize = 0.02f;
    p.endSize = 0.005f;
    p.lifetime = 0.2f + (float)(rand() % 5) / 10.0f;
    p.maxLifetime = p.lifetime;
    p.gravity = -15.0f;
    p.drag = 0.99f;
    return p;
}

Particle ParticleSystem::GenerateShellEject(const Vector3& pos, const Vector3& dir) {
    Particle p;
    p.position = pos;
    p.velocity = {
        -dir.x * 3.0f + ((float)(rand() % 20) - 10) * 0.2f,
        3.0f + (float)(rand() % 20) / 10.0f,
        -dir.z * 3.0f + ((float)(rand() % 20) - 10) * 0.2f
    };
    p.startColor = GOLD;
    p.endColor = DARKGRAY;
    p.startSize = 0.03f;
    p.endSize = 0.03f;
    p.lifetime = 1.5f;
    p.maxLifetime = 1.5f;
    p.gravity = -9.81f;
    p.drag = 0.99f;
    return p;
}

Particle ParticleSystem::GenerateDust(const Vector3& pos, const Vector3& dir) {
    Particle p;
    p.position = pos;
    p.velocity = {
        ((float)(rand() % 20) - 10) * 0.1f,
        0.5f,
        ((float)(rand() % 20) - 10) * 0.1f
    };
    p.startColor = {139, 119, 101, 150};
    p.endColor = {139, 119, 101, 0};
    p.startSize = 0.1f;
    p.endSize = 0.4f;
    p.lifetime = 1.0f + (float)(rand() % 10) / 10.0f;
    p.maxLifetime = p.lifetime;
    p.gravity = 0.2f;
    p.drag = 0.99f;
    return p;
}

Particle ParticleSystem::GenerateFire(const Vector3& pos, const Vector3& dir) {
    Particle p;
    p.position = pos;
    p.velocity = {
        ((float)(rand() % 20) - 10) * 0.05f,
        2.0f + (float)(rand() % 10) / 5.0f,
        ((float)(rand() % 20) - 10) * 0.05f
    };
    p.startColor = ORANGE;
    p.endColor = RED;
    p.startSize = 0.12f;
    p.endSize = 0.02f;
    p.lifetime = 0.3f + (float)(rand() % 5) / 10.0f;
    p.maxLifetime = p.lifetime;
    p.gravity = 2.0f;
    p.drag = 0.98f;
    return p;
}

Color ParticleSystem::LerpColor(Color a, Color b, float t) const {
    return {
        (uint8_t)(a.r + (b.r - a.r) * t),
        (uint8_t)(a.g + (b.g - a.g) * t),
        (uint8_t)(a.b + (b.b - a.b) * t),
        (uint8_t)(a.a + (b.a - a.a) * t)
    };
}

} // namespace EOSShooter
