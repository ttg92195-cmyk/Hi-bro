// ============================================================================
// EOS Shooter - Map.cpp
// ============================================================================

#include "Map.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace EOSShooter {

bool Map::Load(const std::string& mapName) {
    mapName_ = mapName;

    walls_.clear();
    spawnPositions_.clear();
    enemySpawnPositions_.clear();
    pickups_.clear();
    coverPositions_.clear();

    if (mapName == "urban_warehouse") {
        GenerateUrbanWarehouse();
    } else if (mapName == "desert_outpost") {
        GenerateDesertOutpost();
    } else if (mapName == "arctic_base") {
        GenerateArcticBase();
    } else {
        // Default fallback
        GenerateUrbanWarehouse();
    }

    SpawnPickups();
    return true;
}

void Map::GenerateUrbanWarehouse() {
    bounds_ = {80, 15, 80};

    // Floor and outer walls
    // Outer walls (perimeter)
    walls_.push_back({{-40, 0, -40}, {40, 0, -40}, 10, 1.0f, DARKGRAY});  // North
    walls_.push_back({{-40, 0, 40}, {40, 0, 40}, 10, 1.0f, DARKGRAY});    // South
    walls_.push_back({{-40, 0, -40}, {-40, 0, 40}, 10, 1.0f, DARKGRAY});  // West
    walls_.push_back({{40, 0, -40}, {40, 0, 40}, 10, 1.0f, DARKGRAY});    // East

    // Interior walls - warehouse sections
    walls_.push_back({{-15, 0, -20}, {-15, 0, 10}, 6, 0.5f, GRAY});
    walls_.push_back({{15, 0, -10}, {15, 0, 20}, 6, 0.5f, GRAY});
    walls_.push_back({{-20, 0, 10}, {10, 0, 10}, 6, 0.5f, GRAY});
    walls_.push_back({{-5, 0, -30}, {20, 0, -30}, 4, 0.5f, LIGHTGRAY});

    // Crates / cover objects (represented as short thick walls)
    walls_.push_back({{-25, 0, -25}, {-23, 0, -23}, 2, 2.0f, BROWN});
    walls_.push_back({{25, 0, 25}, {27, 0, 27}, 2, 2.0f, BROWN});
    walls_.push_back({{-30, 0, 15}, {-28, 0, 17}, 2, 2.0f, BROWN});
    walls_.push_back({{10, 0, -15}, {12, 0, -13}, 2, 2.0f, BROWN});
    walls_.push_back({{30, 0, -20}, {32, 0, -18}, 2, 2.0f, BROWN});
    walls_.push_back({{-10, 0, 30}, {-8, 0, 32}, 2, 2.0f, BROWN});

    // Player spawn points
    spawnPositions_ = {
        {-35, 1, -35},
        {35, 1, -35},
        {-35, 1, 35},
        {35, 1, 35},
        {0, 1, -35},
        {0, 1, 35},
        {-35, 1, 0},
        {35, 1, 0}
    };

    // Enemy spawn points (scattered around the map)
    enemySpawnPositions_ = {
        {-20, 1, -15}, {20, 1, -15}, {-20, 1, 15}, {20, 1, 15},
        {0, 1, 0}, {30, 1, -30}, {-30, 1, 30}, {10, 1, 25},
        {-25, 1, -5}, {15, 1, -25}, {-5, 1, 20}, {25, 1, 10}
    };

    // Cover positions for AI
    coverPositions_ = {
        {-26, 1, -25}, {26, 1, 25}, {-30, 1, 16}, {11, 1, -15},
        {31, 1, -20}, {-10, 1, 31}
    };
}

void Map::GenerateDesertOutpost() {
    bounds_ = {120, 10, 120};

    // Outer walls
    walls_.push_back({{-60, 0, -60}, {60, 0, -60}, 8, 1.0f, SAND});
    walls_.push_back({{-60, 0, 60}, {60, 0, 60}, 8, 1.0f, SAND});
    walls_.push_back({{-60, 0, -60}, {-60, 0, 60}, 8, 1.0f, SAND});
    walls_.push_back({{60, 0, -60}, {60, 0, 60}, 8, 1.0f, SAND});

    // Buildings
    walls_.push_back({{-30, 0, -30}, {-20, 0, -30}, 6, 8.0f, BEIGE});
    walls_.push_back({{20, 0, 20}, {30, 0, 20}, 6, 8.0f, BEIGE});
    walls_.push_back({{-10, 0, 10}, {10, 0, 10}, 4, 5.0f, BEIGE});

    spawnPositions_ = {
        {-50, 1, -50}, {50, 1, -50}, {-50, 1, 50}, {50, 1, 50}
    };
    enemySpawnPositions_ = {
        {-25, 1, -25}, {25, 1, 25}, {0, 1, 0}, {-30, 1, 30}, {30, 1, -30}
    };
}

void Map::GenerateArcticBase() {
    bounds_ = {90, 12, 90};

    walls_.push_back({{-45, 0, -45}, {45, 0, -45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back({{-45, 0, 45}, {45, 0, 45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back({{-45, 0, -45}, {-45, 0, 45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back({{45, 0, -45}, {45, 0, 45}, 10, 1.0f, LIGHTGRAY});

    spawnPositions_ = {
        {-40, 1, -40}, {40, 1, -40}, {-40, 1, 40}, {40, 1, 40}
    };
    enemySpawnPositions_ = {
        {-20, 1, -20}, {20, 1, 20}, {0, 1, 0}, {-25, 1, 25}, {25, 1, -25}
    };
}

// ============================================================================
// Update & Render
// ============================================================================

void Map::Update(float deltaTime) {
    // Update pickup respawns
    for (auto& pickup : pickups_) {
        if (!pickup.active) {
            pickup.respawnTimer -= deltaTime;
            if (pickup.respawnTimer <= 0) {
                pickup.active = true;
            }
        }
    }
}

void Map::Render() {
    RenderSkybox();
    RenderFloor();
    RenderWalls();
}

void Map::RenderSkybox() const {
    // Simple gradient sky
    Color topColor = {25, 25, 50, 255};
    Color bottomColor = {80, 80, 120, 255};

    // Draw a large box around the scene
    DrawSphere({0, 0, 0}, 200, Fade(SKYBLUE, 0.1f));
}

void Map::RenderFloor() const {
    // Ground plane
    DrawPlane({0, 0, 0}, {bounds_.x * 2, bounds_.z * 2}, DARKGREEN);

    // Grid lines for reference
    for (int i = -(int)bounds_.x; i <= (int)bounds_.x; i += 10) {
        DrawLine3D({(float)i, 0.01f, -bounds_.z}, {(float)i, 0.01f, bounds_.z}, Fade(GRAY, 0.3f));
        DrawLine3D({-bounds_.x, 0.01f, (float)i}, {bounds_.x, 0.01f, (float)i}, Fade(GRAY, 0.3f));
    }
}

void Map::RenderWalls() const {
    for (const auto& wall : walls_) {
        // Calculate wall center and dimensions
        Vector3 center = {
            (wall.start.x + wall.end.x) * 0.5f,
            wall.height * 0.5f,
            (wall.start.z + wall.end.z) * 0.5f
        };

        float length = sqrtf(
            (wall.end.x - wall.start.x) * (wall.end.x - wall.start.x) +
            (wall.end.z - wall.start.z) * (wall.end.z - wall.start.z)
        );

        // Calculate wall rotation
        float angle = atan2f(wall.end.z - wall.start.z, wall.end.x - wall.start.x);

        // Draw wall as a rotated box
        DrawCube(center, length, wall.height, wall.thickness, wall.color);
    }
}

void Map::RenderPickups() const {
    for (const auto& pickup : pickups_) {
        if (!pickup.active) continue;

        Color color;
        switch (pickup.type) {
        case PickupType::HEALTH:  color = GREEN; break;
        case PickupType::ARMOR:   color = BLUE; break;
        case PickupType::AMMO:    color = YELLOW; break;
        case PickupType::WEAPON:  color = RED; break;
        default: color = WHITE; break;
        }

        // Floating and rotating pickup
        float bobOffset = sinf(GetTime() * 3.0f) * 0.2f;
        Vector3 pos = {
            pickup.position.x,
            pickup.position.y + bobOffset,
            pickup.position.z
        };

        DrawSphere(pos, 0.3f, color);
        DrawSphereWires(pos, 0.4f, 8, 8, Fade(color, 0.5f));
    }
}

void Map::SpawnPickups() {
    pickups_.clear();

    // Health pickups
    pickups_.push_back({PickupType::HEALTH, {-20, 0.5f, 0}, 50.0f});
    pickups_.push_back({PickupType::HEALTH, {20, 0.5f, 0}, 50.0f});
    pickups_.push_back({PickupType::HEALTH, {0, 0.5f, -25}, 50.0f});

    // Armor pickups
    pickups_.push_back({PickupType::ARMOR, {-10, 0.5f, -20}, 50.0f});
    pickups_.push_back({PickupType::ARMOR, {10, 0.5f, 20}, 50.0f});

    // Ammo pickups
    pickups_.push_back({PickupType::AMMO, {0, 0.5f, 0}, 60.0f});
    pickups_.push_back({PickupType::AMMO, {-30, 0.5f, -30}, 60.0f});
    pickups_.push_back({PickupType::AMMO, {30, 0.5f, 30}, 60.0f});

    // Weapon pickups
    pickups_.push_back({PickupType::WEAPON, {-25, 0.5f, 15}, 0, WeaponType::SHOTGUN});
    pickups_.push_back({PickupType::WEAPON, {25, 0.5f, -15}, 0, WeaponType::SNIPER_RIFLE});
    pickups_.push_back({PickupType::WEAPON, {0, 0.5f, 30}, 0, WeaponType::RPG});
}

// ============================================================================
// Collision
// ============================================================================

RayHitInfo Map::Raycast(const Ray& ray, float maxDistance) const {
    RayHitInfo closestHit = {0};
    closestHit.hit = false;
    closestHit.distance = maxDistance;

    // Test against each wall bounding box
    for (const auto& wall : walls_) {
        Vector3 center = {
            (wall.start.x + wall.end.x) * 0.5f,
            wall.height * 0.5f,
            (wall.start.z + wall.end.z) * 0.5f
        };

        float length = sqrtf(
            (wall.end.x - wall.start.x) * (wall.end.x - wall.start.x) +
            (wall.end.z - wall.start.z) * (wall.end.z - wall.start.z)
        );

        BoundingBox box = {
            {center.x - length * 0.5f, 0, center.z - wall.thickness * 0.5f},
            {center.x + length * 0.5f, wall.height, center.z + wall.thickness * 0.5f}
        };

        RayHitInfo hit = GetRayCollisionBox(ray, box);
        if (hit.hit && hit.distance < closestHit.distance) {
            closestHit = hit;
        }
    }

    // Floor collision
    if (ray.direction.y < 0) {
        float t = -ray.position.y / ray.direction.y;
        if (t > 0 && t < closestHit.distance) {
            closestHit.hit = true;
            closestHit.distance = t;
            closestHit.position = {
                ray.position.x + ray.direction.x * t,
                0,
                ray.position.z + ray.direction.z * t
            };
            closestHit.normal = {0, 1, 0};
        }
    }

    return closestHit;
}

bool Map::CheckCollision(const Vector3& position, float radius) const {
    for (const auto& wall : walls_) {
        Vector3 center = {
            (wall.start.x + wall.end.x) * 0.5f,
            wall.height * 0.5f,
            (wall.start.z + wall.end.z) * 0.5f
        };

        // Simple AABB vs sphere check
        float length = sqrtf(
            (wall.end.x - wall.start.x) * (wall.end.x - wall.start.x) +
            (wall.end.z - wall.start.z) * (wall.end.z - wall.start.z)
        );

        BoundingBox box = {
            {center.x - length * 0.5f, 0, center.z - wall.thickness * 0.5f},
            {center.x + length * 0.5f, wall.height, center.z + wall.thickness * 0.5f}
        };

        if (CheckCollisionBoxSphere(box, position, radius)) {
            return true;
        }
    }
    return false;
}

Vector3 Map::ResolveCollision(const Vector3& position, float radius) const {
    Vector3 resolved = position;

    for (const auto& wall : walls_) {
        Vector3 center = {
            (wall.start.x + wall.end.x) * 0.5f,
            wall.height * 0.5f,
            (wall.start.z + wall.end.z) * 0.5f
        };

        float length = sqrtf(
            (wall.end.x - wall.start.x) * (wall.end.x - wall.start.x) +
            (wall.end.z - wall.start.z) * (wall.end.z - wall.start.z)
        );

        BoundingBox box = {
            {center.x - length * 0.5f, 0, center.z - wall.thickness * 0.5f},
            {center.x + length * 0.5f, wall.height, center.z + wall.thickness * 0.5f}
        };

        if (CheckCollisionBoxSphere(box, resolved, radius)) {
            // Push out of collision
            Vector3 dir = {
                resolved.x - center.x,
                0,
                resolved.z - center.z
            };
            float dist = sqrtf(dir.x * dir.x + dir.z * dir.z);
            if (dist > 0.01f) {
                dir.x /= dist;
                dir.z /= dist;
                resolved.x = center.x + dir.x * (length * 0.5f + radius + 0.1f);
                resolved.z = center.z + dir.z * (wall.thickness * 0.5f + radius + 0.1f);
            }
        }
    }

    // Clamp to map bounds
    resolved.x = Clamp(resolved.x, -bounds_.x + radius, bounds_.x - radius);
    resolved.z = Clamp(resolved.z, -bounds_.z + radius, bounds_.z - radius);

    return resolved;
}

Vector3 Map::GetRandomSpawnPoint() const {
    if (spawnPositions_.empty()) return {0, 1, 0};
    return spawnPositions_[rand() % spawnPositions_.size()];
}

Vector3 Map::GetRandomEnemySpawnPoint() const {
    if (enemySpawnPositions_.empty()) return {0, 1, 5};
    return enemySpawnPositions_[rand() % enemySpawnPositions_.size()];
}

} // namespace EOSShooter
