#!/bin/bash
# ============================================================================
# EOS Shooter - Setup Script
# Clones dependencies and prepares the build environment.
# ============================================================================

set -e

echo "=========================================="
echo "  EOS Shooter - Project Setup"
echo "=========================================="

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
THIRD_PARTY="$PROJECT_DIR/third_party"

# Create third_party directory
mkdir -p "$THIRD_PARTY"
cd "$THIRD_PARTY"

# === Clone Raylib ===
echo ""
echo "[1/2] Cloning Raylib..."
if [ -d "raylib" ]; then
    echo "  Raylib already exists, updating..."
    cd raylib && git pull && cd ..
else
    git clone --depth 1 https://github.com/raysan5/raylib.git
    echo "  Raylib cloned successfully!"
fi

# === EOS SDK ===
echo ""
echo "[2/2] EOS SDK Setup..."
if [ -d "eos-sdk" ] && [ -f "eos-sdk/Include/eos_sdk.h" ]; then
    echo "  EOS SDK already installed!"
else
    echo "  ============================================"
    echo "  EOS SDK is NOT included (proprietary)."
    echo "  To enable multiplayer:"
    echo "  1. Register at https://dev.epicgames.com/"
    echo "  2. Download the EOS SDK"
    echo "  3. Extract to: $THIRD_PARTY/eos-sdk/"
    echo ""
    echo "  Expected structure:"
    echo "    eos-sdk/Include/eos_sdk.h"
    echo "    eos-sdk/Lib/Linux/Release/libEOSSDK.a"
    echo "    eos-sdk/Lib/Win64/Release/EOSSDK.lib"
    echo "    eos-sdk/Lib/Android/arm64-v8a/libEOSSDK.a"
    echo "  ============================================"
    echo ""
    echo "  The game will build in DEV MODE (no multiplayer) without EOS SDK."
fi

# === Build ===
echo ""
echo "Setup complete! To build:"
echo ""
echo "  Desktop:"
echo "    mkdir build && cd build"
echo "    cmake .. -DCMAKE_BUILD_TYPE=Release"
echo "    cmake --build . -j\$(nproc)"
echo ""
echo "  Android:"
echo "    cd android && ./gradlew assembleRelease"
echo ""
echo "=========================================="
