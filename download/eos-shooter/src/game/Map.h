#pragma once
// ============================================================================
// EOS Shooter - Map.h
// Map system with geometry, spawn points, pickups, and collision.
// ============================================================================

#include "raylib.h"
#include "Weapon.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace EOSShooter {

enum class PickupType {
    HEALTH,
    ARMOR,
    AMMO,
    WEAPON
};

struct Pickup {
    PickupType type = PickupType::HEALTH;
    Vector3 position = {0, 0.5f, 0};
    float value = 0.0f;
    WeaponType weaponType = WeaponType::ASSAULT_RIFLE;
    bool active = true;
    float respawnTimer = 0.0f;
    float respawnTime = 30.0f;
};

struct Wall {
    Vector3 start;
    Vector3 end;
    float height;
    float thickness;
    Color color;
};

struct MapSpawnPoint {
    Vector3 position;
    float rotation;
    bool occupied = false;
};

class Map {
public:
    Map() = default;
    ~Map() = default;

    bool Load(const std::string& mapName);
    void Render();
    void Update(float deltaTime);
    void RenderPickups() const;

    // === Collision ===
    RayCollision Raycast(const Ray& ray, float maxDistance) const;
    bool CheckCollision(const Vector3& position, float radius) const;
    Vector3 ResolveCollision(const Vector3& position, float radius) const;

    // === Spawn Points ===
    std::vector<Vector3>& GetSpawnPoints() { return spawnPositions_; }
    Vector3 GetRandomSpawnPoint() const;
    Vector3 GetRandomEnemySpawnPoint() const;

    // === Pickups ===
    std::vector<Pickup>& GetPickups() { return pickups_; }
    void SpawnPickups();

    // === Map Data ===
    const std::string& GetName() const { return mapName_; }
    Vector3 GetBounds() const { return bounds_; }

private:
    std::string mapName_;
    Vector3 bounds_ = {100, 20, 100};
    std::vector<Wall> walls_;
    std::vector<Vector3> spawnPositions_;
    std::vector<Vector3> enemySpawnPositions_;
    std::vector<Pickup> pickups_;
    std::vector<Vector3> coverPositions_;

    void GenerateUrbanWarehouse();
    void GenerateDesertOutpost();
    void GenerateArcticBase();
    void RenderFloor() const;
    void RenderWalls() const;
    void RenderSkybox() const;
};

} // namespace EOSShooter
