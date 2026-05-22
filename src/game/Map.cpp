// ============================================================================
// EOS Shooter - Map.cpp
// ============================================================================

#include "Map.h"
#include <cmath>
#include <cstdlib>
#include <cstdint>
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
        GenerateUrbanWarehouse();
    }

    SpawnPickups();
    return true;
}

void Map::GenerateUrbanWarehouse() {
    bounds_ = {80, 15, 80};

    // Outer walls (perimeter)
    walls_.push_back(Wall{Vector3{-40, 0, -40}, Vector3{40, 0, -40}, 10, 1.0f, DARKGRAY});
    walls_.push_back(Wall{Vector3{-40, 0, 40}, Vector3{40, 0, 40}, 10, 1.0f, DARKGRAY});
    walls_.push_back(Wall{Vector3{-40, 0, -40}, Vector3{-40, 0, 40}, 10, 1.0f, DARKGRAY});
    walls_.push_back(Wall{Vector3{40, 0, -40}, Vector3{40, 0, 40}, 10, 1.0f, DARKGRAY});

    // Interior walls - warehouse sections
    walls_.push_back(Wall{Vector3{-15, 0, -20}, Vector3{-15, 0, 10}, 6, 0.5f, GRAY});
    walls_.push_back(Wall{Vector3{15, 0, -10}, Vector3{15, 0, 20}, 6, 0.5f, GRAY});
    walls_.push_back(Wall{Vector3{-20, 0, 10}, Vector3{10, 0, 10}, 6, 0.5f, GRAY});
    walls_.push_back(Wall{Vector3{-5, 0, -30}, Vector3{20, 0, -30}, 4, 0.5f, LIGHTGRAY});

    // Crates / cover objects
    walls_.push_back(Wall{Vector3{-25, 0, -25}, Vector3{-23, 0, -23}, 2, 2.0f, BROWN});
    walls_.push_back(Wall{Vector3{25, 0, 25}, Vector3{27, 0, 27}, 2, 2.0f, BROWN});
    walls_.push_back(Wall{Vector3{-30, 0, 15}, Vector3{-28, 0, 17}, 2, 2.0f, BROWN});
    walls_.push_back(Wall{Vector3{10, 0, -15}, Vector3{12, 0, -13}, 2, 2.0f, BROWN});
    walls_.push_back(Wall{Vector3{30, 0, -20}, Vector3{32, 0, -18}, 2, 2.0f, BROWN});
    walls_.push_back(Wall{Vector3{-10, 0, 30}, Vector3{-8, 0, 32}, 2, 2.0f, BROWN});

    spawnPositions_ = {
        {-35, 1, -35}, {35, 1, -35}, {-35, 1, 35}, {35, 1, 35},
        {0, 1, -35}, {0, 1, 35}, {-35, 1, 0}, {35, 1, 0}
    };

    enemySpawnPositions_ = {
        {-20, 1, -15}, {20, 1, -15}, {-20, 1, 15}, {20, 1, 15},
        {0, 1, 0}, {30, 1, -30}, {-30, 1, 30}, {10, 1, 25},
        {-25, 1, -5}, {15, 1, -25}, {-5, 1, 20}, {25, 1, 10}
    };

    coverPositions_ = {
        {-26, 1, -25}, {26, 1, 25}, {-30, 1, 16}, {11, 1, -15},
        {31, 1, -20}, {-10, 1, 31}
    };
}

void Map::GenerateDesertOutpost() {
    bounds_ = {120, 10, 120};

    walls_.push_back(Wall{Vector3{-60, 0, -60}, Vector3{60, 0, -60}, 8, 1.0f, BEIGE});
    walls_.push_back(Wall{Vector3{-60, 0, 60}, Vector3{60, 0, 60}, 8, 1.0f, BEIGE});
    walls_.push_back(Wall{Vector3{-60, 0, -60}, Vector3{-60, 0, 60}, 8, 1.0f, BEIGE});
    walls_.push_back(Wall{Vector3{60, 0, -60}, Vector3{60, 0, 60}, 8, 1.0f, BEIGE});

    walls_.push_back(Wall{Vector3{-30, 0, -30}, Vector3{-20, 0, -30}, 6, 8.0f, BEIGE});
    walls_.push_back(Wall{Vector3{20, 0, 20}, Vector3{30, 0, 20}, 6, 8.0f, BEIGE});
    walls_.push_back(Wall{Vector3{-10, 0, 10}, Vector3{10, 0, 10}, 4, 5.0f, BEIGE});

    spawnPositions_ = {
        {-50, 1, -50}, {50, 1, -50}, {-50, 1, 50}, {50, 1, 50}
    };
    enemySpawnPositions_ = {
        {-25, 1, -25}, {25, 1, 25}, {0, 1, 0}, {-30, 1, 30}, {30, 1, -30}
    };
}

void Map::GenerateArcticBase() {
    bounds_ = {90, 12, 90};

    walls_.push_back(Wall{Vector3{-45, 0, -45}, Vector3{45, 0, -45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back(Wall{Vector3{-45, 0, 45}, Vector3{45, 0, 45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back(Wall{Vector3{-45, 0, -45}, Vector3{-45, 0, 45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back(Wall{Vector3{45, 0, -45}, Vector3{45, 0, 45}, 10, 1.0f, LIGHTGRAY});

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
    // Proper gradient sky using horizontal strips
    float skyRadius = 200.0f;
    Color topColor = {8, 8, 35, 255};        // Dark navy at top
    Color midColor = {20, 30, 70, 255};       // Mid dark blue
    Color bottomColor = {40, 50, 80, 255};    // Lighter blue at horizon

    // Draw a large background sphere with subtle gradient
    DrawSphere({0, 50, 0}, skyRadius, Fade(topColor, 0.15f));
    DrawSphere({0, -20, 0}, skyRadius * 0.95f, Fade(midColor, 0.08f));

    // Horizon haze
    DrawSphere({0, -5, 0}, skyRadius * 0.9f, Fade(bottomColor, 0.03f));
}

void Map::RenderFloor() const {
    // Dark ground plane
    Color floorColor = {15, 20, 15, 255};
    DrawPlane({0, 0, 0}, {bounds_.x * 2, bounds_.z * 2}, floorColor);

    // === Neon-style grid lines ===
    // Cyan grid for sci-fi feel
    Color gridColorDim = {0, 80, 80, 255};
    Color gridColorBright = {0, 220, 220, 255};

    // Major grid lines every 10 units
    for (int i = -(int)bounds_.x; i <= (int)bounds_.x; i += 10) {
        // Wider dim line (glow)
        DrawLine3D({(float)i, 0.02f, -bounds_.z}, {(float)i, 0.02f, bounds_.z}, Fade(gridColorDim, 0.2f));
        // Narrow bright line
        DrawLine3D({(float)i, 0.03f, -bounds_.z}, {(float)i, 0.03f, bounds_.z}, Fade(gridColorBright, 0.15f));

        DrawLine3D({-bounds_.x, 0.02f, (float)i}, {bounds_.x, 0.02f, (float)i}, Fade(gridColorDim, 0.2f));
        DrawLine3D({-bounds_.x, 0.03f, (float)i}, {bounds_.x, 0.03f, (float)i}, Fade(gridColorBright, 0.15f));
    }

    // Minor grid lines every 5 units (dimmer)
    Color minorGridColor = {0, 60, 60, 255};
    for (int i = -(int)bounds_.x; i <= (int)bounds_.x; i += 5) {
        if (i % 10 == 0) continue; // Skip major lines
        DrawLine3D({(float)i, 0.01f, -bounds_.z}, {(float)i, 0.01f, bounds_.z}, Fade(minorGridColor, 0.1f));
        DrawLine3D({-bounds_.x, 0.01f, (float)i}, {bounds_.x, 0.01f, (float)i}, Fade(minorGridColor, 0.1f));
    }
}

void Map::RenderWalls() const {
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

        // Draw wall as a rotated box
        DrawCube(center, length, wall.height, wall.thickness, wall.color);

        // === Sci-fi edge highlights ===
        // Draw glowing edge lines along wall top
        Color edgeColor = {0, 180, 180, 255};
        float topY = wall.height;

        // Top edge of wall (along its length)
        Vector3 topStart = {wall.start.x, topY, wall.start.z};
        Vector3 topEnd = {wall.end.x, topY, wall.end.z};
        DrawLine3D(topStart, topEnd, Fade(edgeColor, 0.25f));

        // Bottom edge glow
        Vector3 botStart = {wall.start.x, 0.05f, wall.start.z};
        Vector3 botEnd = {wall.end.x, 0.05f, wall.end.z};
        DrawLine3D(botStart, botEnd, Fade(edgeColor, 0.15f));

        // Vertical edge highlights at wall ends
        DrawLine3D({wall.start.x, 0.05f, wall.start.z}, {wall.start.x, topY, wall.start.z}, Fade(edgeColor, 0.15f));
        DrawLine3D({wall.end.x, 0.05f, wall.end.z}, {wall.end.x, topY, wall.end.z}, Fade(edgeColor, 0.15f));
    }
}

void Map::RenderPickups() const {
    for (const auto& pickup : pickups_) {
        if (!pickup.active) continue;

        Color color;
        Color glowColor;
        switch (pickup.type) {
        case PickupType::HEALTH:  color = GREEN; glowColor = {0, 255, 100, 255}; break;
        case PickupType::ARMOR:   color = BLUE;  glowColor = {60, 140, 255, 255}; break;
        case PickupType::AMMO:    color = YELLOW; glowColor = {255, 220, 0, 255}; break;
        case PickupType::WEAPON:  color = RED;   glowColor = {255, 60, 60, 255}; break;
        default: color = WHITE; glowColor = WHITE; break;
        }

        float bobOffset = sinf(GetTime() * 3.0f) * 0.2f;
        Vector3 pos = {
            pickup.position.x,
            pickup.position.y + bobOffset,
            pickup.position.z
        };

        // Outer glow ring (larger, faded)
        DrawSphere(pos, 0.55f, Fade(glowColor, 0.1f));
        DrawSphereWires(pos, 0.55f, 8, 8, Fade(glowColor, 0.15f));

        // Main pickup sphere
        DrawSphere(pos, 0.3f, color);

        // Inner wireframe detail
        DrawSphereWires(pos, 0.35f, 8, 8, Fade(color, 0.6f));

        // Rotating ring effect around pickup
        float ringAngle = GetTime() * 2.0f;
        float ringRadius = 0.5f;
        for (int i = 0; i < 16; i++) {
            float a1 = ringAngle + (float)i * PI * 2.0f / 16.0f;
            float a2 = ringAngle + (float)(i + 1) * PI * 2.0f / 16.0f;
            Vector3 p1 = {pos.x + cosf(a1) * ringRadius, pos.y, pos.z + sinf(a1) * ringRadius};
            Vector3 p2 = {pos.x + cosf(a2) * ringRadius, pos.y, pos.z + sinf(a2) * ringRadius};
            DrawLine3D(p1, p2, Fade(glowColor, 0.3f));
        }
    }
}

void Map::SpawnPickups() {
    pickups_.clear();

    // Health pickups
    {
        Pickup p;
        p.type = PickupType::HEALTH;
        p.position = {-20, 0.5f, 0};
        p.value = 50.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p;
        p.type = PickupType::HEALTH;
        p.position = {20, 0.5f, 0};
        p.value = 50.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p;
        p.type = PickupType::HEALTH;
        p.position = {0, 0.5f, -25};
        p.value = 50.0f;
        pickups_.push_back(p);
    }

    // Armor pickups
    {
        Pickup p;
        p.type = PickupType::ARMOR;
        p.position = {-10, 0.5f, -20};
        p.value = 50.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p;
        p.type = PickupType::ARMOR;
        p.position = {10, 0.5f, 20};
        p.value = 50.0f;
        pickups_.push_back(p);
    }

    // Ammo pickups
    {
        Pickup p;
        p.type = PickupType::AMMO;
        p.position = {0, 0.5f, 0};
        p.value = 60.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p;
        p.type = PickupType::AMMO;
        p.position = {-30, 0.5f, -30};
        p.value = 60.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p;
        p.type = PickupType::AMMO;
        p.position = {30, 0.5f, 30};
        p.value = 60.0f;
        pickups_.push_back(p);
    }

    // Weapon pickups
    {
        Pickup p;
        p.type = PickupType::WEAPON;
        p.position = {-25, 0.5f, 15};
        p.weaponType = WeaponType::SHOTGUN;
        pickups_.push_back(p);
    }
    {
        Pickup p;
        p.type = PickupType::WEAPON;
        p.position = {25, 0.5f, -15};
        p.weaponType = WeaponType::SNIPER_RIFLE;
        pickups_.push_back(p);
    }
    {
        Pickup p;
        p.type = PickupType::WEAPON;
        p.position = {0, 0.5f, 30};
        p.weaponType = WeaponType::RPG;
        pickups_.push_back(p);
    }
}

// ============================================================================
// Collision
// ============================================================================

RayCollision Map::Raycast(const Ray& ray, float maxDistance) const {
    RayCollision closestHit = {0};
    closestHit.hit = false;
    closestHit.distance = maxDistance;

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

        RayCollision hit = GetRayCollisionBox(ray, box);
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
            closestHit.point = {
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

    resolved.x = std::clamp(resolved.x, -bounds_.x + radius, bounds_.x - radius);
    resolved.z = std::clamp(resolved.z, -bounds_.z + radius, bounds_.z - radius);

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
