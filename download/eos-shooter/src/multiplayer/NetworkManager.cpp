// ============================================================================
// EOS Shooter - NetworkManager.cpp
// ============================================================================

#include "NetworkManager.h"
#include <cstring>
#include <chrono>
#include <algorithm>

namespace EOSShooter {

bool NetworkManager::Initialize(EOSManager* eosManager) {
    eosManager_ = eosManager;
    initialized_ = true;

    outSequence_ = 0;
    updateTimer_ = 0.0f;
    packetsSent_ = 0;
    packetsReceived_ = 0;

    TraceLog(LOG_INFO, "Network: Initialized (update rate: %.0f Hz)", updateRate_);
    return true;
}

void NetworkManager::Shutdown() {
    eosManager_ = nullptr;
    initialized_ = false;

    // Clear message queue
    while (!incomingMessages_.empty()) {
        incomingMessages_.pop();
    }

    TraceLog(LOG_INFO, "Network: Shut down");
}

void NetworkManager::ProcessMessages() {
    if (!initialized_ || !eosManager_) return;

    // Process incoming EOS P2P packets
    ProcessIncomingPackets();

    // Process all queued messages
    while (!incomingMessages_.empty()) {
        NetMessage msg = incomingMessages_.front();
        incomingMessages_.pop();
        HandleMessage(msg);
    }

    // Update send timer for regular position updates
    updateTimer_ += GetFrameTime();
}

// ============================================================================
// Sending
// ============================================================================

void NetworkManager::SendPlayerUpdate(const Vector3& pos, const Vector3& rot) {
    if (!eosManager_ || !eosManager_->IsInSession()) return;

    // Throttle position updates
    if (updateTimer_ < updateInterval_) return;
    updateTimer_ = 0.0f;

    NetPlayerUpdateData data = {};
    data.position = pos;
    data.rotation = rot;
    data.velocity = {0, 0, 0};
    data.moveState = 0;
    data.health = 100.0f;
    data.armor = 0.0f;
    data.currentWeapon = 0;
    data.isShooting = false;
    data.isAiming = false;

    SendNetMessage(NetMessageType::PLAYER_UPDATE,
                   reinterpret_cast<const uint8_t*>(&data), sizeof(data), false);
}

void NetworkManager::SendPlayerShot(const Vector3& origin, const Vector3& direction) {
    if (!eosManager_ || !eosManager_->IsInSession()) return;

    NetPlayerShotData data = {};
    data.origin = origin;
    data.direction = direction;

    SendNetMessage(NetMessageType::PLAYER_SHOT,
                   reinterpret_cast<const uint8_t*>(&data), sizeof(data), true);
}

void NetworkManager::SendPlayerHit(uint64_t victimId, float damage, uint64_t attackerId) {
    if (!eosManager_ || !eosManager_->IsInSession()) return;

    NetPlayerHitData data = {};
    data.victimId = victimId;
    data.damage = damage;
    data.attackerId = attackerId;

    SendNetMessage(NetMessageType::PLAYER_HIT,
                   reinterpret_cast<const uint8_t*>(&data), sizeof(data), true);
}

void NetworkManager::SendChatMessage(const std::string& message) {
    if (!eosManager_ || !eosManager_->IsInSession()) return;

    SendNetMessage(NetMessageType::PLAYER_CHAT,
                   reinterpret_cast<const uint8_t*>(message.c_str()),
                   (uint32_t)message.size() + 1, true);
}

void NetworkManager::SendPing() {
    if (!eosManager_ || !eosManager_->IsInSession()) return;

    lastPingTime_ = GetTime();
    float timestamp = lastPingTime_;

    SendNetMessage(NetMessageType::PING,
                   reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp), false);
}

// ============================================================================
// Internal Processing
// ============================================================================

void NetworkManager::ProcessIncomingPackets() {
    if (!eosManager_) return;

    uint8_t buffer[4096];
    uint32_t size = 0;
    uint64_t peerId = 0;

    // Process all available packets from EOS P2P
    while (eosManager_->ReceivePacket(channelId_, buffer, size, peerId)) {
        packetsReceived_++;

        if (size < sizeof(NetMessageHeader)) {
            TraceLog(LOG_WARNING, "Network: Received undersized packet (%u bytes)", size);
            continue;
        }

        // Parse header
        NetMessage msg;
        std::memcpy(&msg.header, buffer, sizeof(NetMessageHeader));

        // Parse payload
        uint32_t payloadSize = size - sizeof(NetMessageHeader);
        if (payloadSize > 0) {
            msg.payload.resize(payloadSize);
            std::memcpy(msg.payload.data(), buffer + sizeof(NetMessageHeader), payloadSize);
        }

        // Sequence check (drop old/duplicate packets)
        auto it = lastReceivedSeq_.find(peerId);
        if (it != lastReceivedSeq_.end()) {
            if (msg.header.sequenceNumber <= it->second) {
                continue; // Drop old packet
            }
        }
        lastReceivedSeq_[peerId] = msg.header.sequenceNumber;

        incomingMessages_.push(msg);
    }
}

void NetworkManager::HandleMessage(const NetMessage& msg) {
    switch (msg.header.type) {
    case NetMessageType::PLAYER_JOIN: {
        if (msg.payload.size() >= sizeof(NetPlayerJoinData)) {
            NetPlayerJoinData data;
            std::memcpy(&data, msg.payload.data(), sizeof(data));
            if (onPlayerJoined_) {
                onPlayerJoined_(data.playerId, std::string(data.name));
            }
        }
        break;
    }

    case NetMessageType::PLAYER_LEAVE: {
        if (msg.payload.size() >= sizeof(NetPlayerLeaveData)) {
            NetPlayerLeaveData data;
            std::memcpy(&data, msg.payload.data(), sizeof(data));
            if (onPlayerLeft_) {
                onPlayerLeft_(data.playerId);
            }
        }
        break;
    }

    case NetMessageType::PLAYER_UPDATE: {
        if (msg.payload.size() >= sizeof(NetPlayerUpdateData)) {
            NetPlayerUpdateData data;
            std::memcpy(&data, msg.payload.data(), sizeof(data));
            if (onPlayerUpdate_) {
                onPlayerUpdate_(data.playerId, data.position, data.rotation);
            }
        }
        break;
    }

    case NetMessageType::PLAYER_SHOT: {
        if (msg.payload.size() >= sizeof(NetPlayerShotData)) {
            NetPlayerShotData data;
            std::memcpy(&data, msg.payload.data(), sizeof(data));
            if (onPlayerShot_) {
                onPlayerShot_(data.playerId, data.origin, data.direction);
            }
        }
        break;
    }

    case NetMessageType::PLAYER_HIT: {
        if (msg.payload.size() >= sizeof(NetPlayerHitData)) {
            NetPlayerHitData data;
            std::memcpy(&data, msg.payload.data(), sizeof(data));
            if (onPlayerHit_) {
                onPlayerHit_(data.victimId, data.damage, data.attackerId);
            }
        }
        break;
    }

    case NetMessageType::PING: {
        // Respond with PONG
        float timestamp;
        std::memcpy(&timestamp, msg.payload.data(), sizeof(float));
        SendNetMessage(NetMessageType::PONG,
                       reinterpret_cast<const uint8_t*>(&timestamp), sizeof(timestamp), false);
        break;
    }

    case NetMessageType::PONG: {
        float sentTime;
        std::memcpy(&sentTime, msg.payload.data(), sizeof(float));
        pingMs_ = (GetTime() - sentTime) * 500.0f; // RTT / 2
        break;
    }

    default:
        break;
    }
}

void NetworkManager::SendNetMessage(NetMessageType type, const uint8_t* data,
                                     uint32_t size, bool reliable) {
    if (!eosManager_) return;

    // Build packet: header + payload
    std::vector<uint8_t> packet(sizeof(NetMessageHeader) + size);

    NetMessageHeader header;
    header.type = type;
    header.senderId = 0; // Would be set from EOS user ID
    header.sequenceNumber = outSequence_++;
    header.timestamp = GetTime();

    std::memcpy(packet.data(), &header, sizeof(NetMessageHeader));
    if (size > 0) {
        std::memcpy(packet.data() + sizeof(NetMessageHeader), data, size);
    }

    eosManager_->SendPacket(channelId_, packet.data(), (uint32_t)packet.size(), reliable);
    packetsSent_++;
}

float NetworkManager::GetTime() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::duration<float>>(epoch);
    return seconds.count();
}

} // namespace EOSShooter
