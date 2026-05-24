// ============================================================================
// EOS Shooter - ModelManager.cpp
// Full implementation of the 3D model management system.
// Loads .glb weapon models and provides rendering configuration.
// ============================================================================

#include "ModelManager.h"
#include <cstring>

namespace EOSShooter {

// ============================================================================
// Construction / Destruction
// ============================================================================

ModelManager::ModelManager() {
    std::memset(weaponModels_, 0, sizeof(weaponModels_));
    std::memset(weaponModelLoaded_, 0, sizeof(weaponModelLoaded_));
    SetDefaultConfigs();
}

ModelManager::~ModelManager() {
    Shutdown();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool ModelManager::Initialize() {
    if (initialized_) return true;

    TraceLog(LOG_INFO, "ModelManager: Initializing...");

    LoadWeaponModels();

    initialized_ = true;

    int loadedCount = GetLoadedModelCount();
    TraceLog(LOG_INFO, "ModelManager: Initialized - %d/%d weapon models loaded",
             loadedCount, MAX_WEAPON_TYPES);

    return true;
}

void ModelManager::Shutdown() {
    if (!initialized_) return;

    TraceLog(LOG_INFO, "ModelManager: Shutting down...");
    UnloadWeaponModels();
    initialized_ = false;
}

// ============================================================================
// Weapon Model Access
// ============================================================================

Model* ModelManager::GetWeaponModel(WeaponType type) {
    int idx = TypeToIndex(type);
    if (idx < 0 || idx >= MAX_WEAPON_TYPES) return nullptr;
    if (!weaponModelLoaded_[idx]) return nullptr;
    return &weaponModels_[idx];
}

bool ModelManager::HasWeaponModel(WeaponType type) const {
    int idx = TypeToIndex(type);
    if (idx < 0 || idx >= MAX_WEAPON_TYPES) return false;
    return weaponModelLoaded_[idx];
}

const WeaponModelConfig& ModelManager::GetWeaponConfig(WeaponType type) const {
    int idx = TypeToIndex(type);
    if (idx < 0 || idx >= MAX_WEAPON_TYPES) idx = 0;
    return weaponConfigs_[idx];
}

const FPSViewModelConfig& ModelManager::GetFPSViewModelConfig(WeaponType type) const {
    int idx = TypeToIndex(type);
    if (idx < 0 || idx >= MAX_WEAPON_TYPES) idx = 0;
    return fpsConfigs_[idx];
}

int ModelManager::GetLoadedModelCount() const {
    int count = 0;
    for (int i = 0; i < MAX_WEAPON_TYPES; i++) {
        if (weaponModelLoaded_[i]) count++;
    }
    return count;
}

// ============================================================================
// Default Configuration
// ============================================================================

void ModelManager::SetDefaultConfigs() {
    // === ASSAULT_RIFLE (M4A1) ===
    // Index 0
    weaponConfigs_[0] = {
        {0.35f, 0.35f, 0.35f},     // scale
        {0.0f, 1.0f, 0.0f},        // rotationAxis
        -90.0f,                      // rotationAngle - rotate to face forward
        {0.0f, -0.05f, 0.1f},       // positionOffset
        false, false, false          // no flip
    };
    fpsConfigs_[0] = {
        {0.5f, 0.5f, 0.5f},         // scale (larger for FPS view)
        {0.0f, 1.0f, 0.0f},         // rotationAxis
        -90.0f,                       // rotationAngle
        {0.25f, -0.25f, 0.5f},      // rightOffset (right, down, FORWARD)
        0.003f,                       // bobAmplitude
        10.0f                         // bobSpeed
    };

    // === SMG (AK47 as SMG) ===
    // Index 1
    weaponConfigs_[1] = {
        {0.3f, 0.3f, 0.3f},         // scale
        {0.0f, 1.0f, 0.0f},         // rotationAxis
        -90.0f,                       // rotationAngle
        {0.0f, -0.05f, 0.1f},       // positionOffset
        false, false, false
    };
    fpsConfigs_[1] = {
        {0.45f, 0.45f, 0.45f},
        {0.0f, 1.0f, 0.0f},
        -90.0f,
        {0.25f, -0.22f, 0.45f},     // rightOffset (right, down, FORWARD)
        0.004f,   // faster bob for SMG
        12.0f
    };

    // === SHOTGUN ===
    // Index 2
    weaponConfigs_[2] = {
        {0.35f, 0.35f, 0.35f},     // scale
        {0.0f, 1.0f, 0.0f},        // rotationAxis
        -90.0f,                      // rotationAngle
        {0.0f, -0.05f, 0.1f},      // positionOffset
        false, false, false
    };
    fpsConfigs_[2] = {
        {0.5f, 0.5f, 0.5f},
        {0.0f, 1.0f, 0.0f},
        -90.0f,
        {0.28f, -0.28f, 0.5f},     // rightOffset (right, down, FORWARD)
        0.002f,                      // slower bob for heavy weapon
        8.0f
    };

    // === SNIPER_RIFLE ===
    // Index 3
    weaponConfigs_[3] = {
        {0.4f, 0.4f, 0.4f},        // scale (longer weapon)
        {0.0f, 1.0f, 0.0f},         // rotationAxis
        -90.0f,                       // rotationAngle
        {0.0f, -0.05f, 0.15f},      // positionOffset (further forward)
        false, false, false
    };
    fpsConfigs_[3] = {
        {0.55f, 0.55f, 0.55f},
        {0.0f, 1.0f, 0.0f},
        -90.0f,
        {0.22f, -0.25f, 0.55f},    // rightOffset (right, down, FORWARD)
        0.001f,                      // minimal bob for precision weapon
        6.0f
    };

    // === PISTOL ===
    // Index 4
    weaponConfigs_[4] = {
        {0.5f, 0.5f, 0.5f},        // scale (pistol is smaller, needs more scale)
        {0.0f, 1.0f, 0.0f},         // rotationAxis
        -90.0f,                       // rotationAngle
        {0.0f, -0.03f, 0.05f},      // positionOffset
        false, false, false
    };
    fpsConfigs_[4] = {
        {0.7f, 0.7f, 0.7f},
        {0.0f, 1.0f, 0.0f},
        -90.0f,
        {0.2f, -0.18f, 0.4f},      // rightOffset (right, down, FORWARD)
        0.004f,
        11.0f
    };

    // === RPG ===
    // Index 5
    weaponConfigs_[5] = {
        {0.3f, 0.3f, 0.3f},        // scale
        {0.0f, 1.0f, 0.0f},         // rotationAxis
        -90.0f,                       // rotationAngle
        {0.0f, -0.08f, 0.12f},      // positionOffset (lower, further forward)
        false, false, false
    };
    fpsConfigs_[5] = {
        {0.45f, 0.45f, 0.45f},
        {0.0f, 1.0f, 0.0f},
        -90.0f,
        {0.28f, -0.3f, 0.5f},      // rightOffset (right, down, FORWARD)
        0.001f,                      // minimal bob for heavy weapon
        5.0f
    };

    // === MELEE ===
    // Index 6 - No model, keep as primitive
    weaponConfigs_[6] = {
        {1.0f, 1.0f, 1.0f},
        {0.0f, 1.0f, 0.0f},
        0.0f,
        {0.0f, 0.0f, 0.0f},
        false, false, false
    };
    fpsConfigs_[6] = {
        {1.0f, 1.0f, 1.0f},
        {0.0f, 1.0f, 0.0f},
        0.0f,
        {0.2f, -0.15f, 0.35f},     // rightOffset (right, down, FORWARD)
        0.005f,
        14.0f
    };
}

// ============================================================================
// Model Loading
// ============================================================================

void ModelManager::LoadWeaponModels() {
    TraceLog(LOG_INFO, "ModelManager: Loading weapon models...");

    // Define model file for each weapon type
    struct ModelFileDef {
        WeaponType type;
        const char* filename;
    };

    ModelFileDef modelDefs[] = {
        { WeaponType::ASSAULT_RIFLE,  "m4a1.glb" },
        { WeaponType::SMG,            "ak47.glb" },
        { WeaponType::SHOTGUN,        "shotgun.glb" },
        { WeaponType::SNIPER_RIFLE,   "sniper.glb" },
        { WeaponType::PISTOL,         "pistol.glb" },
        { WeaponType::RPG,            "rpg.glb" },
        // MELEE has no model
    };

    int numDefs = sizeof(modelDefs) / sizeof(modelDefs[0]);

    for (int i = 0; i < numDefs; i++) {
        int idx = TypeToIndex(modelDefs[i].type);
        if (idx < 0 || idx >= MAX_WEAPON_TYPES) continue;

        std::string path = GetAssetPath(modelDefs[i].filename);
        TraceLog(LOG_INFO, "ModelManager: Loading %s from %s",
                 modelDefs[i].filename, path.c_str());

        if (FileExists(path.c_str())) {
            weaponModels_[idx] = LoadModel(path.c_str());

            // Check if model loaded successfully
            if (weaponModels_[idx].meshCount > 0) {
                weaponModelLoaded_[idx] = true;
                TraceLog(LOG_INFO, "ModelManager: Loaded %s (%d meshes)",
                         modelDefs[i].filename, weaponModels_[idx].meshCount);
            } else {
                TraceLog(LOG_WARNING, "ModelManager: Failed to load %s - model has no meshes",
                         modelDefs[i].filename);
                weaponModelLoaded_[idx] = false;
            }
        } else {
            TraceLog(LOG_WARNING, "ModelManager: File not found: %s", path.c_str());
            weaponModelLoaded_[idx] = false;
        }
    }

    // MELEE has no model - always false
    weaponModelLoaded_[TypeToIndex(WeaponType::MELEE)] = false;
}

void ModelManager::UnloadWeaponModels() {
    for (int i = 0; i < MAX_WEAPON_TYPES; i++) {
        if (weaponModelLoaded_[i]) {
            UnloadModel(weaponModels_[i]);
            weaponModels_[i] = {};
            weaponModelLoaded_[i] = false;
        }
    }
}

std::string ModelManager::GetAssetPath(const std::string& filename) const {
    // Try multiple paths to support different platforms
    // Desktop: assets/models/ relative to working directory
    // Android: Raylib VFS handles the assets directory

#if defined(PLATFORM_ANDROID)
    // On Android, Raylib searches APK assets directory
    return "assets/models/" + filename;
#else
    // Desktop: relative to working directory
    return "assets/models/" + filename;
#endif
}

// ============================================================================
// Helpers
// ============================================================================

int ModelManager::TypeToIndex(WeaponType type) {
    switch (type) {
    case WeaponType::ASSAULT_RIFLE:  return 0;
    case WeaponType::SMG:            return 1;
    case WeaponType::SHOTGUN:        return 2;
    case WeaponType::SNIPER_RIFLE:   return 3;
    case WeaponType::PISTOL:         return 4;
    case WeaponType::RPG:            return 5;
    case WeaponType::MELEE:          return 6;
    default:                         return -1;
    }
}

} // namespace EOSShooter
