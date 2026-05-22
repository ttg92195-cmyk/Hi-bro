#pragma once
// ============================================================================
// EOS Shooter - TextureGenerator.h
// Procedural texture generation using Raylib image functions.
// Creates Brick, Concrete, Metal, Wood, Stone, FloorTile, Sand, Snow,
// Crate, Barrel, Rust, Door, Stair, Grass textures at runtime.
// No external asset files needed - everything is generated from code.
// ============================================================================

#include "raylib.h"
#include <cstdint>

namespace EOSShooter {

class TextureGenerator {
public:
    // === Individual texture generators ===
    // Each returns a Texture2D ready for rendering with TEXTURE_WRAP_REPEAT

    static Texture2D GenerateBrick(int size = 256);
    static Texture2D GenerateConcrete(int size = 256);
    static Texture2D GenerateMetal(int size = 256);
    static Texture2D GenerateWood(int size = 256);
    static Texture2D GenerateStone(int size = 256);
    static Texture2D GenerateFloorTile(int size = 256);
    static Texture2D GenerateSand(int size = 256);
    static Texture2D GenerateSnow(int size = 256);
    static Texture2D GenerateCrate(int size = 256);
    static Texture2D GenerateBarrel(int size = 256);
    static Texture2D GenerateRust(int size = 256);
    static Texture2D GenerateDoor(int size = 256);
    static Texture2D GenerateStair(int size = 256);
    static Texture2D GenerateGrass(int size = 256);

private:
    // Helper: add subtle noise overlay to an image
    static void AddNoise(Image& img, float intensity, int seed);
};

} // namespace EOSShooter
