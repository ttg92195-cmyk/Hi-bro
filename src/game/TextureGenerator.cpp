// ============================================================================
// EOS Shooter - TextureGenerator.cpp
// Procedural texture generation - all textures created from code.
// Uses Raylib's GenImage*, ImageDraw* functions.
// ============================================================================

#include "TextureGenerator.h"
#include <cmath>
#include <cstdlib>

namespace EOSShooter {

// ============================================================================
// Local helper: clamp color channel to 0-255
// ============================================================================
namespace {
    uint8_t ClampChannel(int value) {
        return static_cast<uint8_t>(value < 0 ? 0 : (value > 255 ? 255 : value));
    }
}

// ============================================================================
// Helper: Add subtle noise overlay
// ============================================================================

void TextureGenerator::AddNoise(Image& img, float intensity, int seed) {
    if (img.data == nullptr) return;
    int w = img.width;
    int h = img.height;
    Color* pixels = static_cast<Color*>(img.data);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            // Simple hash-based noise
            int n = ((x * 374761393 + y * 668265263 + seed * 1274126177) & 0x7fffffff);
            n = (n >> 13) & 0xff;
            int noise = static_cast<int>((n - 128) * intensity);
            pixels[idx].r = ClampChannel(pixels[idx].r + noise);
            pixels[idx].g = ClampChannel(pixels[idx].g + noise);
            pixels[idx].b = ClampChannel(pixels[idx].b + noise);
        }
    }
}

// ============================================================================
// Brick - Running bond pattern with mortar and per-brick color variation
// ============================================================================

Texture2D TextureGenerator::GenerateBrick(int size) {
    Image img = GenImageColor(size, size, (Color){180, 170, 155, 255}); // mortar color

    int brickH = size / 8;      // 32px per brick height
    int brickW = size / 4;      // 64px per brick width
    int mortar = 2;

    for (int row = 0; row < 8; row++) {
        int offset = (row % 2) * (brickW / 2); // running bond offset
        for (int col = -1; col < 5; col++) {
            int x = col * brickW + offset + mortar;
            int y = row * brickH + mortar;
            int w = brickW - mortar * 2;
            int h = brickH - mortar * 2;

            // Per-brick color variation
            int v = ((row * 7 + col * 13 + 3) * 17) % 40 - 20;
            Color brickColor = {
                ClampChannel(165 + v),
                ClampChannel(72 + v / 2),
                ClampChannel(52 + v / 3),
                255
            };

            // Clip to image bounds
            int drawX = (x < 0) ? 0 : x;
            int drawY = (y < 0) ? 0 : y;
            int drawW = (drawX + w > size) ? (size - drawX) : w;
            int drawH = (drawY + h > size) ? (size - drawY) : h;
            if (drawW > 0 && drawH > 0) {
                ImageDrawRectangle(&img, drawX, drawY, drawW, drawH, brickColor);
            }
        }
    }

    // Add subtle weathering noise
    AddNoise(img, 0.15f, 42);

    // Ensure RGBA format
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Concrete - Perlin noise based with grey tones
// ============================================================================

Texture2D TextureGenerator::GenerateConcrete(int size) {
    Image img = GenImagePerlinNoise(size, size, 0, 0, 5.0f);

    // Tint to concrete grey
    if (img.data != nullptr) {
        Color* pixels = static_cast<Color*>(img.data);
        for (int i = 0; i < size * size; i++) {
            int g = ClampChannel(pixels[i].r * 0.4f + 110);
            pixels[i] = Color{ClampChannel(g + 2), ClampChannel(g + 2), ClampChannel(g + 5), 255};
        }
    }

    // Add fine noise for texture
    AddNoise(img, 0.2f, 77);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Metal - Dark brushed metal with rivets
// ============================================================================

Texture2D TextureGenerator::GenerateMetal(int size) {
    Image img = GenImageColor(size, size, (Color){75, 80, 90, 255});

    // Horizontal brushed lines
    for (int y = 0; y < size; y += 4) {
        int v = ((y * 7 + 3) % 15) - 7;
        Color lineColor = {
            ClampChannel(82 + v),
            ClampChannel(87 + v),
            ClampChannel(97 + v),
            255
        };
        ImageDrawRectangle(&img, 0, y, size, 1, lineColor);
    }

    // Rivets at grid positions
    for (int gy = 0; gy < 2; gy++) {
        for (int gx = 0; gx < 4; gx++) {
            int cx = size / 8 + gx * size / 4;
            int cy = size / 4 + gy * size / 2;
            ImageDrawCircle(&img, cx, cy, 4, (Color){100, 105, 115, 255});
            ImageDrawCircle(&img, cx, cy, 3, (Color){90, 95, 105, 255});
        }
    }

    AddNoise(img, 0.1f, 99);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Wood - Brown with grain lines and knots
// ============================================================================

Texture2D TextureGenerator::GenerateWood(int size) {
    Image img = GenImageColor(size, size, (Color){130, 85, 45, 255});

    // Horizontal grain lines
    for (int y = 0; y < size; y++) {
        int wave = static_cast<int>(sinf(y * 0.15f) * 3.0f);
        int v = ((y * 3 + 7) % 20) - 10;
        Color grainColor = {
            ClampChannel(125 + v + wave),
            ClampChannel(80 + v + wave),
            ClampChannel(42 + v / 2),
            255
        };
        ImageDrawRectangle(&img, 0, y, size, 1, grainColor);
    }

    // Dark knots
    int knotPositions[][2] = {{64, 80}, {180, 160}, {100, 220}};
    for (auto& pos : knotPositions) {
        ImageDrawCircle(&img, pos[0], pos[1], 8, (Color){90, 55, 25, 255});
        ImageDrawCircle(&img, pos[0], pos[1], 5, (Color){80, 48, 20, 255});
    }

    AddNoise(img, 0.12f, 55);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Stone - Cellular noise for rock surface
// ============================================================================

Texture2D TextureGenerator::GenerateStone(int size) {
    Image img = GenImageCellular(size, size, 32);

    // Tint to stone colors
    if (img.data != nullptr) {
        Color* pixels = static_cast<Color*>(img.data);
        for (int i = 0; i < size * size; i++) {
            int v = pixels[i].r;
            int g = v * 0.45f + 75;
            pixels[i] = Color{
                ClampChannel(g + 5),
                ClampChannel(g),
                ClampChannel(g - 5),
                255
            };
        }
    }

    AddNoise(img, 0.2f, 123);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Floor Tile - Dark tiles with grid lines
// ============================================================================

Texture2D TextureGenerator::GenerateFloorTile(int size) {
    Image img = GenImageColor(size, size, (Color){35, 38, 42, 255});

    int tileSize = size / 4; // 4x4 tiles

    // Draw tile grid
    for (int i = 0; i <= 4; i++) {
        int pos = i * tileSize;
        // Dark grout line
        ImageDrawRectangle(&img, 0, pos, size, 2, (Color){20, 22, 25, 255});
        ImageDrawRectangle(&img, pos, 0, 2, size, (Color){20, 22, 25, 255});
    }

    // Per-tile color variation
    for (int ty = 0; ty < 4; ty++) {
        for (int tx = 0; tx < 4; tx++) {
            int v = ((tx * 7 + ty * 13 + 5) * 3) % 12 - 6;
            Color tileColor = {
                ClampChannel(38 + v),
                ClampChannel(41 + v),
                ClampChannel(45 + v),
                255
            };
            int x = tx * tileSize + 2;
            int y = ty * tileSize + 2;
            ImageDrawRectangle(&img, x, y, tileSize - 2, tileSize - 2, tileColor);
        }
    }

    AddNoise(img, 0.08f, 33);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Sand - Tan with dune patterns
// ============================================================================

Texture2D TextureGenerator::GenerateSand(int size) {
    Image img = GenImagePerlinNoise(size, size, 10, 20, 4.0f);

    // Tint to sandy colors
    if (img.data != nullptr) {
        Color* pixels = static_cast<Color*>(img.data);
        for (int i = 0; i < size * size; i++) {
            int v = pixels[i].r;
            pixels[i] = Color{
                ClampChannel(v * 0.5f + 160),
                ClampChannel(v * 0.4f + 135),
                ClampChannel(v * 0.3f + 85),
                255
            };
        }
    }

    AddNoise(img, 0.1f, 88);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Snow - White/light blue with subtle drifts
// ============================================================================

Texture2D TextureGenerator::GenerateSnow(int size) {
    Image img = GenImagePerlinNoise(size, size, 50, 50, 6.0f);

    // Tint to snow/ice colors
    if (img.data != nullptr) {
        Color* pixels = static_cast<Color*>(img.data);
        for (int i = 0; i < size * size; i++) {
            int v = pixels[i].r;
            pixels[i] = Color{
                ClampChannel(v * 0.3f + 195),
                ClampChannel(v * 0.3f + 205),
                ClampChannel(v * 0.35f + 215),
                255
            };
        }
    }

    AddNoise(img, 0.05f, 111);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Crate - Wood planks with cross pattern and metal brackets
// ============================================================================

Texture2D TextureGenerator::GenerateCrate(int size) {
    // Wood base
    Image img = GenImageColor(size, size, (Color){150, 105, 55, 255});

    // Horizontal planks
    int plankH = size / 5;
    for (int i = 0; i < 5; i++) {
        int y = i * plankH;
        int v = (i * 17) % 20 - 10;
        Color plankColor = {
            ClampChannel(145 + v),
            ClampChannel(100 + v),
            ClampChannel(50 + v / 2),
            255
        };
        ImageDrawRectangle(&img, 0, y + 1, size, plankH - 2, plankColor);
        // Plank gap
        ImageDrawRectangle(&img, 0, y, size, 1, (Color){80, 55, 25, 255});
    }

    // Cross braces
    Color braceColor = {120, 85, 40, 255};
    for (int i = 0; i < size; i++) {
        int x1 = i;
        int y1 = i;
        int x2 = size - 1 - i;
        ImageDrawPixel(&img, x1, y1, braceColor);
        ImageDrawPixel(&img, x1, y1 + 1, braceColor);
        ImageDrawPixel(&img, x2, y1, braceColor);
        ImageDrawPixel(&img, x2, y1 + 1, braceColor);
    }

    // Metal corner brackets
    Color metalColor = {100, 105, 110, 255};
    int bSize = size / 6;
    // Top-left
    ImageDrawRectangle(&img, 0, 0, bSize, 3, metalColor);
    ImageDrawRectangle(&img, 0, 0, 3, bSize, metalColor);
    // Top-right
    ImageDrawRectangle(&img, size - bSize, 0, bSize, 3, metalColor);
    ImageDrawRectangle(&img, size - 3, 0, 3, bSize, metalColor);
    // Bottom-left
    ImageDrawRectangle(&img, 0, size - 3, bSize, 3, metalColor);
    ImageDrawRectangle(&img, 0, size - bSize, 3, bSize, metalColor);
    // Bottom-right
    ImageDrawRectangle(&img, size - bSize, size - 3, bSize, 3, metalColor);
    ImageDrawRectangle(&img, size - 3, size - bSize, 3, bSize, metalColor);

    // Nail dots
    Color nailColor = {130, 135, 140, 255};
    int ns = size / 8;
    ImageDrawCircle(&img, ns, ns, 2, nailColor);
    ImageDrawCircle(&img, size - ns, ns, 2, nailColor);
    ImageDrawCircle(&img, ns, size - ns, 2, nailColor);
    ImageDrawCircle(&img, size - ns, size - ns, 2, nailColor);

    AddNoise(img, 0.1f, 66);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Barrel - Metal with horizontal bands
// ============================================================================

Texture2D TextureGenerator::GenerateBarrel(int size) {
    Image img = GenImageColor(size, size, (Color){70, 75, 80, 255});

    // Horizontal bands (darker stripes)
    int bandH = size / 8;
    for (int i = 0; i < 8; i++) {
        int y = i * bandH;
        if (i % 2 == 0) {
            ImageDrawRectangle(&img, 0, y, size, bandH, (Color){60, 65, 70, 255});
        } else {
            ImageDrawRectangle(&img, 0, y, size, bandH, (Color){85, 90, 95, 255});
        }
    }

    // Band ridges
    for (int i = 0; i < 4; i++) {
        int y = (i * 2 + 1) * bandH;
        ImageDrawRectangle(&img, 0, y - 1, size, 3, (Color){50, 55, 60, 255});
    }

    // Center label area
    int labelY = size / 3;
    int labelH = size / 3;
    ImageDrawRectangle(&img, size / 6, labelY, size * 2 / 3, labelH, (Color){150, 50, 30, 255});

    // Hazard stripes
    Color hazardColor = {200, 200, 0, 255};
    for (int i = 0; i < 8; i++) {
        int x1 = i * size / 8;
        int x2 = x1 + size / 16;
        if (i % 2 == 0) {
            ImageDrawRectangle(&img, x1, labelY, x2 - x1, labelH, hazardColor);
        }
    }

    AddNoise(img, 0.08f, 44);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Rust - Perlin noise with orange/red tint
// ============================================================================

Texture2D TextureGenerator::GenerateRust(int size) {
    Image img = GenImagePerlinNoise(size, size, 30, 30, 4.0f);

    // Tint to rust colors
    if (img.data != nullptr) {
        Color* pixels = static_cast<Color*>(img.data);
        for (int i = 0; i < size * size; i++) {
            int v = pixels[i].r;
            pixels[i] = Color{
                ClampChannel(v * 0.6f + 100),
                ClampChannel(v * 0.25f + 40),
                ClampChannel(v * 0.1f + 15),
                255
            };
        }
    }

    AddNoise(img, 0.15f, 200);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Door - Metal panel with frame
// ============================================================================

Texture2D TextureGenerator::GenerateDoor(int size) {
    Image img = GenImageColor(size, size, (Color){65, 70, 78, 255});

    int frameW = size / 12;

    // Outer frame (darker)
    Color frameColor = {50, 55, 62, 255};
    ImageDrawRectangle(&img, 0, 0, size, frameW, frameColor);
    ImageDrawRectangle(&img, 0, size - frameW, size, frameW, frameColor);
    ImageDrawRectangle(&img, 0, 0, frameW, size, frameColor);
    ImageDrawRectangle(&img, size - frameW, 0, frameW, size, frameColor);

    // Inner panel
    int panelM = size / 5;
    Color panelColor = {80, 85, 95, 255};
    ImageDrawRectangle(&img, panelM, panelM, size - panelM * 2, size - panelM * 2, panelColor);

    // Panel emboss lines
    Color embossColor = {70, 75, 83, 255};
    ImageDrawRectangleLines(&img,
        (Rectangle){(float)panelM, (float)panelM,
                    (float)(size - panelM * 2), (float)(size - panelM * 2)},
        2, embossColor);

    // Handle
    int handleX = size - panelM - size / 10;
    int handleY = size / 2;
    ImageDrawCircle(&img, handleX, handleY, size / 16, (Color){180, 170, 50, 255});
    ImageDrawCircle(&img, handleX, handleY, size / 24, (Color){200, 190, 60, 255});

    AddNoise(img, 0.08f, 155);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Stair - Concrete with step edge marking
// ============================================================================

Texture2D TextureGenerator::GenerateStair(int size) {
    // Base concrete
    Image img = GenImagePerlinNoise(size, size, 5, 5, 5.0f);

    // Tint to concrete grey
    if (img.data != nullptr) {
        Color* pixels = static_cast<Color*>(img.data);
        for (int i = 0; i < size * size; i++) {
            int g = ClampChannel(pixels[i].r * 0.35f + 90);
            pixels[i] = Color{ClampChannel(g + 3), ClampChannel(g + 3), ClampChannel(g + 8), 255};
        }
    }

    // Step edge marking (yellow safety strip at top)
    int stripH = size / 10;
    Color stripColor = {180, 170, 40, 255};
    ImageDrawRectangle(&img, 0, 0, size, stripH, stripColor);

    // Anti-slip pattern dots on the strip
    Color dotColor = {160, 150, 30, 255};
    for (int y = 2; y < stripH - 2; y += 6) {
        for (int x = 4; x < size; x += 8) {
            ImageDrawPixel(&img, x, y, dotColor);
        }
    }

    AddNoise(img, 0.1f, 222);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// ============================================================================
// Grass - Green with variation
// ============================================================================

Texture2D TextureGenerator::GenerateGrass(int size) {
    Image img = GenImagePerlinNoise(size, size, 15, 15, 6.0f);

    // Tint to grass green
    if (img.data != nullptr) {
        Color* pixels = static_cast<Color*>(img.data);
        for (int i = 0; i < size * size; i++) {
            int v = pixels[i].r;
            pixels[i] = Color{
                ClampChannel(v * 0.25f + 35),
                ClampChannel(v * 0.5f + 80),
                ClampChannel(v * 0.15f + 20),
                255
            };
        }
    }

    AddNoise(img, 0.15f, 77);

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

} // namespace EOSShooter
