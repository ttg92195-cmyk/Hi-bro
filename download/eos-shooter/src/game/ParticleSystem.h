#pragma once
// ============================================================================
// EOS Shooter - ParticleSystem.h
// GPU-friendly particle system with pool allocation and batch rendering.
// ============================================================================

#include "raylib.h"
#include <vector>

namespace EOSShooter {

enum class ParticleEmitterType {
    MUZZLE_FLASH,
    BLOOD,
    SMOKE,
    EXPLOSION,
    SPARK,
    SHELL_EJECT,
    DUST,
    FIRE
};

struct Particle {
    Vector3 position  = {0, 0, 0};
    Vector3 velocity  = {0, 0, 0};
    Color   startColor = WHITE;
    Color   endColor   = GRAY;
    float   startSize  = 0.1f;
    float   endSize    = 0.01f;
    float   lifetime   = 1.0f;
    float   maxLifetime = 1.0f;
    float   gravity    = -9.81f;
    float   drag       = 0.98f;
    bool    active     = false;
};

class ParticleSystem {
public:
    explicit ParticleSystem(int maxParticles = 5000);
    ~ParticleSystem() = default;

    void Update(float deltaTime);
    void Render() const;

    // Emit particles with predefined emitter settings
    void Emit(ParticleEmitterType type, const Vector3& position,
              const Vector3& direction, int count);

    // Emit custom particle
    void EmitCustom(const Particle& particle);

    // Getters
    int GetActiveCount() const;
    int GetMaxParticles() const { return maxParticles_; }
    void Clear();

private:
    int maxParticles_;
    std::vector<Particle> particles_;
    int nextIndex_ = 0;

    // Emitter configuration generators
    Particle GenerateMuzzleFlash(const Vector3& pos, const Vector3& dir);
    Particle GenerateBlood(const Vector3& pos, const Vector3& dir);
    Particle GenerateSmoke(const Vector3& pos, const Vector3& dir);
    Particle GenerateExplosion(const Vector3& pos, const Vector3& dir);
    Particle GenerateSpark(const Vector3& pos, const Vector3& dir);
    Particle GenerateShellEject(const Vector3& pos, const Vector3& dir);
    Particle GenerateDust(const Vector3& pos, const Vector3& dir);
    Particle GenerateFire(const Vector3& pos, const Vector3& dir);

    int FindNextInactive();
    Color LerpColor(Color a, Color b, float t) const;
};

} // namespace EOSShooter
