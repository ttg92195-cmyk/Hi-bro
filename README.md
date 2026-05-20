# EOS Shooter - Multiplayer FPS

A large-scale **C++ first-person shooter** built with **Raylib** and **Epic Online Services (EOS)** for completely free, real-time multiplayer. Cross-platform: **Desktop (Windows/Linux/macOS)** and **Android**.

## Features

### Gameplay
- **5 Game Modes**: Deathmatch, Team Deathmatch, Battle Royale, Co-op Survival, Capture the Flag
- **7 Weapon Types**: Assault Rifle, SMG, Shotgun, Sniper Rifle, Pistol, RPG, Melee
- **5 Enemy Classes** (Co-op): Grunt, Heavy, Sniper, Shotgunner, Rocketeer
- **Attachment System**: Red Dot, ACOG, Suppressor, Grip, Extended Mag, Laser, Flash Hider
- **3 Maps**: Urban Warehouse, Desert Outpost, Arctic Base
- **Wave System**: Progressive difficulty in Co-op Survival mode
- **Pickup System**: Health, Armor, Ammo, and Weapon pickups with respawns

### Multiplayer (EOS)
- **Epic Online Services** - 100% Free, no server costs
- **P2P Networking** - Direct peer-to-peer via EOS
- **Session Browser** - Find and join public games
- **Voice Chat** - Built-in EOS Voice
- **Stats & Achievements** - EOS backend tracking
- **Rich Presence** - Show game status to friends

### Technical
- **C++17** with modern features
- **Raylib** for rendering, audio, input
- **CMake** cross-platform build system
- **GitHub Actions CI/CD** with automatic APK builds
- **Android NDK** support with Gradle build
- **Pool-based Particle System** (5000+ particles)
- **First-person Camera** with recoil, head bob, ADS, shake

## Quick Start

### Prerequisites
- CMake 3.22+
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Raylib 4.5+
- Git

### Desktop Build
```bash
# Clone
git clone https://github.com/YOUR_USERNAME/eos-shooter.git
cd eos-shooter

# Clone Raylib
mkdir -p third_party
git clone --depth 1 https://github.com/raysan5/raylib.git third_party/raylib

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Run
./eos-shooter
```

### Android Build
```bash
# Install Android SDK & NDK
sdkmanager "ndk;25.2.9519653"

# Build APK
cd android
chmod +x gradlew
./gradlew assembleRelease

# APK output: android/app/build/outputs/apk/release/
```

## EOS Setup (Multiplayer)

To enable multiplayer, you need the EOS SDK:

1. Register at [Epic Games Developer Portal](https://dev.epicgames.com/)
2. Create a Product and get credentials:
   - Product ID
   - Sandbox ID
   - Deployment ID
   - Client ID
   - Client Secret
3. Download the [EOS SDK](https://dev.epicgames.com/docs/services/en-US/EpicAccountServices/index.html)
4. Extract to `third_party/eos-sdk/` with structure:
   ```
   third_party/eos-sdk/
   ├── Include/
   │   └── eos_sdk.h
   └── Lib/
       ├── Android/
       │   ├── arm64-v8a/libEOSSDK.a
       │   └── armeabi-v7a/libEOSSDK.a
       ├── Linux/Release/libEOSSDK.a
       ├── Win64/Release/EOSSDK.lib
       └── Mac/Release/libEOSSDK.a
   ```
5. Set credentials in code or environment variables

> **Without EOS SDK**, the game builds in **dev mode** - fully playable single-player/co-op with AI enemies.

## Controls

| Action | Key | Alternative |
|--------|-----|-------------|
| Move | W/A/S/D | Arrow Keys |
| Look | Mouse | - |
| Shoot | Left Click | - |
| Aim (ADS) | Right Click | - |
| Reload | R | - |
| Jump | Space | - |
| Sprint | Left Shift | - |
| Crouch | Left Ctrl | - |
| Weapon 1/2/3 | 1/2/3 | Mouse Wheel |
| Interact | E | - |
| Melee | V | - |
| Grenade | G | - |
| Pause | Escape | - |

## Project Structure

```
eos-shooter/
├── .github/workflows/build.yml    # CI/CD: Auto build APK + Desktop
├── src/
│   ├── main.cpp                   # Entry point
│   ├── game/
│   │   ├── Game.h/.cpp            # Main game class, state machine
│   │   ├── Player.h/.cpp          # Player entity, movement, combat
│   │   ├── Weapon.h/.cpp          # 7 weapon types, attachments, recoil
│   │   ├── Bullet.h/.cpp          # Bullet physics, trails
│   │   ├── Enemy.h/.cpp           # AI state machine, 5 classes
│   │   ├── Map.h/.cpp             # 3 maps, collision, pickups
│   │   ├── ParticleSystem.h/.cpp  # Pool-based particles
│   │   └── CameraController.h/.cpp # FPS camera, recoil, ADS
│   ├── multiplayer/
│   │   ├── EOSManager.h/.cpp      # EOS: Auth, Sessions, P2P, Voice
│   │   └── NetworkManager.h/.cpp  # Network sync, RPCs, lag comp
│   └── utils/
│       ├── Math.h                 # 3D math, spread, interpolation
│       ├── InputManager.h/.cpp    # Rebindable input system
│       └── AudioManager.h/.cpp    # 3D positional audio
├── android/                       # Android build (Gradle + NDK)
│   ├── app/
│   │   ├── build.gradle
│   │   └── src/main/
│   │       ├── AndroidManifest.xml
│   │       └── java/com/eosshooter/GameActivity.java
│   ├── build.gradle
│   └── settings.gradle
├── assets/                        # Game assets
├── CMakeLists.txt                 # CMake build configuration
└── README.md
```

## GitHub Actions CI/CD

Every push automatically builds:
- **Android APK** (arm64-v8a) → Upload as artifact
- **Linux** binary → Upload as artifact
- **Windows** binary → Upload as artifact
- **macOS** binary → Upload as artifact

On version tags (`v1.0.0`), a **GitHub Release** is created with all builds attached.

### Setup APK Signing
Add these secrets to your GitHub repository:
- `SIGNING_KEY_BASE64` - Base64 encoded keystore
- `KEY_ALIAS` - Keystore alias
- `KEY_STORE_PASSWORD` - Keystore password
- `KEY_PASSWORD` - Key password

## License

MIT License - See LICENSE file for details.

## Credits

- [Raylib](https://www.raylib.com/) - Simple and easy-to-use library for videogames programming
- [Epic Online Services](https://dev.epicgames.com/) - Free multiplayer services
- [Android NDK](https://developer.android.com/ndk) - Native development for Android
