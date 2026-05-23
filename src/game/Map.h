#pragma once
// ============================================================================
// EOS Shooter - Map.h
// Map system with geometry, spawn points, pickups, and collision.
// Enhanced with platforms, ramps, pillars, environmental objects,
// neon lights, decorative structures, and procedural textures.
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
    Pickup() = default;
};

struct Wall {
    Vector3 start;
    Vector3 end;
    float height;
    float thickness;
    Color color;
    Wall() = default;
    Wall(Vector3 s, Vector3 e, float h, float t, Color c)
        : start(s), end(e), height(h), thickness(t), color(c) {}
};

// Elevated platform (can be walked on)
struct Platform {
    Vector3 center;       // Center position
    float width;          // X extent
    float depth;          // Z extent
    float elevation;      // Y position of top surface
    float thickness;      // Height of the platform block
    Color color;          // Surface color
    Color edgeColor;      // Edge/rim color (for neon trim)
    bool hasRailing;      // Draw safety railing edges
    Platform() = default;
    Platform(Vector3 c, float w, float d, float elev, float thick, Color col, Color edge, bool rail = false)
        : center(c), width(w), depth(d), elevation(elev), thickness(thick),
          color(col), edgeColor(edge), hasRailing(rail) {}
};

// Ramp connecting different elevations
struct Ramp {
    Vector3 baseCenter;   // Center of the bottom edge
    float width;          // X extent
    float length;         // Z extent (along the slope)
    float baseHeight;     // Height at the bottom
    float topHeight;      // Height at the top
    float angle;          // Rotation in radians (which direction ramp faces)
    Color color;
    Color edgeColor;
    Ramp() = default;
    Ramp(Vector3 bc, float w, float l, float bh, float th, float a, Color c, Color e)
        : baseCenter(bc), width(w), length(l), baseHeight(bh), topHeight(th),
          angle(a), color(c), edgeColor(e) {}
};

// Cylindrical pillar/column
struct Pillar {
    Vector3 base;
    float radius;
    float height;
    Color color;
    Color capColor;       // Top/bottom cap color
    Pillar() = default;
    Pillar(Vector3 b, float r, float h, Color c, Color cap)
        : base(b), radius(r), height(h), color(c), capColor(cap) {}
};

// Environmental object types
enum class EnvObjectType {
    CRATE,           // Wooden/metal crate
    BARREL,          // Metal barrel
    ROCK_LARGE,      // Large boulder
    ROCK_SMALL,      // Small rock
    SANDBAG_WALL,    // Sandbag fortification
    METAL_BARRIER,   // Road barrier / jersey barrier
    CONCRETE_BARRIER,// Concrete wall segment
    TIRE_STACK,      // Stack of tires
    PALLET,          // Wooden pallet
    GENERATOR,       // Industrial generator
    WATER_TANK,      // Cylindrical water tank
    SATELLITE_DISH   // Satellite dish
};

struct EnvObject {
    EnvObjectType type = EnvObjectType::CRATE;
    Vector3 position = {0, 0, 0};
    float scale = 1.0f;
    float rotation = 0.0f;     // Y-axis rotation
    Color color;
    EnvObject() = default;
    EnvObject(EnvObjectType t, Vector3 pos, float s, float rot, Color c)
        : type(t), position(pos), scale(s), rotation(rot), color(c) {}
};

// Neon light strip for atmospheric lighting
struct NeonLight {
    Vector3 start;
    Vector3 end;
    float height;          // Y position
    Color color;
    float intensity;       // 0-1 brightness pulse factor
    NeonLight() = default;
    NeonLight(Vector3 s, Vector3 e, float h, Color c, float i = 0.5f)
        : start(s), end(e), height(h), color(c), intensity(i) {}
};

// Decorative arch/door frame structure
struct ArchStruct {
    Vector3 position;
    float width;
    float height;
    float depth;           // Wall thickness
    float rotation;        // Y-axis rotation
    Color color;
    Color trimColor;
    ArchStruct() = default;
    ArchStruct(Vector3 p, float w, float h, float d, float r, Color c, Color tc)
        : position(p), width(w), height(h), depth(d), rotation(r), color(c), trimColor(tc) {}
};

// Staircase structure
struct Staircase {
    Vector3 position;      // Bottom-center of the staircase
    float width;           // X extent
    float totalHeight;     // Total rise
    float totalDepth;      // Total Z run
    int numSteps;
    float rotation;        // Y-axis rotation (radians)
    Color stepColor;
    Color riserColor;
    Staircase() = default;
    Staircase(Vector3 p, float w, float th, float td, int ns, float rot, Color sc, Color rc)
        : position(p), width(w), totalHeight(th), totalDepth(td),
          numSteps(ns), rotation(rot), stepColor(sc), riserColor(rc) {}
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
    float GetGroundHeight(const Vector3& position, float playerRadius = 0.4f) const;

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
    float animTime_ = 0.0f;    // For animated effects

    // === Map Geometry ===
    std::vector<Wall> walls_;
    std::vector<Platform> platforms_;
    std::vector<Ramp> ramps_;
    std::vector<Pillar> pillars_;
    std::vector<EnvObject> envObjects_;
    std::vector<NeonLight> neonLights_;
    std::vector<ArchStruct> arches_;
    std::vector<Staircase> staircases_;

    // === Procedural Textures ===
    Texture2D texBrick_ = {};
    Texture2D texConcrete_ = {};
    Texture2D texMetal_ = {};
    Texture2D texWood_ = {};
    Texture2D texStone_ = {};
    Texture2D texFloorTile_ = {};
    Texture2D texSand_ = {};
    Texture2D texSnow_ = {};
    Texture2D texCrate_ = {};
    Texture2D texBarrel_ = {};
    Texture2D texRust_ = {};
    Texture2D texDoor_ = {};
    Texture2D texStair_ = {};
    Texture2D texGrass_ = {};
    bool texturesLoaded_ = false;

    // === Textured Drawing Helpers ===
    void DrawTexturedBox(Texture2D tex, Vector3 pos, float w, float h, float d,
                          Color tint = WHITE, float tileScale = 0.5f) const;
    void DrawTexturedPlane(Texture2D tex, Vector3 pos, Vector2 size,
                            Color tint = WHITE, float tileScale = 0.5f) const;
    void DrawTexturedCylinder(Texture2D tex, Vector3 pos, float radius, float height,
                               int slices, Color tint = WHITE,
                               float tileU = 1.0f, float tileV = 1.0f) const;

    // === Texture Management ===
    void InitTextures();
    void UnloadTextures();

    // === Positions ===
    std::vector<Vector3> spawnPositions_;
    std::vector<Vector3> enemySpawnPositions_;
    std::vector<Pickup> pickups_;
    std::vector<Vector3> coverPositions_;

    // === Texture Selection (per-map) ===
    Texture2D GetWallTexture() const;
    Texture2D GetFloorTexture() const;
    Texture2D GetPlatformTexture() const;
    Texture2D GetStairTexture() const;

    // === Map Generators ===
    void GenerateUrbanWarehouse();
    void GenerateDesertOutpost();
    void GenerateArcticBase();

    // === Render Helpers ===
    void RenderFloor() const;
    void RenderWalls() const;
    void RenderSkybox() const;
    void RenderPlatforms() const;
    void RenderRamps() const;
    void RenderPillars() const;
    void RenderEnvObjects() const;
    void RenderNeonLights() const;
    void RenderArches() const;
    void RenderStaircases() const;

    // === Individual EnvObject renderers ===
    void RenderCrate(const EnvObject& obj) const;
    void RenderBarrel(const EnvObject& obj) const;
    void RenderRockLarge(const EnvObject& obj) const;
    void RenderRockSmall(const EnvObject& obj) const;
    void RenderSandbagWall(const EnvObject& obj) const;
    void RenderMetalBarrier(const EnvObject& obj) const;
    void RenderConcreteBarrier(const EnvObject& obj) const;
    void RenderTireStack(const EnvObject& obj) const;
    void RenderPallet(const EnvObject& obj) const;
    void RenderGenerator(const EnvObject& obj) const;
    void RenderWaterTank(const EnvObject& obj) const;
    void RenderSatelliteDish(const EnvObject& obj) const;
};

} // namespace EOSShooter
