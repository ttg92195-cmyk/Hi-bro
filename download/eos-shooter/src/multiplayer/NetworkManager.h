#pragma once
// ============================================================================
// EOS Shooter - NetworkManager.h
// Network synchronization layer on top of EOS P2P.
// Handles entity replication, RPCs, and lag compensation.
// ============================================================================

#include "raylib.h"
#include "EOSManager.h"
#include <vector>
#include <queue>
#include <functional>
#include <unordered_map>
#include <cstdint>
#include <string>

namespace EOSShooter {

// Network message types
enum class NetMessageType : uint8_t {
    PLAYER_JOIN = 1,
    PLAYER_LEAVE = 2,
    PLAYER_UPDATE = 3,
    PLAYER_SHOT = 4,
    PLAYER_HIT = 5,
    PLAYER_DEATH = 6,
    PLAYER_RESPAWN = 7,
    PLAYER_CHAT = 8,
    ENEMY_UPDATE = 9,
    GAME_STATE = 10,
    PING = 11,
    PONG = 12,
    VOICE_DATA = 13,
    SESSION_SETTINGS = 14
};

// Network message header
struct NetMessageHeader {
    NetMessageType type;
    uint64_t senderId;
    uint32_t sequenceNumber;
    float timestamp;
};

// Complete network message
struct NetMessage {
    NetMessageHeader header;
    std::vector<uint8_t> payload;
};

// Player join/leave data
struct NetPlayerJoinData {
    uint64_t playerId;
    char name[32];
    Vector3 spawnPosition;
};

struct NetPlayerLeaveData {
    uint64_t playerId;
};

// Player transform update (sent at 20Hz)
struct NetPlayerUpdateData {
    uint64_t playerId;
    Vector3 position;
    Vector3 rotation;
    Vector3 velocity;
    uint8_t moveState;
    float health;
    float armor;
    uint8_t currentWeapon;
    bool isShooting;
    bool isAiming;
};

// Shot event data
struct NetPlayerShotData {
    uint64_t playerId;
    Vector3 origin;
    Vector3 direction;
    uint8_t weaponType;
};

// Hit event data
struct NetPlayerHitData {
    uint64_t victimId;
    float damage;
    uint64_t attackerId;
};

// Callback types for network events
using PlayerJoinedCallback = std::function<void(uint64_t, const std::string&)>;
using PlayerLeftCallback = std::function<void(uint64_t)>;
using PlayerUpdateCallback = std::function<void(uint64_t, const Vector3&, const Vector3&)>;
using PlayerShotCallback = std::function<void(uint64_t, const Vector3&, const Vector3&)>;
using PlayerHitCallback = std::function<void(uint64_t, float, uint64_t)>;

class NetworkManager {
public:
    NetworkManager() = default;
    ~NetworkManager() = default;

    // === Lifecycle ===
    bool Initialize(EOSManager* eosManager);
    void Shutdown();
    void ProcessMessages();

    // === Sending ===
    void SendPlayerUpdate(const Vector3& pos, const Vector3& rot);
    void SendPlayerShot(const Vector3& origin, const Vector3& direction);
    void SendPlayerHit(uint64_t victimId, float damage, uint64_t attackerId);
    void SendChatMessage(const std::string& message);
    void SendPing();

    // === Callbacks ===
    void SetOnPlayerJoinedCallback(PlayerJoinedCallback cb) { onPlayerJoined_ = cb; }
    void SetOnPlayerLeftCallback(PlayerLeftCallback cb) { onPlayerLeft_ = cb; }
    void SetOnPlayerUpdateCallback(PlayerUpdateCallback cb) { onPlayerUpdate_ = cb; }
    void SetOnPlayerShotCallback(PlayerShotCallback cb) { onPlayerShot_ = cb; }
    void SetOnPlayerHitCallback(PlayerHitCallback cb) { onPlayerHit_ = cb; }

    // === Stats ===
    float GetPing() const { return pingMs_; }
    int GetPacketsSent() const { return packetsSent_; }
    int GetPacketsReceived() const { return packetsReceived_; }
    float GetPacketLoss() const { return packetLoss_; }

    // === Configuration ===
    void SetUpdateRate(float hz) { updateRate_ = hz; updateInterval_ = 1.0f / hz; }
    void SetChannelId(const std::string& id) { channelId_ = id; }

private:
    EOSManager* eosManager_ = nullptr;
    bool initialized_ = false;

    // Update rate
    float updateRate_ = 20.0f;          // 20Hz position updates
    float updateInterval_ = 0.05f;
    float updateTimer_ = 0.0f;

    // Sequence numbering
    uint32_t outSequence_ = 0;
    std::unordered_map<uint64_t, uint32_t> lastReceivedSeq_;

    // Ping
    float pingMs_ = 0.0f;
    float lastPingTime_ = 0.0f;

    // Stats
    int packetsSent_ = 0;
    int packetsReceived_ = 0;
    float packetLoss_ = 0.0f;

    // Channel
    std::string channelId_ = "game";

    // Message queue
    std::queue<NetMessage> incomingMessages_;

    // Callbacks
    PlayerJoinedCallback onPlayerJoined_;
    PlayerLeftCallback onPlayerLeft_;
    PlayerUpdateCallback onPlayerUpdate_;
    PlayerShotCallback onPlayerShot_;
    PlayerHitCallback onPlayerHit_;

    // Internal
    void ProcessIncomingPackets();
    void HandleMessage(const NetMessage& msg);
    void SendNetMessage(NetMessageType type, const uint8_t* data, uint32_t size, bool reliable = true);
    float GetTime() const;
};

} // namespace EOSShooter
