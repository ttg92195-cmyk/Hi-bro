#pragma once
// ============================================================================
// EOS Shooter - AudioManager.h
// Audio system with 3D positional audio, pooling, and music management.
// ============================================================================

#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace EOSShooter {

enum class SoundCategory {
    SFX,
    MUSIC,
    VOICE,
    AMBIENT,
    UI
};

struct SoundEntry {
    Sound sound;
    std::string name;
    SoundCategory category;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    bool is3D = false;
    Vector3 position = {0, 0, 0};
    float maxDistance = 50.0f;
};

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager() = default;

    void Initialize();
    void Shutdown();
    void Update(const Vector3& listenerPosition, const Vector3& listenerForward);

    // === Loading ===
    bool LoadSound2D(const std::string& name, const std::string& filePath, SoundCategory category = SoundCategory::SFX);
    bool LoadSound3D(const std::string& name, const std::string& filePath, float maxDistance = 50.0f);
    bool LoadMusic(const std::string& name, const std::string& filePath);

    // === Playback ===
    void Play(const std::string& name);
    void Play3D(const std::string& name, const Vector3& position);
    void Stop(const std::string& name);
    void StopAll();
    void Pause(const std::string& name);
    void Resume(const std::string& name);

    // === Music ===
    void PlayMusic(const std::string& name, bool loop = true);
    void StopMusic();
    void SetMusicVolume(float volume);

    // === Volume ===
    void SetMasterVolume(float volume);
    void SetCategoryVolume(SoundCategory category, float volume);
    float GetMasterVolume() const { return masterVolume_; }

    // === Utility ===
    bool IsPlaying(const std::string& name) const;
    void UnloadAll();

private:
    std::unordered_map<std::string, SoundEntry> sounds_;
    std::unordered_map<std::string, Music> musicTracks_;
    std::string currentMusic_;
    float masterVolume_ = 1.0f;
    std::unordered_map<int, float> categoryVolumes_;
    Vector3 listenerPos_ = {0, 0, 0};
    Vector3 listenerForward_ = {0, 0, 1};
    bool initialized_ = false;

    float Calculate3DVolume(const Vector3& soundPos, float maxDistance) const;
};

} // namespace EOSShooter
