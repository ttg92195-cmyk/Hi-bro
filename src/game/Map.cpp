// ============================================================================
// EOS Shooter - Map.cpp
// Enhanced map with platforms, ramps, pillars, environmental objects,
// neon lights, arches, and staircases for a rich FPS experience.
// ============================================================================

#include "Map.h"
#include "TextureGenerator.h"
#include "rlgl.h"
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <algorithm>

namespace EOSShooter {

// ============================================================================
// Load
// ============================================================================

bool Map::Load(const std::string& mapName) {
    mapName_ = mapName;
    animTime_ = 0.0f;

    // Unload previous map data
    UnloadTextures();
    walls_.clear();
    platforms_.clear();
    ramps_.clear();
    pillars_.clear();
    envObjects_.clear();
    neonLights_.clear();
    arches_.clear();
    staircases_.clear();
    spawnPositions_.clear();
    enemySpawnPositions_.clear();
    pickups_.clear();
    coverPositions_.clear();

    // Generate procedural textures
    InitTextures();

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

// ============================================================================
// Texture Management
// ============================================================================

void Map::InitTextures() {
    UnloadTextures();  // Clean up any existing textures

    texBrick_ = TextureGenerator::GenerateBrick();
    texConcrete_ = TextureGenerator::GenerateConcrete();
    texMetal_ = TextureGenerator::GenerateMetal();
    texWood_ = TextureGenerator::GenerateWood();
    texStone_ = TextureGenerator::GenerateStone();
    texFloorTile_ = TextureGenerator::GenerateFloorTile();
    texSand_ = TextureGenerator::GenerateSand();
    texSnow_ = TextureGenerator::GenerateSnow();
    texCrate_ = TextureGenerator::GenerateCrate();
    texBarrel_ = TextureGenerator::GenerateBarrel();
    texRust_ = TextureGenerator::GenerateRust();
    texDoor_ = TextureGenerator::GenerateDoor();
    texStair_ = TextureGenerator::GenerateStair();
    texGrass_ = TextureGenerator::GenerateGrass();

    texturesLoaded_ = true;
}

void Map::UnloadTextures() {
    if (!texturesLoaded_) return;
    UnloadTexture(texBrick_);
    UnloadTexture(texConcrete_);
    UnloadTexture(texMetal_);
    UnloadTexture(texWood_);
    UnloadTexture(texStone_);
    UnloadTexture(texFloorTile_);
    UnloadTexture(texSand_);
    UnloadTexture(texSnow_);
    UnloadTexture(texCrate_);
    UnloadTexture(texBarrel_);
    UnloadTexture(texRust_);
    UnloadTexture(texDoor_);
    UnloadTexture(texStair_);
    UnloadTexture(texGrass_);
    texturesLoaded_ = false;
}

Texture2D Map::GetWallTexture() const {
    if (mapName_ == "desert_outpost") return texSand_;
    if (mapName_ == "arctic_base") return texConcrete_;
    return texBrick_;
}

Texture2D Map::GetFloorTexture() const {
    if (mapName_ == "desert_outpost") return texSand_;
    if (mapName_ == "arctic_base") return texSnow_;
    return texFloorTile_;
}

Texture2D Map::GetPlatformTexture() const {
    if (mapName_ == "desert_outpost") return texConcrete_;
    if (mapName_ == "arctic_base") return texConcrete_;
    return texConcrete_;
}

Texture2D Map::GetStairTexture() const {
    return texStair_;
}

// ============================================================================
// Textured Drawing Helpers - using rlgl for UV tiling control
// ============================================================================

void Map::DrawTexturedBox(Texture2D tex, Vector3 pos, float w, float h, float d,
                           Color tint, float tileScale) const {
    float hw = w * 0.5f;
    float hh = h * 0.5f;
    float hd = d * 0.5f;

    // UV tiling based on face dimensions
    float uvW = w * tileScale;
    float uvH = h * tileScale;
    float uvD = d * tileScale;

    rlCheckRenderBatchLimit(24);
    rlSetTexture(tex.id);
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    // Front face (+Z)
    rlNormal3f(0, 0, 1);
    rlTexCoord2f(0, 0); rlVertex3f(-hw, -hh, hd);
    rlTexCoord2f(uvW, 0); rlVertex3f(hw, -hh, hd);
    rlTexCoord2f(uvW, uvH); rlVertex3f(hw, hh, hd);
    rlTexCoord2f(0, uvH); rlVertex3f(-hw, hh, hd);

    // Back face (-Z)
    rlNormal3f(0, 0, -1);
    rlTexCoord2f(uvW, 0); rlVertex3f(-hw, -hh, -hd);
    rlTexCoord2f(0, 0); rlVertex3f(hw, -hh, -hd);
    rlTexCoord2f(0, uvH); rlVertex3f(hw, hh, -hd);
    rlTexCoord2f(uvW, uvH); rlVertex3f(-hw, hh, -hd);

    // Right face (+X)
    rlNormal3f(1, 0, 0);
    rlTexCoord2f(0, 0); rlVertex3f(hw, -hh, hd);
    rlTexCoord2f(uvD, 0); rlVertex3f(hw, -hh, -hd);
    rlTexCoord2f(uvD, uvH); rlVertex3f(hw, hh, -hd);
    rlTexCoord2f(0, uvH); rlVertex3f(hw, hh, hd);

    // Left face (-X)
    rlNormal3f(-1, 0, 0);
    rlTexCoord2f(uvD, 0); rlVertex3f(-hw, -hh, -hd);
    rlTexCoord2f(0, 0); rlVertex3f(-hw, -hh, hd);
    rlTexCoord2f(0, uvH); rlVertex3f(-hw, hh, hd);
    rlTexCoord2f(uvD, uvH); rlVertex3f(-hw, hh, -hd);

    // Top face (+Y)
    rlNormal3f(0, 1, 0);
    rlTexCoord2f(0, uvD); rlVertex3f(-hw, hh, -hd);
    rlTexCoord2f(uvW, uvD); rlVertex3f(hw, hh, -hd);
    rlTexCoord2f(uvW, 0); rlVertex3f(hw, hh, hd);
    rlTexCoord2f(0, 0); rlVertex3f(-hw, hh, hd);

    // Bottom face (-Y)
    rlNormal3f(0, -1, 0);
    rlTexCoord2f(0, 0); rlVertex3f(-hw, -hh, hd);
    rlTexCoord2f(uvW, 0); rlVertex3f(hw, -hh, hd);
    rlTexCoord2f(uvW, uvD); rlVertex3f(hw, -hh, -hd);
    rlTexCoord2f(0, uvD); rlVertex3f(-hw, -hh, -hd);

    rlEnd();
    rlPopMatrix();
    rlSetTexture(0);
}

void Map::DrawTexturedPlane(Texture2D tex, Vector3 pos, Vector2 size,
                              Color tint, float tileScale) const {
    float hw = size.x * 0.5f;
    float hd = size.y * 0.5f;
    float uvW = size.x * tileScale;
    float uvD = size.y * tileScale;

    rlCheckRenderBatchLimit(4);
    rlSetTexture(tex.id);
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlBegin(RL_QUADS);
    rlNormal3f(0, 1, 0);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    rlTexCoord2f(0, 0); rlVertex3f(-hw, 0, hd);
    rlTexCoord2f(uvW, 0); rlVertex3f(hw, 0, hd);
    rlTexCoord2f(uvW, uvD); rlVertex3f(hw, 0, -hd);
    rlTexCoord2f(0, uvD); rlVertex3f(-hw, 0, -hd);
    rlEnd();
    rlPopMatrix();
    rlSetTexture(0);
}

void Map::DrawTexturedCylinder(Texture2D tex, Vector3 pos, float radius, float height,
                                 int slices, Color tint, float tileU, float tileV) const {
    float halfH = height * 0.5f;
    float angleStep = 2.0f * PI / slices;

    rlCheckRenderBatchLimit(slices * 4 + slices * 6);
    rlSetTexture(tex.id);
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);

    // Cylinder body
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    for (int i = 0; i < slices; i++) {
        float a1 = i * angleStep;
        float a2 = (i + 1) * angleStep;
        float u1 = i * tileU / slices;
        float u2 = (i + 1) * tileU / slices;

        rlNormal3f(cosf(a1), 0, sinf(a1));
        rlTexCoord2f(u1, 0); rlVertex3f(radius * cosf(a1), -halfH, radius * sinf(a1));
        rlTexCoord2f(u2, 0); rlVertex3f(radius * cosf(a2), -halfH, radius * sinf(a2));
        rlTexCoord2f(u2, tileV); rlVertex3f(radius * cosf(a2), halfH, radius * sinf(a2));
        rlTexCoord2f(u1, tileV); rlVertex3f(radius * cosf(a1), halfH, radius * sinf(a1));
    }
    rlEnd();

    // Top cap (triangle fan emulated with triangles)
    rlBegin(RL_TRIANGLES);
    rlNormal3f(0, 1, 0);
    for (int i = 0; i < slices; i++) {
        float a1 = i * angleStep;
        float a2 = (i + 1) * angleStep;
        float u1 = cosf(a1) * 0.5f + 0.5f;
        float v1 = sinf(a1) * 0.5f + 0.5f;
        float u2 = cosf(a2) * 0.5f + 0.5f;
        float v2 = sinf(a2) * 0.5f + 0.5f;
        rlTexCoord2f(0.5f, 0.5f); rlVertex3f(0, halfH, 0);
        rlTexCoord2f(u1, v1); rlVertex3f(radius * cosf(a1), halfH, radius * sinf(a1));
        rlTexCoord2f(u2, v2); rlVertex3f(radius * cosf(a2), halfH, radius * sinf(a2));
    }
    rlEnd();

    // Bottom cap (triangle fan emulated with triangles)
    rlBegin(RL_TRIANGLES);
    rlNormal3f(0, -1, 0);
    for (int i = 0; i < slices; i++) {
        float a1 = i * angleStep;
        float a2 = (i + 1) * angleStep;
        float u1 = cosf(a1) * 0.5f + 0.5f;
        float v1 = sinf(a1) * 0.5f + 0.5f;
        float u2 = cosf(a2) * 0.5f + 0.5f;
        float v2 = sinf(a2) * 0.5f + 0.5f;
        rlTexCoord2f(0.5f, 0.5f); rlVertex3f(0, -halfH, 0);
        rlTexCoord2f(u2, v2); rlVertex3f(radius * cosf(a2), -halfH, radius * sinf(a2));
        rlTexCoord2f(u1, v1); rlVertex3f(radius * cosf(a1), -halfH, radius * sinf(a1));
    }
    rlEnd();

    rlPopMatrix();
    rlSetTexture(0);
}

// ============================================================================
// Urban Warehouse - Full detailed map
// ============================================================================

void Map::GenerateUrbanWarehouse() {
    bounds_ = {80, 20, 80};

    // === Outer Perimeter Walls ===
    walls_.push_back(Wall{Vector3{-40, 0, -40}, Vector3{40, 0, -40}, 12, 1.0f, DARKGRAY});
    walls_.push_back(Wall{Vector3{-40, 0, 40},  Vector3{40, 0, 40},  12, 1.0f, DARKGRAY});
    walls_.push_back(Wall{Vector3{-40, 0, -40}, Vector3{-40, 0, 40}, 12, 1.0f, DARKGRAY});
    walls_.push_back(Wall{Vector3{40, 0, -40},  Vector3{40, 0, 40},  12, 1.0f, DARKGRAY});

    // === Interior Walls - Warehouse Sections ===
    walls_.push_back(Wall{Vector3{-15, 0, -20}, Vector3{-15, 0, 10}, 6, 0.5f, GRAY});
    walls_.push_back(Wall{Vector3{15, 0, -10},  Vector3{15, 0, 20},  6, 0.5f, GRAY});
    walls_.push_back(Wall{Vector3{-20, 0, 10},  Vector3{10, 0, 10},  6, 0.5f, GRAY});
    walls_.push_back(Wall{Vector3{-5, 0, -30},  Vector3{20, 0, -30}, 4, 0.5f, LIGHTGRAY});
    // Additional dividing walls
    walls_.push_back(Wall{Vector3{-30, 0, -10}, Vector3{-30, 0, 5},  5, 0.4f, (Color){100, 100, 110, 255}});
    walls_.push_back(Wall{Vector3{25, 0, -5},   Vector3{25, 0, 15},  5, 0.4f, (Color){100, 100, 110, 255}});

    // === Elevated Platforms (sniper positions, walkways) ===
    // Central raised platform - good overview position
    platforms_.push_back(Platform{
        {0, 0, 0}, 8.0f, 8.0f, 3.0f, 0.5f,
        (Color){50, 55, 60, 255}, (Color){0, 200, 200, 255}, true
    });

    // North sniper tower platform
    platforms_.push_back(Platform{
        {0, 0, -30}, 6.0f, 4.0f, 5.0f, 0.5f,
        (Color){45, 50, 55, 255}, (Color){0, 255, 150, 255}, true
    });

    // South walkway
    platforms_.push_back(Platform{
        {0, 0, 25}, 16.0f, 3.0f, 2.5f, 0.4f,
        (Color){50, 55, 60, 255}, (Color){0, 200, 200, 255}, true
    });

    // East guard post
    platforms_.push_back(Platform{
        {30, 0, 0}, 5.0f, 5.0f, 4.0f, 0.5f,
        (Color){55, 50, 45, 255}, (Color){255, 150, 0, 255}, true
    });

    // West elevated position
    platforms_.push_back(Platform{
        {-30, 0, -5}, 4.0f, 6.0f, 3.5f, 0.5f,
        (Color){45, 55, 50, 255}, (Color){150, 0, 255, 255}, true
    });

    // Corner lookouts
    platforms_.push_back(Platform{
        {-33, 0, -33}, 3.0f, 3.0f, 6.0f, 0.4f,
        (Color){50, 50, 55, 255}, (Color){0, 200, 200, 255}, true
    });
    platforms_.push_back(Platform{
        {33, 0, 33}, 3.0f, 3.0f, 6.0f, 0.4f,
        (Color){50, 50, 55, 255}, (Color){0, 200, 200, 255}, true
    });

    // === Staircases (connecting ground to platforms) ===
    // Central platform stairs (south side)
    staircases_.push_back(Staircase{
        {0, 0, 5}, 3.0f, 3.0f, 4.0f, 6, 0.0f,
        (Color){70, 75, 80, 255}, (Color){0, 180, 180, 255}
    });

    // North tower stairs (south side, facing south)
    staircases_.push_back(Staircase{
        {0, 0, -25}, 3.0f, 5.0f, 5.0f, 10, 0.0f,
        (Color){70, 75, 80, 255}, (Color){0, 220, 150, 255}
    });

    // South walkway stairs (north side)
    staircases_.push_back(Staircase{
        {-5, 0, 22}, 2.5f, 2.5f, 3.0f, 5, 0.0f,
        (Color){70, 75, 80, 255}, (Color){0, 180, 180, 255}
    });

    // East guard post stairs (west side, facing west)
    staircases_.push_back(Staircase{
        {27, 0, 0}, 2.5f, 4.0f, 3.5f, 8, 1.5708f,
        (Color){70, 75, 80, 255}, (Color){200, 120, 0, 255}
    });

    // West position stairs (east side, facing east)
    staircases_.push_back(Staircase{
        {-27, 0, -5}, 2.5f, 3.5f, 3.0f, 7, -1.5708f,
        (Color){70, 75, 80, 255}, (Color){120, 0, 200, 255}
    });

    // Corner lookout stairs
    staircases_.push_back(Staircase{
        {-30, 0, -30}, 2.0f, 6.0f, 4.0f, 12, 0.7854f,
        (Color){70, 75, 80, 255}, (Color){0, 180, 180, 255}
    });
    staircases_.push_back(Staircase{
        {30, 0, 30}, 2.0f, 6.0f, 4.0f, 12, -2.3562f,
        (Color){70, 75, 80, 255}, (Color){0, 180, 180, 255}
    });

    // === Ramps ===
    // Loading dock ramp
    ramps_.push_back(Ramp{
        {-25, 0, 20}, 4.0f, 6.0f, 0.0f, 2.0f, 0.0f,
        (Color){80, 85, 75, 255}, (Color){200, 200, 0, 255}
    });
    // Small ramp near crates
    ramps_.push_back(Ramp{
        {20, 0, -20}, 3.0f, 4.0f, 0.0f, 1.5f, 1.5708f,
        (Color){80, 85, 75, 255}, (Color){200, 200, 0, 255}
    });

    // === Pillars / Columns ===
    // Central area support pillars
    pillars_.push_back(Pillar{{-10, 0, -10}, 0.6f, 12, (Color){90, 90, 95, 255}, (Color){110, 110, 115, 255}});
    pillars_.push_back(Pillar{{10, 0, -10}, 0.6f, 12, (Color){90, 90, 95, 255}, (Color){110, 110, 115, 255}});
    pillars_.push_back(Pillar{{-10, 0, 10}, 0.6f, 12, (Color){90, 90, 95, 255}, (Color){110, 110, 115, 255}});
    pillars_.push_back(Pillar{{10, 0, 10}, 0.6f, 12, (Color){90, 90, 95, 255}, (Color){110, 110, 115, 255}});

    // Entrance pillars
    pillars_.push_back(Pillar{{-5, 0, -38}, 0.5f, 10, (Color){100, 95, 90, 255}, (Color){120, 115, 110, 255}});
    pillars_.push_back(Pillar{{5, 0, -38}, 0.5f, 10, (Color){100, 95, 90, 255}, (Color){120, 115, 110, 255}});
    pillars_.push_back(Pillar{{-5, 0, 38}, 0.5f, 10, (Color){100, 95, 90, 255}, (Color){120, 115, 110, 255}});
    pillars_.push_back(Pillar{{5, 0, 38}, 0.5f, 10, (Color){100, 95, 90, 255}, (Color){120, 115, 110, 255}});

    // Corner pillars
    pillars_.push_back(Pillar{{-35, 0, -35}, 0.8f, 12, (Color){85, 85, 90, 255}, (Color){105, 105, 110, 255}});
    pillars_.push_back(Pillar{{35, 0, -35}, 0.8f, 12, (Color){85, 85, 90, 255}, (Color){105, 105, 110, 255}});
    pillars_.push_back(Pillar{{-35, 0, 35}, 0.8f, 12, (Color){85, 85, 90, 255}, (Color){105, 105, 110, 255}});
    pillars_.push_back(Pillar{{35, 0, 35}, 0.8f, 12, (Color){85, 85, 90, 255}, (Color){105, 105, 110, 255}});

    // === Environmental Objects ===
    // Crates scattered around
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {-25, 0, -25}, 1.0f, 0.3f, (Color){140, 100, 50, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {25, 0, 25}, 1.0f, 0.8f, (Color){140, 100, 50, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {-30, 0, 15}, 1.2f, 0.5f, (Color){160, 110, 55, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {10, 0, -15}, 0.8f, 1.2f, (Color){130, 90, 45, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {30, 0, -20}, 1.0f, 2.0f, (Color){140, 100, 50, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {-10, 0, 30}, 1.1f, 0.7f, (Color){150, 105, 52, 255}});

    // Barrels (hazmat / oil drums)
    envObjects_.push_back(EnvObject{EnvObjectType::BARREL, {-20, 0, -15}, 1.0f, 0.0f, (Color){180, 50, 30, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::BARREL, {-19, 0, -13}, 1.0f, 0.0f, (Color){50, 100, 180, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::BARREL, {20, 0, 15}, 1.0f, 0.0f, (Color){180, 50, 30, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::BARREL, {35, 0, -10}, 1.0f, 0.5f, (Color){50, 120, 50, 255}});

    // Rocks / boulders (outdoor debris)
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_LARGE, {-35, 0, 10}, 1.5f, 0.4f, (Color){100, 100, 95, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_LARGE, {35, 0, -30}, 1.3f, 1.0f, (Color){105, 100, 90, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_SMALL, {-33, 0, 8}, 0.8f, 0.0f, (Color){110, 110, 105, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_SMALL, {37, 0, -28}, 0.7f, 2.0f, (Color){115, 110, 100, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_SMALL, {-15, 0, -35}, 0.9f, 1.5f, (Color){100, 100, 95, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_SMALL, {15, 0, 35}, 0.8f, 0.8f, (Color){100, 100, 95, 255}});

    // Sandbag walls (cover positions)
    envObjects_.push_back(EnvObject{EnvObjectType::SANDBAG_WALL, {-5, 0, -20}, 1.0f, 0.0f, (Color){160, 140, 90, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::SANDBAG_WALL, {5, 0, 20}, 1.0f, 1.5708f, (Color){160, 140, 90, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::SANDBAG_WALL, {20, 0, -5}, 1.0f, 0.7854f, (Color){155, 135, 85, 255}});

    // Metal barriers
    envObjects_.push_back(EnvObject{EnvObjectType::METAL_BARRIER, {-20, 0, -5}, 1.0f, 0.0f, (Color){200, 200, 50, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::METAL_BARRIER, {20, 0, 5}, 1.0f, 1.5708f, (Color){200, 200, 50, 255}});

    // Concrete barriers
    envObjects_.push_back(EnvObject{EnvObjectType::CONCRETE_BARRIER, {-10, 0, -25}, 1.0f, 0.3f, (Color){150, 150, 145, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::CONCRETE_BARRIER, {10, 0, 25}, 1.0f, 1.0f, (Color){150, 150, 145, 255}});

    // Tire stacks
    envObjects_.push_back(EnvObject{EnvObjectType::TIRE_STACK, {-28, 0, -18}, 1.0f, 0.0f, (Color){30, 30, 30, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::TIRE_STACK, {28, 0, 18}, 1.0f, 0.5f, (Color){35, 35, 35, 255}});

    // Pallets
    envObjects_.push_back(EnvObject{EnvObjectType::PALLET, {-22, 0, -8}, 1.0f, 0.2f, (Color){170, 140, 80, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::PALLET, {22, 0, 8}, 1.0f, 1.0f, (Color){170, 140, 80, 255}});

    // Generator (industrial)
    envObjects_.push_back(EnvObject{EnvObjectType::GENERATOR, {-35, 0, -20}, 1.2f, 0.0f, (Color){70, 80, 70, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::GENERATOR, {35, 0, 20}, 1.2f, 1.5708f, (Color){70, 80, 70, 255}});

    // Water tank
    envObjects_.push_back(EnvObject{EnvObjectType::WATER_TANK, {-33, 0, 25}, 1.5f, 0.0f, (Color){80, 100, 120, 255}});

    // Satellite dish
    envObjects_.push_back(EnvObject{EnvObjectType::SATELLITE_DISH, {33, 0, -25}, 1.3f, 0.5f, (Color){180, 180, 190, 255}});

    // === Neon Lights (atmospheric lighting) ===
    // Central platform ring light
    neonLights_.push_back(NeonLight{{-4, 0, -4}, {4, 0, -4}, 3.5f, (Color){0, 220, 220, 255}, 0.6f});
    neonLights_.push_back(NeonLight{{4, 0, -4}, {4, 0, 4}, 3.5f, (Color){0, 220, 220, 255}, 0.6f});
    neonLights_.push_back(NeonLight{{4, 0, 4}, {-4, 0, 4}, 3.5f, (Color){0, 220, 220, 255}, 0.6f});
    neonLights_.push_back(NeonLight{{-4, 0, 4}, {-4, 0, -4}, 3.5f, (Color){0, 220, 220, 255}, 0.6f});

    // Sniper tower lights
    neonLights_.push_back(NeonLight{{-2, 0, -32}, {2, 0, -32}, 5.5f, (Color){0, 255, 150, 255}, 0.8f});
    neonLights_.push_back(NeonLight{{2, 0, -32}, {2, 0, -28}, 5.5f, (Color){0, 255, 150, 255}, 0.8f});
    neonLights_.push_back(NeonLight{{2, 0, -28}, {-2, 0, -28}, 5.5f, (Color){0, 255, 150, 255}, 0.8f});
    neonLights_.push_back(NeonLight{{-2, 0, -28}, {-2, 0, -32}, 5.5f, (Color){0, 255, 150, 255}, 0.8f});

    // East guard post lights (orange warning)
    neonLights_.push_back(NeonLight{{28, 0, -2}, {32, 0, -2}, 4.5f, (Color){255, 150, 0, 255}, 0.7f});
    neonLights_.push_back(NeonLight{{32, 0, -2}, {32, 0, 2}, 4.5f, (Color){255, 150, 0, 255}, 0.7f});
    neonLights_.push_back(NeonLight{{32, 0, 2}, {28, 0, 2}, 4.5f, (Color){255, 150, 0, 255}, 0.7f});
    neonLights_.push_back(NeonLight{{28, 0, 2}, {28, 0, -2}, 4.5f, (Color){255, 150, 0, 255}, 0.7f});

    // West position lights (purple)
    neonLights_.push_back(NeonLight{{-32, 0, -7}, {-28, 0, -7}, 4.0f, (Color){150, 0, 255, 255}, 0.5f});
    neonLights_.push_back(NeonLight{{-28, 0, -7}, {-28, 0, -3}, 4.0f, (Color){150, 0, 255, 255}, 0.5f});
    neonLights_.push_back(NeonLight{{-28, 0, -3}, {-32, 0, -3}, 4.0f, (Color){150, 0, 255, 255}, 0.5f});
    neonLights_.push_back(NeonLight{{-32, 0, -3}, {-32, 0, -7}, 4.0f, (Color){150, 0, 255, 255}, 0.5f});

    // South walkway strip lights
    neonLights_.push_back(NeonLight{{-8, 0, 26.5f}, {8, 0, 26.5f}, 3.0f, (Color){0, 200, 200, 255}, 0.4f});

    // Wall-mounted neon strips
    neonLights_.push_back(NeonLight{{-38, 0, -30}, {-38, 0, -10}, 8.0f, (Color){0, 180, 180, 255}, 0.3f});
    neonLights_.push_back(NeonLight{{38, 0, 10}, {38, 0, 30}, 8.0f, (Color){0, 180, 180, 255}, 0.3f});
    neonLights_.push_back(NeonLight{{-20, 0, -38}, {20, 0, -38}, 6.0f, (Color){0, 180, 180, 255}, 0.3f});
    neonLights_.push_back(NeonLight{{-20, 0, 38}, {20, 0, 38}, 6.0f, (Color){0, 180, 180, 255}, 0.3f});

    // === Arches / Door Frames ===
    arches_.push_back(ArchStruct{{-15, 0, -5}, 3.0f, 5.0f, 0.5f, 1.5708f,
        (Color){100, 100, 105, 255}, (Color){0, 200, 200, 255}});
    arches_.push_back(ArchStruct{{15, 0, 5}, 3.0f, 5.0f, 0.5f, 1.5708f,
        (Color){100, 100, 105, 255}, (Color){0, 200, 200, 255}});
    arches_.push_back(ArchStruct{{0, 0, -38}, 4.0f, 6.0f, 0.5f, 0.0f,
        (Color){90, 90, 95, 255}, (Color){0, 255, 150, 255}});
    arches_.push_back(ArchStruct{{0, 0, 38}, 4.0f, 6.0f, 0.5f, 0.0f,
        (Color){90, 90, 95, 255}, (Color){0, 255, 150, 255}});

    // === Spawn Points ===
    spawnPositions_ = {
        {-35, 1, -35}, {35, 1, -35}, {-35, 1, 35}, {35, 1, 35},
        {0, 1, -35}, {0, 1, 35}, {-35, 1, 0}, {35, 1, 0}
    };

    enemySpawnPositions_ = {
        {-20, 1, -15}, {20, 1, -15}, {-20, 1, 15}, {20, 1, 15},
        {0, 4, 0}, {30, 1, -30}, {-30, 1, 30}, {10, 1, 25},
        {-25, 1, -5}, {15, 1, -25}, {-5, 1, 20}, {25, 1, 10}
    };

    coverPositions_ = {
        {-26, 1, -25}, {26, 1, 25}, {-30, 1, 16}, {11, 1, -15},
        {31, 1, -20}, {-10, 1, 31}
    };
}

// ============================================================================
// Desert Outpost
// ============================================================================

void Map::GenerateDesertOutpost() {
    bounds_ = {120, 15, 120};

    // Outer walls
    walls_.push_back(Wall{Vector3{-60, 0, -60}, Vector3{60, 0, -60}, 8, 1.0f, BEIGE});
    walls_.push_back(Wall{Vector3{-60, 0, 60}, Vector3{60, 0, 60}, 8, 1.0f, BEIGE});
    walls_.push_back(Wall{Vector3{-60, 0, -60}, Vector3{-60, 0, 60}, 8, 1.0f, BEIGE});
    walls_.push_back(Wall{Vector3{60, 0, -60}, Vector3{60, 0, 60}, 8, 1.0f, BEIGE});

    // Buildings
    walls_.push_back(Wall{Vector3{-30, 0, -30}, Vector3{-20, 0, -30}, 6, 8.0f, (Color){180, 160, 120, 255}});
    walls_.push_back(Wall{Vector3{20, 0, 20}, Vector3{30, 0, 20}, 6, 8.0f, (Color){180, 160, 120, 255}});
    walls_.push_back(Wall{Vector3{-10, 0, 10}, Vector3{10, 0, 10}, 4, 5.0f, (Color){170, 150, 110, 255}});

    // Elevated platforms (watchtowers)
    platforms_.push_back(Platform{
        {-45, 0, -45}, 4.0f, 4.0f, 6.0f, 0.5f,
        (Color){160, 140, 100, 255}, (Color){200, 180, 100, 255}, true
    });
    platforms_.push_back(Platform{
        {45, 0, 45}, 4.0f, 4.0f, 6.0f, 0.5f,
        (Color){160, 140, 100, 255}, (Color){200, 180, 100, 255}, true
    });

    // Stairs to watchtowers
    staircases_.push_back(Staircase{
        {-42, 0, -42}, 2.5f, 6.0f, 5.0f, 12, 0.7854f,
        (Color){140, 120, 80, 255}, (Color){180, 160, 80, 255}
    });
    staircases_.push_back(Staircase{
        {42, 0, 42}, 2.5f, 6.0f, 5.0f, 12, -2.3562f,
        (Color){140, 120, 80, 255}, (Color){180, 160, 80, 255}
    });

    // Pillars
    pillars_.push_back(Pillar{{-30, 0, 0}, 0.7f, 8, (Color){170, 150, 110, 255}, (Color){190, 170, 130, 255}});
    pillars_.push_back(Pillar{{30, 0, 0}, 0.7f, 8, (Color){170, 150, 110, 255}, (Color){190, 170, 130, 255}});

    // Desert environmental objects
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_LARGE, {-40, 0, 20}, 2.0f, 0.5f, (Color){160, 140, 100, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_LARGE, {40, 0, -20}, 1.8f, 1.0f, (Color){155, 135, 95, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_SMALL, {-25, 0, 35}, 1.0f, 0.3f, (Color){150, 130, 90, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::ROCK_SMALL, {25, 0, -35}, 1.2f, 1.5f, (Color){145, 125, 85, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::SANDBAG_WALL, {0, 0, -20}, 1.0f, 0.0f, (Color){170, 150, 100, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::SANDBAG_WALL, {0, 0, 20}, 1.0f, 1.5708f, (Color){170, 150, 100, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::BARREL, {-15, 0, -15}, 1.0f, 0.0f, (Color){50, 80, 50, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {15, 0, 15}, 1.2f, 0.5f, (Color){140, 100, 50, 255}});

    // Neon lights (desert amber)
    neonLights_.push_back(NeonLight{{-58, 0, -50}, {-58, 0, -30}, 7.0f, (Color){200, 180, 80, 255}, 0.3f});
    neonLights_.push_back(NeonLight{{58, 0, 30}, {58, 0, 50}, 7.0f, (Color){200, 180, 80, 255}, 0.3f});

    spawnPositions_ = {
        {-50, 1, -50}, {50, 1, -50}, {-50, 1, 50}, {50, 1, 50}
    };
    enemySpawnPositions_ = {
        {-25, 1, -25}, {25, 1, 25}, {0, 1, 0}, {-30, 1, 30}, {30, 1, -30}
    };
}

// ============================================================================
// Arctic Base
// ============================================================================

void Map::GenerateArcticBase() {
    bounds_ = {90, 15, 90};

    walls_.push_back(Wall{Vector3{-45, 0, -45}, Vector3{45, 0, -45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back(Wall{Vector3{-45, 0, 45}, Vector3{45, 0, 45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back(Wall{Vector3{-45, 0, -45}, Vector3{-45, 0, 45}, 10, 1.0f, LIGHTGRAY});
    walls_.push_back(Wall{Vector3{45, 0, -45}, Vector3{45, 0, 45}, 10, 1.0f, LIGHTGRAY});

    // Interior research sections
    walls_.push_back(Wall{Vector3{-20, 0, -15}, Vector3{-20, 0, 15}, 5, 0.5f, (Color){200, 210, 220, 255}});
    walls_.push_back(Wall{Vector3{20, 0, -15}, Vector3{20, 0, 15}, 5, 0.5f, (Color){200, 210, 220, 255}});

    // Elevated research platforms
    platforms_.push_back(Platform{
        {0, 0, -20}, 10.0f, 6.0f, 2.0f, 0.4f,
        (Color){180, 190, 200, 255}, (Color){100, 200, 255, 255}, true
    });
    platforms_.push_back(Platform{
        {0, 0, 20}, 10.0f, 6.0f, 2.0f, 0.4f,
        (Color){180, 190, 200, 255}, (Color){100, 200, 255, 255}, true
    });

    staircases_.push_back(Staircase{
        {-3, 0, -16}, 3.0f, 2.0f, 3.0f, 4, 0.0f,
        (Color){170, 180, 190, 255}, (Color){80, 180, 230, 255}
    });
    staircases_.push_back(Staircase{
        {3, 0, 16}, 3.0f, 2.0f, 3.0f, 4, 3.14159f,
        (Color){170, 180, 190, 255}, (Color){80, 180, 230, 255}
    });

    // Pillars
    pillars_.push_back(Pillar{{-15, 0, -15}, 0.5f, 10, (Color){190, 200, 210, 255}, (Color){210, 220, 230, 255}});
    pillars_.push_back(Pillar{{15, 0, -15}, 0.5f, 10, (Color){190, 200, 210, 255}, (Color){210, 220, 230, 255}});
    pillars_.push_back(Pillar{{-15, 0, 15}, 0.5f, 10, (Color){190, 200, 210, 255}, (Color){210, 220, 230, 255}});
    pillars_.push_back(Pillar{{15, 0, 15}, 0.5f, 10, (Color){190, 200, 210, 255}, (Color){210, 220, 230, 255}});

    // Arctic environmental objects
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {-30, 0, -10}, 1.0f, 0.3f, (Color){180, 190, 200, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::CRATE, {30, 0, 10}, 1.0f, 0.8f, (Color){180, 190, 200, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::METAL_BARRIER, {-10, 0, -30}, 1.0f, 0.0f, (Color){200, 200, 200, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::GENERATOR, {-35, 0, -30}, 1.2f, 0.0f, (Color){60, 80, 90, 255}});
    envObjects_.push_back(EnvObject{EnvObjectType::WATER_TANK, {35, 0, 30}, 1.5f, 0.0f, (Color){100, 120, 140, 255}});

    // Cold blue neon lights
    neonLights_.push_back(NeonLight{{-43, 0, -30}, {-43, 0, -10}, 8.0f, (Color){100, 200, 255, 255}, 0.4f});
    neonLights_.push_back(NeonLight{{43, 0, 10}, {43, 0, 30}, 8.0f, (Color){100, 200, 255, 255}, 0.4f});
    neonLights_.push_back(NeonLight{{-10, 0, -43}, {10, 0, -43}, 5.0f, (Color){100, 200, 255, 255}, 0.5f});

    spawnPositions_ = {
        {-40, 1, -40}, {40, 1, -40}, {-40, 1, 40}, {40, 1, 40}
    };
    enemySpawnPositions_ = {
        {-20, 1, -20}, {20, 1, 20}, {0, 1, 0}, {-25, 1, 25}, {25, 1, -25}
    };
}

// ============================================================================
// Update
// ============================================================================

void Map::Update(float deltaTime) {
    animTime_ += deltaTime;

    for (auto& pickup : pickups_) {
        if (!pickup.active) {
            pickup.respawnTimer -= deltaTime;
            if (pickup.respawnTimer <= 0) {
                pickup.active = true;
            }
        }
    }
}

// ============================================================================
// Main Render
// ============================================================================

void Map::Render() {
    RenderSkybox();
    RenderFloor();
    RenderWalls();
    RenderPlatforms();
    RenderRamps();
    RenderStaircases();
    RenderPillars();
    RenderArches();
    RenderEnvObjects();
    RenderNeonLights();
}

// ============================================================================
// Skybox - Layered gradient sky with atmosphere
// ============================================================================

void Map::RenderSkybox() const {
    // Draw sky as layered horizontal strips for a proper gradient effect
    float skyHeight = 50.0f;
    float skyRadius = 180.0f;
    int strips = 20;

    Color topColor = {5, 5, 25, 255};         // Deep space navy
    Color midColor = {15, 20, 55, 255};        // Dark blue
    Color horizonColor = {35, 40, 70, 255};    // Blue-grey horizon
    Color groundHaze = {25, 25, 35, 255};      // Ground fog

    for (int i = 0; i < strips; i++) {
        float t = (float)i / strips;
        float y = skyHeight - t * skyHeight * 2.0f;
        float stripH = skyHeight * 2.0f / strips;

        Color c;
        if (t < 0.3f) {
            float lt = t / 0.3f;
            c.r = (uint8_t)(topColor.r + (midColor.r - topColor.r) * lt);
            c.g = (uint8_t)(topColor.g + (midColor.g - topColor.g) * lt);
            c.b = (uint8_t)(topColor.b + (midColor.b - topColor.b) * lt);
            c.a = 255;
        } else if (t < 0.6f) {
            float lt = (t - 0.3f) / 0.3f;
            c.r = (uint8_t)(midColor.r + (horizonColor.r - midColor.r) * lt);
            c.g = (uint8_t)(midColor.g + (horizonColor.g - midColor.g) * lt);
            c.b = (uint8_t)(midColor.b + (horizonColor.b - midColor.b) * lt);
            c.a = 255;
        } else {
            float lt = (t - 0.6f) / 0.4f;
            c.r = (uint8_t)(horizonColor.r + (groundHaze.r - horizonColor.r) * lt);
            c.g = (uint8_t)(horizonColor.g + (groundHaze.g - horizonColor.g) * lt);
            c.b = (uint8_t)(horizonColor.b + (groundHaze.b - horizonColor.b) * lt);
            c.a = 255;
        }

        // Draw as a very large ring of quads around the scene
        float rad = skyRadius;
        DrawPlane({0, y, 0}, {rad * 2, rad * 2}, Fade(c, 0.04f));
    }

    // Subtle atmospheric haze near ground
    DrawPlane({0, 0.3f, 0}, {bounds_.x * 3, bounds_.z * 3}, Fade((Color){20, 25, 35, 255}, 0.08f));
}

// ============================================================================
// Floor - Multi-section floor with color zones and detailed grid
// ============================================================================

void Map::RenderFloor() const {
    // Main ground plane - textured
    Texture2D floorTex = GetFloorTexture();
    DrawTexturedPlane(floorTex, {0, 0, 0}, {bounds_.x * 2, bounds_.z * 2},
                      (Color){200, 200, 200, 255}, 0.25f);

    // === Section-based floor coloring with textures ===
    // Central zone - slightly lighter tint
    DrawTexturedPlane(floorTex, {0, 0.01f, 0}, {20, 20},
                      (Color){220, 220, 220, 255}, 0.25f);

    // Loading dock area (southwest) - concrete tint
    DrawTexturedPlane(texConcrete_, {-25, 0.01f, 20}, {16, 16},
                      (Color){180, 180, 180, 255}, 0.3f);

    // East guard area - metallic tint
    DrawTexturedPlane(texMetal_, {30, 0.01f, 0}, {12, 12},
                      (Color){160, 165, 175, 255}, 0.3f);

    // === Neon-style grid lines ===
    Color gridColorDim = {0, 80, 80, 255};
    Color gridColorBright = {0, 220, 220, 255};

    // Major grid lines every 10 units
    for (int i = -(int)bounds_.x; i <= (int)bounds_.x; i += 10) {
        DrawLine3D({(float)i, 0.02f, -bounds_.z}, {(float)i, 0.02f, bounds_.z}, Fade(gridColorDim, 0.15f));
        DrawLine3D({(float)i, 0.03f, -bounds_.z}, {(float)i, 0.03f, bounds_.z}, Fade(gridColorBright, 0.1f));
        DrawLine3D({-bounds_.x, 0.02f, (float)i}, {bounds_.x, 0.02f, (float)i}, Fade(gridColorDim, 0.15f));
        DrawLine3D({-bounds_.x, 0.03f, (float)i}, {bounds_.x, 0.03f, (float)i}, Fade(gridColorBright, 0.1f));
    }

    // Minor grid lines every 5 units
    Color minorGridColor = {0, 50, 50, 255};
    for (int i = -(int)bounds_.x; i <= (int)bounds_.x; i += 5) {
        if (i % 10 == 0) continue;
        DrawLine3D({(float)i, 0.01f, -bounds_.z}, {(float)i, 0.01f, bounds_.z}, Fade(minorGridColor, 0.07f));
        DrawLine3D({-bounds_.x, 0.01f, (float)i}, {bounds_.x, 0.01f, (float)i}, Fade(minorGridColor, 0.07f));
    }

    // === Floor zone border lines ===
    Color zoneColor = {0, 150, 150, 255};
    // Central zone border
    float cz = 10.0f;
    DrawLine3D({-cz, 0.04f, -cz}, {cz, 0.04f, -cz}, Fade(zoneColor, 0.2f));
    DrawLine3D({cz, 0.04f, -cz}, {cz, 0.04f, cz}, Fade(zoneColor, 0.2f));
    DrawLine3D({cz, 0.04f, cz}, {-cz, 0.04f, cz}, Fade(zoneColor, 0.2f));
    DrawLine3D({-cz, 0.04f, cz}, {-cz, 0.04f, -cz}, Fade(zoneColor, 0.2f));
}

// ============================================================================
// Walls - Enhanced with sci-fi edge highlights and details
// ============================================================================

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

        // Main wall cube - textured with brick/concrete/sand
        Texture2D wallTex = GetWallTexture();
        DrawTexturedBox(wallTex, center, length, wall.height, wall.thickness,
                        (Color){220, 220, 220, 255}, 0.5f);

        // === Sci-fi edge highlights ===
        Color edgeColor = {0, 180, 180, 255};
        float topY = wall.height;

        // Top edge
        DrawLine3D({wall.start.x, topY, wall.start.z}, {wall.end.x, topY, wall.end.z}, Fade(edgeColor, 0.25f));
        // Bottom edge glow
        DrawLine3D({wall.start.x, 0.05f, wall.start.z}, {wall.end.x, 0.05f, wall.end.z}, Fade(edgeColor, 0.15f));
        // Vertical edges
        DrawLine3D({wall.start.x, 0.05f, wall.start.z}, {wall.start.x, topY, wall.start.z}, Fade(edgeColor, 0.15f));
        DrawLine3D({wall.end.x, 0.05f, wall.end.z}, {wall.end.x, topY, wall.end.z}, Fade(edgeColor, 0.15f));

        // === Wall detail strips (horizontal lines at 1/3 and 2/3 height) ===
        Color stripColor = {0, 100, 100, 255};
        float y1 = wall.height * 0.33f;
        float y2 = wall.height * 0.66f;
        DrawLine3D({wall.start.x, y1, wall.start.z}, {wall.end.x, y1, wall.end.z}, Fade(stripColor, 0.1f));
        DrawLine3D({wall.start.x, y2, wall.start.z}, {wall.end.x, y2, wall.end.z}, Fade(stripColor, 0.1f));
    }
}

// ============================================================================
// Platforms - Elevated surfaces with neon trim and railings
// ============================================================================

void Map::RenderPlatforms() const {
    for (const auto& plat : platforms_) {
        float topY = plat.elevation;

        // Platform body - textured concrete
        Vector3 center = {plat.center.x, topY - plat.thickness * 0.5f, plat.center.z};
        DrawTexturedBox(texConcrete_, center, plat.width, plat.thickness, plat.depth,
                        (Color){200, 200, 200, 255}, 0.3f);

        // Top surface highlight (textured)
        Color topColor = {
            (uint8_t)std::min(plat.color.r + 40, 255),
            (uint8_t)std::min(plat.color.g + 40, 255),
            (uint8_t)std::min(plat.color.b + 40, 255),
            255
        };
        DrawTexturedPlane(texConcrete_, {plat.center.x, topY + 0.01f, plat.center.z},
                          {plat.width, plat.depth}, topColor, 0.3f);

        // Neon edge trim
        float hw = plat.width * 0.5f;
        float hd = plat.depth * 0.5f;
        float pulse = sinf(animTime_ * 2.0f) * 0.15f + 0.85f;

        // Four edges of the platform top
        DrawLine3D({plat.center.x - hw, topY + 0.05f, plat.center.z - hd},
                   {plat.center.x + hw, topY + 0.05f, plat.center.z - hd},
                   Fade(plat.edgeColor, 0.6f * pulse));
        DrawLine3D({plat.center.x + hw, topY + 0.05f, plat.center.z - hd},
                   {plat.center.x + hw, topY + 0.05f, plat.center.z + hd},
                   Fade(plat.edgeColor, 0.6f * pulse));
        DrawLine3D({plat.center.x + hw, topY + 0.05f, plat.center.z + hd},
                   {plat.center.x - hw, topY + 0.05f, plat.center.z + hd},
                   Fade(plat.edgeColor, 0.6f * pulse));
        DrawLine3D({plat.center.x - hw, topY + 0.05f, plat.center.z + hd},
                   {plat.center.x - hw, topY + 0.05f, plat.center.z - hd},
                   Fade(plat.edgeColor, 0.6f * pulse));

        // Glow on platform surface
        DrawPlane({plat.center.x, topY + 0.02f, plat.center.z}, {plat.width + 1, plat.depth + 1},
                  Fade(plat.edgeColor, 0.03f * pulse));

        // Railing posts (if has railing)
        if (plat.hasRailing) {
            float railHeight = 1.0f;
            Color railColor = Fade(plat.edgeColor, 0.4f);

            // Corner posts
            Vector3 corners[4] = {
                {plat.center.x - hw, topY, plat.center.z - hd},
                {plat.center.x + hw, topY, plat.center.z - hd},
                {plat.center.x + hw, topY, plat.center.z + hd},
                {plat.center.x - hw, topY, plat.center.z + hd}
            };

            for (int i = 0; i < 4; i++) {
                // Vertical post
                DrawLine3D(corners[i],
                    {corners[i].x, corners[i].y + railHeight, corners[i].z}, railColor);
                // Top rail connecting to next corner
                DrawLine3D(
                    {corners[i].x, corners[i].y + railHeight, corners[i].z},
                    {corners[(i + 1) % 4].x, corners[(i + 1) % 4].y + railHeight, corners[(i + 1) % 4].z},
                    railColor);
                // Mid rail
                DrawLine3D(
                    {corners[i].x, corners[i].y + railHeight * 0.5f, corners[i].z},
                    {corners[(i + 1) % 4].x, corners[(i + 1) % 4].y + railHeight * 0.5f, corners[(i + 1) % 4].z},
                    Fade(plat.edgeColor, 0.2f));
            }

            // Intermediate posts along edges
            for (int i = 0; i < 4; i++) {
                Vector3 c1 = corners[i];
                Vector3 c2 = corners[(i + 1) % 4];
                int numPosts = 3;
                for (int p = 1; p < numPosts; p++) {
                    float t = (float)p / numPosts;
                    Vector3 post = {
                        c1.x + (c2.x - c1.x) * t,
                        topY,
                        c1.z + (c2.z - c1.z) * t
                    };
                    DrawLine3D(post, {post.x, post.y + railHeight, post.z}, Fade(plat.edgeColor, 0.2f));
                }
            }
        }

        // Platform underside shadow (darker plane below)
        DrawPlane({plat.center.x, topY - plat.thickness, plat.center.z},
                  {plat.width + 2, plat.depth + 2}, Fade(BLACK, 0.15f));
    }
}

// ============================================================================
// Ramps - Angled surfaces
// ============================================================================

void Map::RenderRamps() const {
    for (const auto& ramp : ramps_) {
        float cosA = cosf(ramp.angle);
        float sinA = sinf(ramp.angle);

        // Draw ramp as a series of thin steps for visual clarity
        int segments = 12;
        for (int i = 0; i < segments; i++) {
            float t0 = (float)i / segments;
            float t1 = (float)(i + 1) / segments;

            float y0 = ramp.baseHeight + (ramp.topHeight - ramp.baseHeight) * t0;
            float y1 = ramp.baseHeight + (ramp.topHeight - ramp.baseHeight) * t1;

            // Position along ramp direction
            float z0 = -ramp.length * 0.5f + ramp.length * t0;
            float z1 = -ramp.length * 0.5f + ramp.length * t1;

            // Rotate around Y
            float rx0 = z0 * sinA;
            float rz0 = z0 * cosA;
            float rx1 = z1 * sinA;
            float rz1 = z1 * cosA;

            Vector3 pos = {
                ramp.baseCenter.x + (rx0 + rx1) * 0.5f,
                (y0 + y1) * 0.5f,
                ramp.baseCenter.z + (rz0 + rz1) * 0.5f
            };

            float stepHeight = y1 - y0;
            float stepDepth = ramp.length / segments;
            DrawTexturedBox(texConcrete_, pos, ramp.width, std::max(stepHeight, 0.05f), stepDepth + 0.02f,
                            (Color){200, 200, 200, 255}, 0.3f);
        }

        // Edge highlights
        float pulse = sinf(animTime_ * 1.5f) * 0.1f + 0.9f;
        Vector3 baseStart = {
            ramp.baseCenter.x - ramp.width * 0.5f * cosA,
            ramp.baseHeight,
            ramp.baseCenter.z + ramp.width * 0.5f * sinA
        };
        Vector3 baseEnd = {
            ramp.baseCenter.x + ramp.width * 0.5f * cosA,
            ramp.baseHeight,
            ramp.baseCenter.z - ramp.width * 0.5f * sinA
        };
        Vector3 topStart = {
            baseStart.x + ramp.length * sinA,
            ramp.topHeight,
            baseStart.z + ramp.length * cosA
        };
        Vector3 topEnd = {
            baseEnd.x + ramp.length * sinA,
            ramp.topHeight,
            baseEnd.z + ramp.length * cosA
        };

        DrawLine3D(baseStart, baseEnd, Fade(ramp.edgeColor, 0.4f * pulse));
        DrawLine3D(topStart, topEnd, Fade(ramp.edgeColor, 0.4f * pulse));
        DrawLine3D(baseStart, topStart, Fade(ramp.edgeColor, 0.2f * pulse));
        DrawLine3D(baseEnd, topEnd, Fade(ramp.edgeColor, 0.2f * pulse));
    }
}

// ============================================================================
// Staircases - Step-by-step with neon riser highlights
// ============================================================================

void Map::RenderStaircases() const {
    for (const auto& stair : staircases_) {
        float cosR = cosf(stair.rotation);
        float sinR = sinf(stair.rotation);
        float stepHeight = stair.totalHeight / stair.numSteps;
        float stepDepth = stair.totalDepth / stair.numSteps;

        for (int i = 0; i < stair.numSteps; i++) {
            float t = (float)i / stair.numSteps;
            float y = stepHeight * i;
            float z = -stair.totalDepth * 0.5f + stepDepth * (i + 0.5f);

            // Rotate step position
            float rx = z * sinR;
            float rz = z * cosR;

            Vector3 stepPos = {
                stair.position.x + rx,
                stair.position.y + y + stepHeight * 0.5f,
                stair.position.z + rz
            };

            // Draw step - textured
            DrawTexturedBox(texStair_, stepPos, stair.width, stepHeight, stepDepth,
                            (Color){210, 210, 215, 255}, 0.4f);

            // Riser highlight (front face of each step)
            Vector3 riserStart = {
                stepPos.x - stair.width * 0.5f * cosR,
                stepPos.y + stepHeight * 0.5f,
                stepPos.z + stair.width * 0.5f * sinR
            };
            Vector3 riserEnd = {
                stepPos.x + stair.width * 0.5f * cosR,
                stepPos.y + stepHeight * 0.5f,
                stepPos.z - stair.width * 0.5f * sinR
            };

            float pulse = sinf(animTime_ * 2.0f + (float)i * 0.3f) * 0.15f + 0.85f;
            DrawLine3D(riserStart, riserEnd, Fade(stair.riserColor, 0.3f * pulse));
        }

        // Side rails
        Vector3 bottomLeft = {
            stair.position.x - stair.width * 0.5f * cosR,
            stair.position.y,
            stair.position.z + stair.width * 0.5f * sinR
        };
        Vector3 topLeft = {
            bottomLeft.x + stair.totalDepth * sinR,
            stair.position.y + stair.totalHeight,
            bottomLeft.z + stair.totalDepth * cosR
        };
        Vector3 bottomRight = {
            stair.position.x + stair.width * 0.5f * cosR,
            stair.position.y,
            stair.position.z - stair.width * 0.5f * sinR
        };
        Vector3 topRight = {
            bottomRight.x + stair.totalDepth * sinR,
            stair.position.y + stair.totalHeight,
            bottomRight.z + stair.totalDepth * cosR
        };

        Color railColor = Fade(stair.riserColor, 0.3f);
        DrawLine3D(bottomLeft, topLeft, railColor);
        DrawLine3D(bottomRight, topRight, railColor);
    }
}

// ============================================================================
// Pillars - Cylindrical columns with cap details
// ============================================================================

void Map::RenderPillars() const {
    for (const auto& pillar : pillars_) {
        // Main cylinder body - textured concrete
        DrawTexturedCylinder(texConcrete_, pillar.base, pillar.radius, pillar.height, 12,
                             (Color){200, 200, 205, 255}, 1.0f, 0.5f);

        // Top cap (slightly wider) - textured
        Vector3 topPos = {pillar.base.x, pillar.base.y + pillar.height, pillar.base.z};
        DrawTexturedCylinder(texConcrete_, topPos, pillar.radius * 1.15f, 0.2f, 12,
                             (Color){190, 190, 195, 255}, 1.0f, 0.3f);

        // Bottom base (slightly wider) - textured
        DrawTexturedCylinder(texConcrete_, pillar.base, pillar.radius * 1.15f, 0.2f, 12,
                             (Color){190, 190, 195, 255}, 1.0f, 0.3f);

        // Neon ring at mid-height
        float midY = pillar.base.y + pillar.height * 0.5f;
        float ringRadius = pillar.radius + 0.05f;
        Color ringColor = {0, 200, 200, 255};
        int segments = 16;
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i * 2.0f * PI / segments;
            float a2 = (float)(i + 1) * 2.0f * PI / segments;
            Vector3 p1 = {pillar.base.x + cosf(a1) * ringRadius, midY, pillar.base.z + sinf(a1) * ringRadius};
            Vector3 p2 = {pillar.base.x + cosf(a2) * ringRadius, midY, pillar.base.z + sinf(a2) * ringRadius};
            DrawLine3D(p1, p2, Fade(ringColor, 0.2f));
        }

        // Vertical edge lines
        for (int i = 0; i < 8; i++) {
            float a = (float)i * PI * 0.25f;
            float ex = pillar.base.x + cosf(a) * pillar.radius;
            float ez = pillar.base.z + sinf(a) * pillar.radius;
            DrawLine3D({ex, pillar.base.y, ez}, {ex, topPos.y, ez}, Fade(ringColor, 0.05f));
        }
    }
}

// ============================================================================
// Arches - Door frame structures with neon trim
// ============================================================================

void Map::RenderArches() const {
    for (const auto& arch : arches_) {
        float cosR = cosf(arch.rotation);
        float sinR = sinf(arch.rotation);

        // Left pillar of arch
        Vector3 leftBase = {
            arch.position.x - arch.width * 0.5f * cosR,
            arch.position.y,
            arch.position.z + arch.width * 0.5f * sinR
        };
        Vector3 leftTop = {
            leftBase.x,
            arch.position.y + arch.height,
            leftBase.z
        };

        // Right pillar
        Vector3 rightBase = {
            arch.position.x + arch.width * 0.5f * cosR,
            arch.position.y,
            arch.position.z - arch.width * 0.5f * sinR
        };
        Vector3 rightTop = {
            rightBase.x,
            arch.position.y + arch.height,
            rightBase.z
        };

        // Draw pillars as thin cubes - textured concrete
        Vector3 leftCenter = {(leftBase.x + leftTop.x) * 0.5f, arch.height * 0.5f, (leftBase.z + leftTop.z) * 0.5f};
        Vector3 rightCenter = {(rightBase.x + rightTop.x) * 0.5f, arch.height * 0.5f, (rightBase.z + rightTop.z) * 0.5f};

        DrawTexturedBox(texConcrete_, leftCenter, arch.depth, arch.height, arch.depth,
                        (Color){200, 200, 205, 255}, 0.3f);
        DrawTexturedBox(texConcrete_, rightCenter, arch.depth, arch.height, arch.depth,
                        (Color){200, 200, 205, 255}, 0.3f);

        // Top beam - textured
        Vector3 topCenter = {
            arch.position.x,
            arch.position.y + arch.height - arch.depth * 0.5f,
            arch.position.z
        };
        DrawTexturedBox(texConcrete_, topCenter, arch.width + arch.depth, arch.depth, arch.depth,
                        (Color){190, 190, 195, 255}, 0.3f);

        // Neon trim along the arch outline
        float pulse = sinf(animTime_ * 2.5f) * 0.15f + 0.85f;
        DrawLine3D(leftBase, leftTop, Fade(arch.trimColor, 0.4f * pulse));
        DrawLine3D(rightBase, rightTop, Fade(arch.trimColor, 0.4f * pulse));
        DrawLine3D(leftTop, rightTop, Fade(arch.trimColor, 0.5f * pulse));

        // Inner arch curve (approximated with line segments)
        int curveSteps = 8;
        for (int i = 0; i < curveSteps; i++) {
            float t1 = (float)i / curveSteps;
            float t2 = (float)(i + 1) / curveSteps;

            // Parabolic arch curve
            float x1 = arch.width * 0.5f * (1.0f - 2.0f * t1);
            float x2 = arch.width * 0.5f * (1.0f - 2.0f * t2);
            float y1 = arch.height - arch.depth + sinf(t1 * PI) * arch.depth * 2.0f;
            float y2 = arch.height - arch.depth + sinf(t2 * PI) * arch.depth * 2.0f;

            Vector3 p1 = {
                arch.position.x + x1 * cosR,
                arch.position.y + y1,
                arch.position.z - x1 * sinR
            };
            Vector3 p2 = {
                arch.position.x + x2 * cosR,
                arch.position.y + y2,
                arch.position.z - x2 * sinR
            };
            DrawLine3D(p1, p2, Fade(arch.trimColor, 0.3f * pulse));
        }
    }
}

// ============================================================================
// Environmental Objects - Dispatch to specific renderers
// ============================================================================

void Map::RenderEnvObjects() const {
    for (const auto& obj : envObjects_) {
        switch (obj.type) {
        case EnvObjectType::CRATE:            RenderCrate(obj); break;
        case EnvObjectType::BARREL:           RenderBarrel(obj); break;
        case EnvObjectType::ROCK_LARGE:       RenderRockLarge(obj); break;
        case EnvObjectType::ROCK_SMALL:       RenderRockSmall(obj); break;
        case EnvObjectType::SANDBAG_WALL:     RenderSandbagWall(obj); break;
        case EnvObjectType::METAL_BARRIER:    RenderMetalBarrier(obj); break;
        case EnvObjectType::CONCRETE_BARRIER: RenderConcreteBarrier(obj); break;
        case EnvObjectType::TIRE_STACK:       RenderTireStack(obj); break;
        case EnvObjectType::PALLET:           RenderPallet(obj); break;
        case EnvObjectType::GENERATOR:        RenderGenerator(obj); break;
        case EnvObjectType::WATER_TANK:       RenderWaterTank(obj); break;
        case EnvObjectType::SATELLITE_DISH:   RenderSatelliteDish(obj); break;
        }
    }
}

// --- Crate ---
void Map::RenderCrate(const EnvObject& obj) const {
    float s = obj.scale;

    Vector3 center = {obj.position.x, s * 0.5f, obj.position.z};
    DrawTexturedBox(texCrate_, center, s, s, s, (Color){210, 210, 200, 255}, 0.5f);

    // Cross pattern on front face
    Vector3 crossC = {obj.position.x, s * 0.5f, obj.position.z + s * 0.51f};
    Color crossColor = {(uint8_t)std::min(obj.color.r + 40, 255),
                        (uint8_t)std::min(obj.color.g + 30, 255),
                        (uint8_t)std::min(obj.color.b + 10, 255), 255};
    DrawLine3D({crossC.x - s * 0.4f, crossC.y - s * 0.4f, crossC.z},
               {crossC.x + s * 0.4f, crossC.y + s * 0.4f, crossC.z}, crossColor);
    DrawLine3D({crossC.x - s * 0.4f, crossC.y + s * 0.4f, crossC.z},
               {crossC.x + s * 0.4f, crossC.y - s * 0.4f, crossC.z}, crossColor);

    // Edge wires
    Color edgeColor = {0, 120, 120, 255};
    float hs = s * 0.5f;
    // Top face edges
    DrawLine3D({center.x - hs, s, center.z - hs}, {center.x + hs, s, center.z - hs}, Fade(edgeColor, 0.15f));
    DrawLine3D({center.x + hs, s, center.z - hs}, {center.x + hs, s, center.z + hs}, Fade(edgeColor, 0.15f));
    DrawLine3D({center.x + hs, s, center.z + hs}, {center.x - hs, s, center.z + hs}, Fade(edgeColor, 0.15f));
    DrawLine3D({center.x - hs, s, center.z + hs}, {center.x - hs, s, center.z - hs}, Fade(edgeColor, 0.15f));
}

// --- Barrel ---
void Map::RenderBarrel(const EnvObject& obj) const {
    float s = obj.scale;
    float height = s * 1.2f;
    float radius = s * 0.35f;

    // Main cylinder - textured
    DrawTexturedCylinder(texBarrel_, obj.position, radius, height, 12,
                         (Color){200, 200, 200, 255}, 1.0f, 0.5f);

    // Top rim
    DrawCylinder({obj.position.x, height, obj.position.z}, radius * 1.05f, radius * 1.05f, 0.05f, 12, DARKGRAY);

    // Hazard stripe (ring at mid-height)
    Color stripeColor = (obj.color.r > 100) ? (Color){200, 200, 0, 255} : (Color){200, 50, 0, 255};
    float midY = height * 0.5f;
    int segments = 12;
    for (int i = 0; i < segments; i++) {
        float a1 = (float)i * 2.0f * PI / segments;
        float a2 = (float)(i + 1) * 2.0f * PI / segments;
        Vector3 p1 = {obj.position.x + cosf(a1) * radius, midY, obj.position.z + sinf(a1) * radius};
        Vector3 p2 = {obj.position.x + cosf(a2) * radius, midY, obj.position.z + sinf(a2) * radius};
        DrawLine3D(p1, p2, Fade(stripeColor, 0.3f));
    }
}

// --- Large Rock ---
void Map::RenderRockLarge(const EnvObject& obj) const {
    float s = obj.scale;
    // Main body - textured stone
    DrawTexturedBox(texStone_, {obj.position.x, s * 0.6f, obj.position.z},
                    s * 1.4f, s * 1.2f, s * 1.4f, (Color){190, 190, 185, 255}, 0.5f);
    // Bumps
    DrawSphere({obj.position.x + s * 0.3f, s * 0.4f, obj.position.z + s * 0.2f}, s * 0.35f,
               (Color){(uint8_t)std::min(obj.color.r + 10, 255),
                       (uint8_t)std::min(obj.color.g + 10, 255),
                       (uint8_t)std::min(obj.color.b + 5, 255), 255});
    DrawSphere({obj.position.x - s * 0.25f, s * 0.8f, obj.position.z - s * 0.15f}, s * 0.3f,
               (Color){(uint8_t)std::max(obj.color.r - 10, 0),
                       (uint8_t)std::max(obj.color.g - 10, 0),
                       (uint8_t)std::max(obj.color.b - 5, 0), 255});
    // Shadow beneath
    DrawPlane({obj.position.x, 0.02f, obj.position.z}, {s * 2, s * 2}, Fade(BLACK, 0.1f));
}

// --- Small Rock ---
void Map::RenderRockSmall(const EnvObject& obj) const {
    float s = obj.scale;
    DrawTexturedBox(texStone_, {obj.position.x, s * 0.3f, obj.position.z},
                    s * 0.7f, s * 0.6f, s * 0.7f, (Color){195, 195, 190, 255}, 0.5f);
    // Small secondary bump
    DrawSphere({obj.position.x + s * 0.2f, s * 0.15f, obj.position.z - s * 0.1f}, s * 0.2f,
               (Color){(uint8_t)std::min(obj.color.r + 15, 255),
                       (uint8_t)std::min(obj.color.g + 15, 255),
                       (uint8_t)std::min(obj.color.b + 10, 255), 255});
}

// --- Sandbag Wall ---
void Map::RenderSandbagWall(const EnvObject& obj) const {
    float s = obj.scale;
    float cosR = cosf(obj.rotation);
    float sinR = sinf(obj.rotation);

    // Stack of sandbags
    int rows = 3;
    int cols = 4;
    float bagW = s * 0.5f;
    float bagH = s * 0.2f;
    float bagD = s * 0.3f;

    for (int row = 0; row < rows; row++) {
        int numInRow = cols - row;
        for (int col = 0; col < numInRow; col++) {
            float offsetX = (col - numInRow * 0.5f + 0.5f) * bagW;
            float offsetY = row * bagH + bagH * 0.5f;
            float offsetZ = row * bagD * 0.3f; // Slight back-set per row

            // Rotate offset
            float rx = offsetX * cosR - offsetZ * sinR;
            float rz = offsetX * sinR + offsetZ * cosR;

            Vector3 bagPos = {
                obj.position.x + rx,
                offsetY,
                obj.position.z + rz
            };

            Color bagColor = (row % 2 == 0) ? obj.color :
                (Color){(uint8_t)std::min(obj.color.r + 15, 255),
                        (uint8_t)std::min(obj.color.g + 10, 255),
                        (uint8_t)std::min(obj.color.b + 5, 255), 255};
            DrawTexturedBox(texSand_, bagPos, bagW * 0.9f, bagH, bagD,
                            (Color){210, 200, 160, 255}, 0.5f);
        }
    }
}

// --- Metal Barrier ---
void Map::RenderMetalBarrier(const EnvObject& obj) const {
    float s = obj.scale;
    float cosR = cosf(obj.rotation);
    float sinR = sinf(obj.rotation);

    // Jersey barrier shape - trapezoidal
    float height = s * 0.8f;
    float topW = s * 0.3f;
    float botW = s * 0.6f;
    float length = s * 1.5f;

    // Main body - textured metal
    Vector3 center = {obj.position.x, height * 0.5f, obj.position.z};
    DrawTexturedBox(texMetal_, center, length, height, botW,
                    (Color){200, 200, 50, 255}, 0.4f);

    // Top cap (narrower) - textured
    DrawTexturedBox(texMetal_, {center.x, height, center.z}, length, 0.1f, topW,
                    (Color){180, 180, 180, 255}, 0.4f);

    // Yellow/black hazard stripes
    Color stripeColor = {50, 50, 50, 255};
    float stripeW = length * 0.15f;
    for (int i = 0; i < 4; i++) {
        float sx = center.x - length * 0.5f + stripeW * (2 * i + 1);
        DrawCube({sx, height * 0.5f, center.z + botW * 0.51f}, stripeW, height * 0.8f, 0.02f, stripeColor);
    }

    // Reflective strip
    Color refColor = {200, 200, 200, 255};
    float pulse = sinf(animTime_ * 3.0f) * 0.2f + 0.8f;
    DrawLine3D({center.x - length * 0.5f, height * 0.6f, center.z + botW * 0.51f},
               {center.x + length * 0.5f, height * 0.6f, center.z + botW * 0.51f},
               Fade(refColor, 0.3f * pulse));
}

// --- Concrete Barrier ---
void Map::RenderConcreteBarrier(const EnvObject& obj) const {
    float s = obj.scale;
    DrawTexturedBox(texConcrete_, {obj.position.x, s * 0.4f, obj.position.z},
                    s * 1.2f, s * 0.8f, s * 0.4f, (Color){200, 200, 200, 255}, 0.3f);
    // Top edge highlight
    DrawLine3D({obj.position.x - s * 0.6f, s * 0.8f, obj.position.z},
               {obj.position.x + s * 0.6f, s * 0.8f, obj.position.z},
               Fade((Color){0, 150, 150, 255}, 0.15f));
}

// --- Tire Stack ---
void Map::RenderTireStack(const EnvObject& obj) const {
    float s = obj.scale;
    // Stack of tires (cylinders with holes)
    for (int i = 0; i < 3; i++) {
        float y = s * 0.2f * (i + 1);
        float radius = s * 0.4f;
        DrawCylinder({obj.position.x, y, obj.position.z}, radius, radius, s * 0.2f, 12, obj.color);
        // Inner hole (darker circle)
        DrawCylinder({obj.position.x, y, obj.position.z}, radius * 0.4f, radius * 0.4f, s * 0.22f, 8, (Color){15, 15, 15, 255});
    }
}

// --- Pallet ---
void Map::RenderPallet(const EnvObject& obj) const {
    float s = obj.scale;
    // Base planks - textured wood
    float plankW = s * 0.2f;
    float plankL = s * 1.0f;
    float plankH = s * 0.05f;

    for (int i = 0; i < 4; i++) {
        float offset = (i - 1.5f) * plankW * 1.1f;
        DrawTexturedBox(texWood_, {obj.position.x + offset, plankH * 0.5f, obj.position.z},
                        plankW, plankH, plankL, (Color){210, 195, 150, 255}, 0.5f);
    }

    // Cross beams
    DrawTexturedBox(texWood_, {obj.position.x, plankH + s * 0.05f, obj.position.z - plankL * 0.3f},
                    s * 0.8f, s * 0.1f, plankW * 0.5f, (Color){190, 175, 130, 255}, 0.5f);
    DrawTexturedBox(texWood_, {obj.position.x, plankH + s * 0.05f, obj.position.z + plankL * 0.3f},
                    s * 0.8f, s * 0.1f, plankW * 0.5f, (Color){190, 175, 130, 255}, 0.5f);
}

// --- Generator ---
void Map::RenderGenerator(const EnvObject& obj) const {
    float s = obj.scale;
    // Main body - textured metal
    DrawTexturedBox(texMetal_, {obj.position.x, s * 0.5f, obj.position.z}, s * 0.8f, s * 1.0f, s * 0.5f,
                    (Color){170, 180, 170, 255}, 0.4f);
    // Top exhaust
    DrawCylinder({obj.position.x, s * 1.0f, obj.position.z}, s * 0.1f, s * 0.08f, s * 0.2f, 8, DARKGRAY);
    // Control panel (front)
    DrawCube({obj.position.x, s * 0.6f, obj.position.z + s * 0.26f},
             s * 0.3f, s * 0.3f, 0.02f, (Color){60, 80, 100, 255});
    // Warning light (pulsing red)
    float pulse = sinf(animTime_ * 4.0f) * 0.5f + 0.5f;
    DrawSphere({obj.position.x, s * 1.1f, obj.position.z}, s * 0.05f,
               Fade((Color){255, 0, 0, 255}, pulse));
    // Neon status line
    DrawLine3D({obj.position.x - s * 0.2f, s * 0.7f, obj.position.z + s * 0.26f},
               {obj.position.x + s * 0.2f, s * 0.7f, obj.position.z + s * 0.26f},
               Fade((Color){0, 255, 0, 255}, 0.3f * (1.0f - pulse)));
}

// --- Water Tank ---
void Map::RenderWaterTank(const EnvObject& obj) const {
    float s = obj.scale;
    float radius = s * 0.6f;
    float height = s * 1.5f;

    // Main tank cylinder - textured metal
    DrawTexturedCylinder(texMetal_, obj.position, radius, height, 16,
                         (Color){140, 160, 180, 255}, 1.0f, 0.5f);
    // Top dome
    DrawSphere({obj.position.x, height, obj.position.z}, radius, Fade(obj.color, 0.8f));
    // Pipes on side
    DrawCylinder({obj.position.x + radius, height * 0.3f, obj.position.z}, 0.05f, 0.05f, s * 0.4f, 6, DARKGRAY);
    // Ladder rungs
    for (int i = 0; i < 6; i++) {
        float y = height * 0.15f * (i + 1);
        DrawLine3D({obj.position.x - radius, y, obj.position.z},
                   {obj.position.x - radius + 0.15f, y, obj.position.z}, Fade(GRAY, 0.3f));
    }
}

// --- Satellite Dish ---
void Map::RenderSatelliteDish(const EnvObject& obj) const {
    float s = obj.scale;

    // Support pole
    DrawCylinder(obj.position, s * 0.05f, s * 0.05f, s * 1.0f, 6, GRAY);

    // Dish (cone shape)
    Vector3 dishPos = {obj.position.x, s * 1.0f, obj.position.z};
    DrawCylinder(dishPos, s * 0.01f, s * 0.6f, s * 0.15f, 12, obj.color);

    // Feed horn (small sphere)
    DrawSphere({dishPos.x, dishPos.y + s * 0.3f, dishPos.z}, s * 0.06f, DARKGRAY);

    // Support arms
    DrawLine3D(dishPos, {dishPos.x, dishPos.y + s * 0.3f, dishPos.z}, Fade(GRAY, 0.5f));
    DrawLine3D({dishPos.x - s * 0.3f, dishPos.y, dishPos.z}, {dishPos.x, dishPos.y + s * 0.3f, dishPos.z}, Fade(GRAY, 0.3f));
    DrawLine3D({dishPos.x + s * 0.3f, dishPos.y, dishPos.z}, {dishPos.x, dishPos.y + s * 0.3f, dishPos.z}, Fade(GRAY, 0.3f));
}

// ============================================================================
// Neon Lights - Glowing atmospheric light strips
// ============================================================================

void Map::RenderNeonLights() const {
    for (const auto& neon : neonLights_) {
        float pulse = sinf(animTime_ * 2.0f + neon.start.x * 0.1f) * neon.intensity * 0.3f + (1.0f - neon.intensity * 0.3f);

        // Main light line
        DrawLine3D(
            {neon.start.x, neon.height, neon.start.z},
            {neon.end.x, neon.height, neon.end.z},
            Fade(neon.color, 0.8f * pulse)
        );

        // Glow line (thicker, more transparent)
        DrawLine3D(
            {neon.start.x, neon.height + 0.05f, neon.start.z},
            {neon.end.x, neon.height + 0.05f, neon.end.z},
            Fade(neon.color, 0.3f * pulse)
        );
        DrawLine3D(
            {neon.start.x, neon.height - 0.05f, neon.start.z},
            {neon.end.x, neon.height - 0.05f, neon.end.z},
            Fade(neon.color, 0.3f * pulse)
        );

        // Light cast on nearby surface (floor glow)
        float midX = (neon.start.x + neon.end.x) * 0.5f;
        float midZ = (neon.start.z + neon.end.z) * 0.5f;
        float glowSize = 2.0f;
        DrawPlane({midX, neon.height < 2.0f ? 0.05f : neon.height - 0.5f, midZ},
                  {glowSize, glowSize}, Fade(neon.color, 0.02f * pulse));

        // End caps (small glowing dots)
        DrawSphere({neon.start.x, neon.height, neon.start.z}, 0.04f, Fade(neon.color, 0.5f * pulse));
        DrawSphere({neon.end.x, neon.height, neon.end.z}, 0.04f, Fade(neon.color, 0.5f * pulse));
    }
}

// ============================================================================
// Pickups - Enhanced with bobbing, glow, and rotating rings
// ============================================================================

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

        float bobOffset = sinf(animTime_ * 3.0f) * 0.2f;
        Vector3 pos = {
            pickup.position.x,
            pickup.position.y + bobOffset,
            pickup.position.z
        };

        // Outer glow ring
        DrawSphere(pos, 0.55f, Fade(glowColor, 0.1f));
        DrawSphereWires(pos, 0.55f, 8, 8, Fade(glowColor, 0.15f));

        // Main pickup sphere
        DrawSphere(pos, 0.3f, color);

        // Inner wireframe detail
        DrawSphereWires(pos, 0.35f, 8, 8, Fade(color, 0.6f));

        // Rotating ring effect
        float ringAngle = animTime_ * 2.0f;
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

// ============================================================================
// Spawn Pickups
// ============================================================================

void Map::SpawnPickups() {
    pickups_.clear();

    // Health pickups
    {
        Pickup p; p.type = PickupType::HEALTH; p.position = {-20, 0.5f, 0}; p.value = 50.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p; p.type = PickupType::HEALTH; p.position = {20, 0.5f, 0}; p.value = 50.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p; p.type = PickupType::HEALTH; p.position = {0, 3.5f, 0}; p.value = 50.0f;
        pickups_.push_back(p);
    }

    // Armor pickups
    {
        Pickup p; p.type = PickupType::ARMOR; p.position = {-10, 0.5f, -20}; p.value = 50.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p; p.type = PickupType::ARMOR; p.position = {10, 0.5f, 20}; p.value = 50.0f;
        pickups_.push_back(p);
    }

    // Ammo pickups
    {
        Pickup p; p.type = PickupType::AMMO; p.position = {0, 0.5f, -15}; p.value = 60.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p; p.type = PickupType::AMMO; p.position = {-30, 0.5f, -30}; p.value = 60.0f;
        pickups_.push_back(p);
    }
    {
        Pickup p; p.type = PickupType::AMMO; p.position = {30, 0.5f, 30}; p.value = 60.0f;
        pickups_.push_back(p);
    }

    // Weapon pickups
    {
        Pickup p; p.type = PickupType::WEAPON; p.position = {-25, 0.5f, 15}; p.weaponType = WeaponType::SHOTGUN;
        pickups_.push_back(p);
    }
    {
        Pickup p; p.type = PickupType::WEAPON; p.position = {25, 5.5f, -30}; p.weaponType = WeaponType::SNIPER_RIFLE;
        pickups_.push_back(p);
    }
    {
        Pickup p; p.type = PickupType::WEAPON; p.position = {0, 0.5f, 30}; p.weaponType = WeaponType::RPG;
        pickups_.push_back(p);
    }
}

// ============================================================================
// Collision - Enhanced with platforms
// ============================================================================

RayCollision Map::Raycast(const Ray& ray, float maxDistance) const {
    RayCollision closestHit = {};
    closestHit.hit = false;
    closestHit.distance = maxDistance;

    // Wall raycasts
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

    // Platform raycasts
    for (const auto& plat : platforms_) {
        BoundingBox box = {
            {plat.center.x - plat.width * 0.5f, plat.elevation - plat.thickness, plat.center.z - plat.depth * 0.5f},
            {plat.center.x + plat.width * 0.5f, plat.elevation, plat.center.z + plat.depth * 0.5f}
        };

        RayCollision hit = GetRayCollisionBox(ray, box);
        if (hit.hit && hit.distance < closestHit.distance) {
            closestHit = hit;
        }
    }

    // Pillar raycasts (approximate as boxes)
    for (const auto& pillar : pillars_) {
        BoundingBox box = {
            {pillar.base.x - pillar.radius, pillar.base.y, pillar.base.z - pillar.radius},
            {pillar.base.x + pillar.radius, pillar.base.y + pillar.height, pillar.base.z + pillar.radius}
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
    // Wall collisions
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

    // Platform collisions (only sides, not top - player should walk on top)
    for (const auto& plat : platforms_) {
        // Check side collisions
        if (position.y < plat.elevation) {
            BoundingBox box = {
                {plat.center.x - plat.width * 0.5f, plat.elevation - plat.thickness, plat.center.z - plat.depth * 0.5f},
                {plat.center.x + plat.width * 0.5f, plat.elevation, plat.center.z + plat.depth * 0.5f}
            };
            if (CheckCollisionBoxSphere(box, position, radius)) {
                return true;
            }
        }
    }

    // Pillar collisions
    for (const auto& pillar : pillars_) {
        BoundingBox box = {
            {pillar.base.x - pillar.radius, pillar.base.y, pillar.base.z - pillar.radius},
            {pillar.base.x + pillar.radius, pillar.base.y + pillar.height, pillar.base.z + pillar.radius}
        };
        if (CheckCollisionBoxSphere(box, position, radius)) {
            return true;
        }
    }

    // EnvObject collisions (solid objects only)
    for (const auto& obj : envObjects_) {
        BoundingBox box;
        bool solid = true;
        switch (obj.type) {
        case EnvObjectType::CRATE: {
            float s = 0.5f * obj.scale;
            box.min = {obj.position.x - s, obj.position.y, obj.position.z - s};
            box.max = {obj.position.x + s, obj.position.y + s * 2.0f, obj.position.z + s};
            break;
        }
        case EnvObjectType::BARREL: {
            float r = 0.35f * obj.scale;
            float h = 1.0f * obj.scale;
            box.min = {obj.position.x - r, obj.position.y, obj.position.z - r};
            box.max = {obj.position.x + r, obj.position.y + h, obj.position.z + r};
            break;
        }
        case EnvObjectType::SANDBAG_WALL:
        case EnvObjectType::METAL_BARRIER:
        case EnvObjectType::CONCRETE_BARRIER: {
            float w = 1.5f * obj.scale, h = 0.8f * obj.scale, d = 0.4f * obj.scale;
            box.min = {obj.position.x - w * 0.5f, obj.position.y, obj.position.z - d * 0.5f};
            box.max = {obj.position.x + w * 0.5f, obj.position.y + h, obj.position.z + d * 0.5f};
            break;
        }
        case EnvObjectType::GENERATOR: {
            float w = 0.8f * obj.scale, h = 1.0f * obj.scale, d = 0.5f * obj.scale;
            box.min = {obj.position.x - w * 0.5f, obj.position.y, obj.position.z - d * 0.5f};
            box.max = {obj.position.x + w * 0.5f, obj.position.y + h, obj.position.z + d * 0.5f};
            break;
        }
        case EnvObjectType::WATER_TANK: {
            float r = 0.6f * obj.scale;
            float h = 1.5f * obj.scale;
            box.min = {obj.position.x - r, obj.position.y, obj.position.z - r};
            box.max = {obj.position.x + r, obj.position.y + h, obj.position.z + r};
            break;
        }
        default:
            solid = false;
            break;
        }
        if (!solid) continue;

        if (CheckCollisionBoxSphere(box, position, radius)) {
            return true;
        }
    }

    return false;
}

Vector3 Map::ResolveCollision(const Vector3& position, float radius) const {
    Vector3 resolved = position;

    // Wall resolution - push player out using minimum penetration axis
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

        float halfLen = length * 0.5f;
        float halfThick = wall.thickness * 0.5f;

        BoundingBox box = {
            {center.x - halfLen, 0, center.z - halfThick},
            {center.x + halfLen, wall.height, center.z + halfThick}
        };

        if (CheckCollisionBoxSphere(box, resolved, radius)) {
            // Skip if player is above the wall
            if (resolved.y > wall.height) continue;

            // Find minimum penetration axis to push out correctly
            float dx_min = resolved.x - (box.min.x - radius);
            float dx_max = (box.max.x + radius) - resolved.x;
            float dz_min = resolved.z - (box.min.z - radius);
            float dz_max = (box.max.z + radius) - resolved.z;

            float minPen = 999999.0f;
            int minAxis = 0;

            if (dx_min > 0 && dx_min < minPen) { minPen = dx_min; minAxis = 0; }
            if (dx_max > 0 && dx_max < minPen) { minPen = dx_max; minAxis = 1; }
            if (dz_min > 0 && dz_min < minPen) { minPen = dz_min; minAxis = 2; }
            if (dz_max > 0 && dz_max < minPen) { minPen = dz_max; minAxis = 3; }

            switch (minAxis) {
            case 0: resolved.x = box.min.x - radius - 0.01f; break;
            case 1: resolved.x = box.max.x + radius + 0.01f; break;
            case 2: resolved.z = box.min.z - radius - 0.01f; break;
            case 3: resolved.z = box.max.z + radius + 0.01f; break;
            }
        }
    }

    // Platform side resolution (only when below platform surface)
    for (const auto& plat : platforms_) {
        if (resolved.y < plat.elevation) {
            BoundingBox box = {
                {plat.center.x - plat.width * 0.5f, plat.elevation - plat.thickness, plat.center.z - plat.depth * 0.5f},
                {plat.center.x + plat.width * 0.5f, plat.elevation, plat.center.z + plat.depth * 0.5f}
            };
            if (CheckCollisionBoxSphere(box, resolved, radius)) {
                float dx_min = resolved.x - (box.min.x - radius);
                float dx_max = (box.max.x + radius) - resolved.x;
                float dz_min = resolved.z - (box.min.z - radius);
                float dz_max = (box.max.z + radius) - resolved.z;

                float minPen = 999999.0f;
                int minAxis = 0;

                if (dx_min > 0 && dx_min < minPen) { minPen = dx_min; minAxis = 0; }
                if (dx_max > 0 && dx_max < minPen) { minPen = dx_max; minAxis = 1; }
                if (dz_min > 0 && dz_min < minPen) { minPen = dz_min; minAxis = 2; }
                if (dz_max > 0 && dz_max < minPen) { minPen = dz_max; minAxis = 3; }

                switch (minAxis) {
                case 0: resolved.x = box.min.x - radius - 0.01f; break;
                case 1: resolved.x = box.max.x + radius + 0.01f; break;
                case 2: resolved.z = box.min.z - radius - 0.01f; break;
                case 3: resolved.z = box.max.z + radius + 0.01f; break;
                }
            }
        }
    }

    // Pillar resolution - push out radially from pillar center
    for (const auto& pillar : pillars_) {
        Vector3 dir = {resolved.x - pillar.base.x, 0, resolved.z - pillar.base.z};
        float dist = sqrtf(dir.x * dir.x + dir.z * dir.z);
        if (dist < pillar.radius + radius && resolved.y < pillar.base.y + pillar.height) {
            if (dist > 0.01f) {
                dir.x /= dist;
                dir.z /= dist;
                resolved.x = pillar.base.x + dir.x * (pillar.radius + radius + 0.05f);
                resolved.z = pillar.base.z + dir.z * (pillar.radius + radius + 0.05f);
            } else {
                resolved.x = pillar.base.x + pillar.radius + radius + 0.05f;
            }
        }
    }

    // EnvObject collisions (approximate as boxes)
    for (const auto& obj : envObjects_) {
        BoundingBox box;
        bool solid = true;
        switch (obj.type) {
        case EnvObjectType::CRATE: {
            float s = 0.5f * obj.scale;
            box.min = {obj.position.x - s, obj.position.y, obj.position.z - s};
            box.max = {obj.position.x + s, obj.position.y + s * 2.0f, obj.position.z + s};
            break;
        }
        case EnvObjectType::BARREL: {
            float r = 0.35f * obj.scale;
            float h = 1.0f * obj.scale;
            box.min = {obj.position.x - r, obj.position.y, obj.position.z - r};
            box.max = {obj.position.x + r, obj.position.y + h, obj.position.z + r};
            break;
        }
        case EnvObjectType::SANDBAG_WALL:
        case EnvObjectType::METAL_BARRIER:
        case EnvObjectType::CONCRETE_BARRIER: {
            float w = 1.5f * obj.scale, h = 0.8f * obj.scale, d = 0.4f * obj.scale;
            box.min = {obj.position.x - w * 0.5f, obj.position.y, obj.position.z - d * 0.5f};
            box.max = {obj.position.x + w * 0.5f, obj.position.y + h, obj.position.z + d * 0.5f};
            break;
        }
        case EnvObjectType::GENERATOR: {
            float w = 0.8f * obj.scale, h = 1.0f * obj.scale, d = 0.5f * obj.scale;
            box.min = {obj.position.x - w * 0.5f, obj.position.y, obj.position.z - d * 0.5f};
            box.max = {obj.position.x + w * 0.5f, obj.position.y + h, obj.position.z + d * 0.5f};
            break;
        }
        case EnvObjectType::WATER_TANK: {
            float r = 0.6f * obj.scale;
            float h = 1.5f * obj.scale;
            box.min = {obj.position.x - r, obj.position.y, obj.position.z - r};
            box.max = {obj.position.x + r, obj.position.y + h, obj.position.z + r};
            break;
        }
        default:
            solid = false;
            break;
        }

        if (!solid) continue;

        if (CheckCollisionBoxSphere(box, resolved, radius)) {
            float dx_min = resolved.x - (box.min.x - radius);
            float dx_max = (box.max.x + radius) - resolved.x;
            float dz_min = resolved.z - (box.min.z - radius);
            float dz_max = (box.max.z + radius) - resolved.z;

            float minPen = 999999.0f;
            int minAxis = 0;

            if (dx_min > 0 && dx_min < minPen) { minPen = dx_min; minAxis = 0; }
            if (dx_max > 0 && dx_max < minPen) { minPen = dx_max; minAxis = 1; }
            if (dz_min > 0 && dz_min < minPen) { minPen = dz_min; minAxis = 2; }
            if (dz_max > 0 && dz_max < minPen) { minPen = dz_max; minAxis = 3; }

            switch (minAxis) {
            case 0: resolved.x = box.min.x - radius - 0.01f; break;
            case 1: resolved.x = box.max.x + radius + 0.01f; break;
            case 2: resolved.z = box.min.z - radius - 0.01f; break;
            case 3: resolved.z = box.max.z + radius + 0.01f; break;
            }
        }
    }

    resolved.x = std::clamp(resolved.x, -bounds_.x + radius, bounds_.x - radius);
    resolved.z = std::clamp(resolved.z, -bounds_.z + radius, bounds_.z - radius);

    return resolved;
}

float Map::GetGroundHeight(const Vector3& position, float playerRadius) const {
    float groundY = 0.0f;  // Default ground level

    // Check platforms - if player is above or at the top surface of a platform,
    // that becomes their ground level
    for (const auto& plat : platforms_) {
        // Check if player's XZ position is within the platform's horizontal bounds
        float halfW = plat.width * 0.5f + playerRadius;
        float halfD = plat.depth * 0.5f + playerRadius;
        if (position.x >= plat.center.x - halfW && position.x <= plat.center.x + halfW &&
            position.z >= plat.center.z - halfD && position.z <= plat.center.z + halfD) {
            // Player is within platform XZ bounds
            // If they're near or above the platform surface, use platform elevation as ground
            if (position.y >= plat.elevation - 0.3f) {
                if (plat.elevation > groundY) {
                    groundY = plat.elevation;
                }
            }
        }
    }

    // Check stairs - approximate as a slope
    for (const auto& stair : staircases_) {
        // Transform player position into staircase local space
        float dx = position.x - stair.position.x;
        float dz = position.z - stair.position.z;
        float cosR = cosf(-stair.rotation);
        float sinR = sinf(-stair.rotation);
        float localX = dx * cosR - dz * sinR;
        float localZ = dx * sinR + dz * cosR;

        // Check if player is within the staircase bounds
        float halfW = stair.width * 0.5f + playerRadius;
        if (localX >= -halfW && localX <= halfW &&
            localZ >= -playerRadius && localZ <= stair.totalDepth + playerRadius) {
            // Calculate height at this point on the staircase
            float t = std::clamp(localZ / stair.totalDepth, 0.0f, 1.0f);
            float stairY = stair.position.y + stair.totalHeight * t;

            // If player is near or above this stair height
            if (position.y >= stairY - 0.5f && stairY > groundY) {
                groundY = stairY;
            }
        }
    }

    // Check env objects that can be stood on top of
    for (const auto& obj : envObjects_) {
        float topY = 0.0f;
        bool canStandOn = false;
        float halfW = 0.0f, halfD = 0.0f;

        switch (obj.type) {
        case EnvObjectType::CRATE: {
            float s = 0.5f * obj.scale;
            topY = obj.position.y + s * 2.0f;
            halfW = s + playerRadius;
            halfD = s + playerRadius;
            canStandOn = true;
            break;
        }
        case EnvObjectType::METAL_BARRIER:
        case EnvObjectType::CONCRETE_BARRIER: {
            float w = 1.5f * obj.scale, h = 0.8f * obj.scale, d = 0.4f * obj.scale;
            topY = obj.position.y + h;
            halfW = w * 0.5f + playerRadius;
            halfD = d * 0.5f + playerRadius;
            canStandOn = true;
            break;
        }
        case EnvObjectType::GENERATOR: {
            float w = 0.8f * obj.scale, h = 1.0f * obj.scale, d = 0.5f * obj.scale;
            topY = obj.position.y + h;
            halfW = w * 0.5f + playerRadius;
            halfD = d * 0.5f + playerRadius;
            canStandOn = true;
            break;
        }
        case EnvObjectType::WATER_TANK: {
            float r = 0.6f * obj.scale;
            topY = obj.position.y + 1.5f * obj.scale;
            halfW = r + playerRadius;
            halfD = r + playerRadius;
            canStandOn = true;
            break;
        }
        default:
            break;
        }

        if (canStandOn) {
            if (position.x >= obj.position.x - halfW && position.x <= obj.position.x + halfW &&
                position.z >= obj.position.z - halfD && position.z <= obj.position.z + halfD) {
                if (position.y >= topY - 0.3f && topY > groundY) {
                    groundY = topY;
                }
            }
        }
    }

    return groundY;
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
