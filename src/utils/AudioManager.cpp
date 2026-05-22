// ============================================================================
// EOS Shooter - AudioManager.cpp
// ============================================================================

#include "AudioManager.h"
#include <cmath>
#include <algorithm>

namespace EOSShooter {

void AudioManager::Initialize() {
    categoryVolumes_[(int)SoundCategory::SFX] = 1.0f;
    categoryVolumes_[(int)SoundCategory::MUSIC] = 0.7f;
    categoryVolumes_[(int)SoundCategory::VOICE] = 1.0f;
    categoryVolumes_[(int)SoundCategory::AMBIENT] = 0.5f;
    categoryVolumes_[(int)SoundCategory::UI] = 0.8f;
    initialized_ = true;
}

void AudioManager::Shutdown() {
    UnloadAll();
    initialized_ = false;
}

void AudioManager::Update(const Vector3& listenerPosition, const Vector3& listenerForward) {
    listenerPos_ = listenerPosition;
    listenerForward_ = listenerForward;

    // Update music stream
    if (!currentMusic_.empty()) {
        auto it = musicTracks_.find(currentMusic_);
        if (it != musicTracks_.end()) {
            UpdateMusicStream(it->second);
        }
    }
}

bool AudioManager::LoadSound2D(const std::string& name, const std::string& filePath,
                                 SoundCategory category) {
    if (!FileExists(filePath.c_str())) {
        TraceLog(LOG_WARNING, "Audio: File not found: %s", filePath.c_str());
        return false;
    }

    SoundEntry entry;
    entry.sound = LoadSound(filePath.c_str());
    entry.name = name;
    entry.category = category;
    entry.is3D = false;
    sounds_[name] = entry;

    TraceLog(LOG_INFO, "Audio: Loaded 2D sound '%s' from %s", name.c_str(), filePath.c_str());
    return true;
}

bool AudioManager::LoadSound3D(const std::string& name, const std::string& filePath,
                                 float maxDistance) {
    if (!FileExists(filePath.c_str())) {
        TraceLog(LOG_WARNING, "Audio: File not found: %s", filePath.c_str());
        return false;
    }

    SoundEntry entry;
    entry.sound = LoadSound(filePath.c_str());
    entry.name = name;
    entry.category = SoundCategory::SFX;
    entry.is3D = true;
    entry.maxDistance = maxDistance;
    sounds_[name] = entry;

    TraceLog(LOG_INFO, "Audio: Loaded 3D sound '%s' (max dist: %.0f)", name.c_str(), maxDistance);
    return true;
}

bool AudioManager::LoadMusic(const std::string& name, const std::string& filePath) {
    if (!FileExists(filePath.c_str())) {
        TraceLog(LOG_WARNING, "Audio: Music file not found: %s", filePath.c_str());
        return false;
    }

    musicTracks_[name] = LoadMusicStream(filePath.c_str());
    TraceLog(LOG_INFO, "Audio: Loaded music '%s'", name.c_str());
    return true;
}

void AudioManager::Play(const std::string& name) {
    auto it = sounds_.find(name);
    if (it == sounds_.end()) return;

    float volume = masterVolume_ * categoryVolumes_[(int)it->second.category];
    SetSoundVolume(it->second.sound, volume * it->second.volume);
    SetSoundPitch(it->second.sound, it->second.pitch);
    PlaySound(it->second.sound);
}

void AudioManager::Play3D(const std::string& name, const Vector3& position) {
    auto it = sounds_.find(name);
    if (it == sounds_.end()) return;

    float volume = Calculate3DVolume(position, it->second.maxDistance);
    float finalVolume = volume * masterVolume_ * categoryVolumes_[(int)it->second.category];
    SetSoundVolume(it->second.sound, finalVolume);
    PlaySound(it->second.sound);
}

void AudioManager::Stop(const std::string& name) {
    auto it = sounds_.find(name);
    if (it != sounds_.end()) {
        StopSound(it->second.sound);
    }
}

void AudioManager::StopAll() {
    for (auto& [name, entry] : sounds_) {
        StopSound(entry.sound);
    }
    StopMusic();
}

void AudioManager::Pause(const std::string& name) {
    auto it = sounds_.find(name);
    if (it != sounds_.end()) {
        PauseSound(it->second.sound);
    }
}

void AudioManager::Resume(const std::string& name) {
    auto it = sounds_.find(name);
    if (it != sounds_.end()) {
        ResumeSound(it->second.sound);
    }
}

void AudioManager::PlayMusic(const std::string& name, bool loop) {
    auto it = musicTracks_.find(name);
    if (it == musicTracks_.end()) return;

    // Stop current music
    if (!currentMusic_.empty() && currentMusic_ != name) {
        auto current = musicTracks_.find(currentMusic_);
        if (current != musicTracks_.end()) {
            StopMusicStream(current->second);
        }
    }

    it->second.looping = loop;
    ::SetMusicVolume(it->second, masterVolume_ * categoryVolumes_[(int)SoundCategory::MUSIC]);
    PlayMusicStream(it->second);
    currentMusic_ = name;
}

void AudioManager::StopMusic() {
    if (currentMusic_.empty()) return;
    auto it = musicTracks_.find(currentMusic_);
    if (it != musicTracks_.end()) {
        StopMusicStream(it->second);
    }
    currentMusic_.clear();
}

void AudioManager::SetMusicVolume(float volume) {
    categoryVolumes_[(int)SoundCategory::MUSIC] = volume;
    if (!currentMusic_.empty()) {
        auto it = musicTracks_.find(currentMusic_);
        if (it != musicTracks_.end()) {
            ::SetMusicVolume(it->second, masterVolume_ * volume);
        }
    }
}

void AudioManager::SetMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::SetCategoryVolume(SoundCategory category, float volume) {
    categoryVolumes_[(int)category] = std::clamp(volume, 0.0f, 1.0f);
}

bool AudioManager::IsPlaying(const std::string& name) const {
    auto it = sounds_.find(name);
    if (it != sounds_.end()) {
        return IsSoundPlaying(it->second.sound);
    }
    return false;
}

void AudioManager::UnloadAll() {
    for (auto& [name, entry] : sounds_) {
        UnloadSound(entry.sound);
    }
    sounds_.clear();

    for (auto& [name, music] : musicTracks_) {
        UnloadMusicStream(music);
    }
    musicTracks_.clear();
    currentMusic_.clear();
}

float AudioManager::Calculate3DVolume(const Vector3& soundPos, float maxDistance) const {
    float dx = soundPos.x - listenerPos_.x;
    float dy = soundPos.y - listenerPos_.y;
    float dz = soundPos.z - listenerPos_.z;
    float distance = sqrtf(dx * dx + dy * dy + dz * dz);

    if (distance >= maxDistance) return 0.0f;
    if (distance <= 1.0f) return 1.0f;

    // Inverse distance rolloff
    float attenuation = 1.0f / (1.0f + (distance / maxDistance) * (distance / maxDistance) * 4.0f);

    // Directional attenuation (sounds behind listener are slightly quieter)
    Vector3 toSound = {dx / distance, dy / distance, dz / distance};
    float dot = toSound.x * listenerForward_.x + toSound.z * listenerForward_.z;
    float directionalMult = 0.7f + 0.3f * std::clamp(dot, 0.0f, 1.0f);

    return attenuation * directionalMult;
}

} // namespace EOSShooter
