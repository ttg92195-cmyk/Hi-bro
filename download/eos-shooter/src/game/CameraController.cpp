// ============================================================================
// EOS Shooter - CameraController.cpp
// ============================================================================

#include "CameraController.h"
#include "Player.h"
#include <cmath>
#include <cstdlib>

namespace EOSShooter {

void CameraController::Initialize(int screenWidth, int screenHeight) {
    camera_.position = {0, 1.7f, 0};
    camera_.target = {0, 1.7f, -1};
    camera_.up = {0, 1, 0};
    camera_.fovy = normalFOV_;
    camera_.projection = CAMERA_PERSPECTIVE;

    pitch_ = 0.0f;
    yaw_ = 0.0f;
}

void CameraController::Update(float deltaTime) {
    UpdateShake(deltaTime);
    UpdateRecoil(deltaTime);
    UpdateFOV(deltaTime);
}

void CameraController::FollowPlayer(Player* player, float deltaTime) {
    if (!player || !enabled_) return;

    Vector3 playerPos = player->GetPosition();
    float eyeHeight = player->IsCrouching() ? 1.0f : 1.7f;

    // Update view angles from player rotation
    yaw_ = player->GetRotation().y;
    pitch_ = player->GetRotation().x;

    // Update head bob
    bool isMoving = (player->GetMovementState() != EOSShooter::MovementState::IDLE);
    UpdateBob(deltaTime, isMoving);

    // Apply recoil to pitch/yaw
    pitch_ += recoilVertical_;
    yaw_ += recoilHorizontal_;

    // Calculate camera position
    Vector3 eyePos = {
        playerPos.x + bobOffset_.x + shakeOffset_.x,
        playerPos.y + eyeHeight + bobOffset_.y + shakeOffset_.y,
        playerPos.z + bobOffset_.z + shakeOffset_.z
    };

    // Calculate look target
    Vector3 forward = {
        sinf(yaw_) * cosf(pitch_),
        sinf(pitch_),
        cosf(yaw_) * cosf(pitch_)
    };

    Vector3 target = {
        eyePos.x + forward.x,
        eyePos.y + forward.y,
        eyePos.z + forward.z
    };

    camera_.position = eyePos;
    camera_.target = target;
    camera_.fovy = currentFOV_;
}

Vector3 CameraController::GetCameraForward() const {
    Vector3 diff = {
        camera_.target.x - camera_.position.x,
        camera_.target.y - camera_.position.y,
        camera_.target.z - camera_.position.z
    };
    float len = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    if (len < 0.0001f) return {0, 0, 1};
    return {diff.x / len, diff.y / len, diff.z / len};
}

void CameraController::SetPosition(const Vector3& pos) {
    camera_.position = pos;
    camera_.target = {pos.x, pos.y, pos.z - 1.0f};
}

void CameraController::AddShake(float intensity) {
    shakeIntensity_ += intensity;
    if (shakeIntensity_ > 1.0f) shakeIntensity_ = 1.0f;
}

void CameraController::SetRecoilOffset(float vertical, float horizontal) {
    recoilVertical_ += vertical;
    recoilHorizontal_ += horizontal;
}

void CameraController::ClearRecoil() {
    recoilVertical_ = 0.0f;
    recoilHorizontal_ = 0.0f;
}

void CameraController::SetADS(bool enabled) {
    isADS_ = enabled;
}

float CameraController::GetCurrentFOV() const {
    return currentFOV_;
}

// ============================================================================
// Internal Updates
// ============================================================================

void CameraController::UpdateShake(float deltaTime) {
    if (shakeIntensity_ > 0.01f) {
        shakeOffset_ = {
            ((float)(rand() % 200) - 100) * 0.01f * shakeIntensity_,
            ((float)(rand() % 200) - 100) * 0.01f * shakeIntensity_,
            ((float)(rand() % 200) - 100) * 0.01f * shakeIntensity_
        };
        shakeIntensity_ -= shakeDecay_ * deltaTime;
        if (shakeIntensity_ < 0) shakeIntensity_ = 0;
    } else {
        shakeOffset_ = {0, 0, 0};
    }
}

void CameraController::UpdateBob(float deltaTime, bool isMoving) {
    if (isMoving) {
        bobTimer_ += deltaTime * bobSpeed_;
        bobAmount_ = 0.04f; // Bob amplitude
        bobOffset_ = {
            sinf(bobTimer_) * bobAmount_ * 0.5f,
            fabsf(cosf(bobTimer_)) * bobAmount_,
            0
        };
    } else {
        // Smoothly return to center
        bobOffset_.x *= 0.9f;
        bobOffset_.y *= 0.9f;
        bobOffset_.z *= 0.9f;
        bobTimer_ = 0.0f;
    }
}

void CameraController::UpdateRecoil(float deltaTime) {
    // Recover recoil over time
    if (fabsf(recoilVertical_) > 0.001f) {
        recoilVertical_ -= recoilRecoverySpeed_ * deltaTime;
        if (recoilVertical_ < 0) recoilVertical_ = 0;
    }
    if (fabsf(recoilHorizontal_) > 0.001f) {
        recoilHorizontal_ *= (1.0f - recoilRecoverySpeed_ * deltaTime);
        if (fabsf(recoilHorizontal_) < 0.001f) recoilHorizontal_ = 0;
    }
}

void CameraController::UpdateFOV(float deltaTime) {
    float targetFOV = isADS_ ? adsFOV_ : normalFOV_;
    float lerpSpeed = isADS_ ? 12.0f : 8.0f;
    currentFOV_ += (targetFOV - currentFOV_) * lerpSpeed * deltaTime;
}

} // namespace EOSShooter
