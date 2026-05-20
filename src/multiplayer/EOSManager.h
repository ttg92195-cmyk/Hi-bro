#pragma once
// ============================================================================
// EOS Shooter - EOSManager.h
// Epic Online Services (EOS) integration - completely free multiplayer.
// Handles authentication, sessions, P2P networking, and voice chat.
// ============================================================================

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace EOSShooter {

// EOS session info for browsing
struct SessionInfo {
    std::string sessionId;
    std::string name;
    std::string hostName;
    int currentPlayers = 0;
    int maxPlayers = 16;
    std::string mapName;
    int gameMode = 0;
    bool hasPassword = false;
    float ping = 0.0f;
};

// EOS product/user IDs
struct EOSUserData {
    std::string epicAccountId;
    std::string productId;
    std::string displayName;
    bool isLoggedIn = false;
};

// Callback types
using LoginCallback = std::function<void(bool success, const std::string& error)>;
using CreateSessionCallback = std::function<void(bool success, const std::string& sessionId)>;
using FindSessionsCallback = std::function<void(const std::vector<SessionInfo>& sessions)>;
using JoinSessionCallback = std::function<void(bool success, const std::string& error)>;

class EOSManager {
public:
    EOSManager() = default;
    ~EOSManager();

    // === Lifecycle ===
    bool Initialize();
    void Shutdown();
    void Tick();  // Must be called every frame

    // === Configuration ===
    void SetProductUserId(const std::string& id) { productUserId_ = id; }
    void SetDeploymentId(const std::string& id) { deploymentId_ = id; }
    void SetClientId(const std::string& id) { clientId_ = id; }
    void SetClientSecret(const std::string& secret) { clientSecret_ = secret; }

    // === Authentication ===
    void LoginWithDeviceId(const std::string& displayName, LoginCallback callback);
    void LoginWithAccountPortal(LoginCallback callback);
    void LoginWithDevTool(const std::string& userName, LoginCallback callback);
    void Logout();
    bool IsLoggedIn() const { return userData_.isLoggedIn; }
    const EOSUserData& GetUserData() const { return userData_; }

    // === Sessions ===
    bool CreateSession(const std::string& sessionName, int maxPlayers,
                       const std::string& mapName = "", int gameMode = 0);
    bool JoinSession(const std::string& sessionId);
    void LeaveSession();
    void DestroySession();
    void FindSessions();
    void UpdateSession(const std::string& key, const std::string& value);
    const std::vector<SessionInfo>& GetFoundSessions() const { return foundSessions_; }
    bool IsInSession() const { return inSession_; }
    const std::string& GetCurrentSessionId() const { return currentSessionId_; }

    // === P2P Networking ===
    bool SendPacket(const std::string& channelId, const uint8_t* data, uint32_t size,
                    bool reliable = true);
    bool ReceivePacket(const std::string& channelId, uint8_t* buffer, uint32_t& outSize,
                       uint64_t& outPeerId);
    void CloseConnection(uint64_t peerId);

    // === Voice Chat ===
    bool EnableVoiceChat(bool enabled);
    void SetVoiceChatActive(bool active);
    bool IsVoiceChatEnabled() const { return voiceChatEnabled_; }
    bool IsVoiceChatActive() const { return voiceChatActive_; }

    // === Stats / Achievements ===
    void SubmitStat(const std::string& statName, int value);
    void UnlockAchievement(const std::string& achievementId);
    void QueryStats();

    // === Presence ===
    void SetPresence(const std::string& status, const std::string& richPresence);

private:
    bool initialized_ = false;
    bool inSession_ = false;
    std::string currentSessionId_;
    std::string productUserId_;
    std::string deploymentId_;
    std::string clientId_;
    std::string clientSecret_;
    EOSUserData userData_;
    std::vector<SessionInfo> foundSessions_;

    // Voice chat state
    bool voiceChatEnabled_ = false;
    bool voiceChatActive_ = false;

    // Internal EOS platform handle (opaque)
    void* platformHandle_ = nullptr;

    // Callbacks
    LoginCallback loginCallback_;
    CreateSessionCallback createSessionCallback_;
    FindSessionsCallback findSessionsCallback_;
    JoinSessionCallback joinSessionCallback_;

    // Internal methods
    void ProcessLoginResult(bool success, const std::string& error);
    void ProcessSessionCreation(bool success, const std::string& sessionId);
    void ProcessSessionSearch(const std::vector<SessionInfo>& results);
    void ProcessJoinResult(bool success, const std::string& error);
};

} // namespace EOSShooter
