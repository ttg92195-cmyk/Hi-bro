#pragma once
// ============================================================================
// EOS Shooter - Math.h
// Custom math utilities for 3D game calculations.
// ============================================================================

#include "raylib.h"
#include <cmath>
#include <cstdlib>

namespace EOSShooter {
namespace Math {

// === Vector Operations ===

inline float Magnitude3D(const Vector3& v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vector3 Normalize3D(const Vector3& v) {
    float mag = Magnitude3D(v);
    if (mag < 0.0001f) return {0, 0, 0};
    return {v.x / mag, v.y / mag, v.z / mag};
}

inline float Distance3D(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

inline float Distance3DSquared(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

inline float Dot3D(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3 Cross3D(const Vector3& a, const Vector3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vector3 Lerp3D(const Vector3& a, const Vector3& b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

// === Weapon Math ===

inline Vector3 ApplySpread(const Vector3& direction, float spreadAngle) {
    if (spreadAngle <= 0.0001f) return direction;

    // Generate random point within cone
    float theta = (float)(rand() % 1000) / 1000.0f * 2.0f * PI;
    float phi = acosf(1.0f - (float)(rand() % 1000) / 1000.0f * (1.0f - cosf(spreadAngle)));

    // Create perpendicular vectors
    Vector3 up = {0, 1, 0};
    if (fabsf(direction.y) > 0.99f) up = {1, 0, 0};

    Vector3 right = Normalize3D(Cross3D(direction, up));
    up = Cross3D(right, direction);

    // Apply spread rotation
    Vector3 spread = {
        direction.x + right.x * sinf(phi) * cosf(theta) + up.x * sinf(phi) * sinf(theta),
        direction.y + right.y * sinf(phi) * cosf(theta) + up.y * sinf(phi) * sinf(theta),
        direction.z + right.z * sinf(phi) * cosf(theta) + up.z * sinf(phi) * sinf(theta)
    };

    return Normalize3D(spread);
}

// === Angle Helpers ===

inline float DegToRad(float degrees) { return degrees * DEG2RAD; }
inline float RadToDeg(float radians) { return radians * RAD2DEG; }

inline float ClampAngle(float angle, float min, float max) {
    if (angle < min) return min;
    if (angle > max) return max;
    return angle;
}

// === Interpolation ===

inline float SmoothDamp(float current, float target, float& currentVelocity,
                          float smoothTime, float deltaTime, float maxSpeed = FLT_MAX) {
    smoothTime = fmaxf(0.0001f, smoothTime);
    float omega = 2.0f / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = current - target;
    float maxChange = maxSpeed * smoothTime;
    change = fmaxf(fminf(change, maxChange), -maxChange);
    float temp = (currentVelocity + omega * change) * deltaTime;
    currentVelocity = (currentVelocity - omega * temp) * exp;
    return target + (change + temp) * exp;
}

// === Random ===

inline float RandomRange(float min, float max) {
    return min + (float)(rand() % 10000) / 10000.0f * (max - min);
}

inline int RandomRangeInt(int min, int max) {
    return min + rand() % (max - min + 1);
}

} // namespace Math
} // namespace EOSShooter
