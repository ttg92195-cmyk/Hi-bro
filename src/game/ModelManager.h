#pragma once
// ============================================================================
// EOS Shooter - ModelManager.h
// Manages loading, caching, and rendering of 3D models (weapons, props, etc.)
// Falls back to primitive rendering if models fail to load.
// ============================================================================

#include "raylib.h"
#include "Weapon.h"
#include <string>
#include <unordered_map>

namespace EOSShooter {

// Per-weapon model rendering configuration
struct WeaponModelConfig {
    Vector3 scale = {1.0f, 1.0f, 1.0f};          // Model scale factor
    Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};    // Rotation axis
    float rotationAngle = 0.0f;                     // Rotation angle in degrees
    Vector3 positionOffset = {0.0f, 0.0f, 0.0f};   // Position offset from weapon center
    bool flipX = false;                             // Mirror on X axis
    bool flipY = false;                             // Mirror on Y axis
    bool flipZ = false;                             // Mirror on Z axis
};

// First-person viewmodel configuration (separate from 3rd person)
struct FPSViewModelConfig {
    Vector3 scale = {1.0f, 1.0f, 1.0f};
    Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};
    float rotationAngle = 0.0f;
    Vector3 rightOffset = {0.3f, -0.3f, -0.5f};   // Offset from camera (right, down, back)
    float bobAmplitude = 0.003f;                     // Weapon bob amplitude
    float bobSpeed = 10.0f;                          // Weapon bob speed
};

class ModelManager {
public:
    ModelManager();
    ~ModelManager();

    // === Lifecycle ===
    bool Initialize();
    void Shutdown();

    // === Weapon Models ===
    Model* GetWeaponModel(WeaponType type);
    bool HasWeaponModel(WeaponType type) const;
    const WeaponModelConfig& GetWeaponConfig(WeaponType type) const;
    const FPSViewModelConfig& GetFPSViewModelConfig(WeaponType type) const;

    // === Utility ===
    bool IsInitialized() const { return initialized_; }
    int GetLoadedModelCount() const;

private:
    bool initialized_ = false;

    // Weapon models array (indexed by WeaponType cast to int)
    static constexpr int MAX_WEAPON_TYPES = 7;
    Model weaponModels_[MAX_WEAPON_TYPES] = {};
    bool weaponModelLoaded_[MAX_WEAPON_TYPES] = {};

    // Per-weapon configs
    WeaponModelConfig weaponConfigs_[MAX_WEAPON_TYPES] = {};
    FPSViewModelConfig fpsConfigs_[MAX_WEAPON_TYPES] = {};

    // === Internal ===
    void SetDefaultConfigs();
    void LoadWeaponModels();
    Model LoadWeaponModelFile(const std::string& filename);
    void UnloadWeaponModels();
    std::string GetAssetPath(const std::string& filename) const;

    // Helper to convert WeaponType to array index
    static int TypeToIndex(WeaponType type);
};

} // namespace EOSShooter
