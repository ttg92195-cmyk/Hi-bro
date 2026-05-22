#pragma once
// ============================================================================
// EOS Shooter - CameraController.h
// First-person camera with recoil, head bob, ADS, and shake effects.
// ============================================================================

#include "raylib.h"

namespace EOSShooter {

class Player; // Forward declaration

class CameraController {
public:
    CameraController() = default;
    ~CameraController() = default;

    void Initialize(int screenWidth, int screenHeight);
    void Update(float deltaTime);
    void FollowPlayer(class Player* player, float deltaTime);

    // === Camera Effects ===
    void AddShake(float intensity);
    void SetRecoilOffset(float vertical, float horizontal);
    void ClearRecoil();

    // === ADS (Aim Down Sights) ===
    void SetADS(bool enabled);
    float GetCurrentFOV() const;

    // === Getters ===
    Camera3D GetRaylibCamera() const { return camera_; }
    Vector3 GetCameraPosition() const { return camera_.position; }
    Vector3 GetCameraForward() const;
    bool IsEnabled() const { return enabled_; }

    // === Setters ===
    void SetPosition(const Vector3& pos);
    void SetEnabled(bool e) { enabled_ = e; }
    void SetMouseSensitivity(float s) { sensitivity_ = s; }

private:
    // FIX #8: Initialize camera_ with valid defaults instead of zero-initialization.
    // camera_ = {} sets fovy=0 and projection=0, which causes division by zero
    // in the perspective projection matrix if GetRaylibCamera() is called
    // before Initialize(). Use valid defaults so the camera is always safe.
    Camera3D camera_ = {
        .position = {0, 1.7f, 0},
        .target = {0, 1.7f, -1},
        .up = {0, 1, 0},
        .fovy = 70.0f,
        .projection = CAMERA_PERSPECTIVE
    };
    bool enabled_ = false;
    float sensitivity_ = 0.003f;

    // FOV
    float normalFOV_ = 70.0f;
    float adsFOV_ = 45.0f;
    float currentFOV_ = 70.0f;
    bool isADS_ = false;

    // Recoil
    float recoilVertical_ = 0.0f;
    float recoilHorizontal_ = 0.0f;
    float recoilRecoverySpeed_ = 8.0f;

    // Head bob
    float bobTimer_ = 0.0f;
    float bobAmount_ = 0.0f;
    float bobSpeed_ = 10.0f;
    Vector3 bobOffset_ = {0, 0, 0};

    // Camera shake
    float shakeIntensity_ = 0.0f;
    float shakeDecay_ = 10.0f;
    Vector3 shakeOffset_ = {0, 0, 0};

    // View angles
    float pitch_ = 0.0f;  // Up/Down
    float yaw_ = 0.0f;    // Left/Right

    void UpdateShake(float deltaTime);
    void UpdateBob(float deltaTime, bool isMoving);
    void UpdateRecoil(float deltaTime);
    void UpdateFOV(float deltaTime);
};

} // namespace EOSShooter
