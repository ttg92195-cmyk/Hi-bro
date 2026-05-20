#pragma once
// ============================================================================
// EOS Shooter - InputManager.h
// Centralized input handling with rebindable keys and input buffering.
// ============================================================================

#include "raylib.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

namespace EOSShooter {

enum class InputAction {
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    JUMP,
    SPRINT,
    CROUCH,
    SHOOT,
    AIM,
    RELOAD,
    SWITCH_WEAPON_1,
    SWITCH_WEAPON_2,
    SWITCH_WEAPON_3,
    INTERACT,
    MELEE,
    GRENADE,
    SCOREBOARD,
    CHAT,
    PAUSE,
    MAP
};

struct InputBinding {
    InputAction action;
    int primaryKey;
    int secondaryKey;
    int mouseButton = -1;
    bool requiresHold = false;
};

class InputManager {
public:
    InputManager() = default;
    ~InputManager() = default;

    void Initialize();
    void Update();
    void Shutdown();

    // === Query ===
    bool IsActionPressed(InputAction action) const;
    bool IsActionJustPressed(InputAction action) const;
    bool IsActionJustReleased(InputAction action) const;
    float GetActionValue(InputAction action) const;

    // === Mouse ===
    Vector2 GetMouseDelta() const;
    float GetMouseScroll() const;
    void SetMouseSensitivity(float sens) { sensitivity_ = sens; }
    float GetMouseSensitivity() const { return sensitivity_; }

    // === Binding ===
    void BindAction(InputAction action, int key, int secondaryKey = -1, int mouseBtn = -1);
    void ResetBindings();
    const InputBinding& GetBinding(InputAction action) const;

    // === Callbacks ===
    using ActionCallback = std::function<void(InputAction)>;
    void AddActionCallback(InputAction action, ActionCallback callback);

private:
    std::unordered_map<int, InputBinding> bindings_;
    std::unordered_map<int, bool> previousState_;
    float sensitivity_ = 0.003f;

    // Callbacks per action
    std::unordered_map<int, std::vector<ActionCallback>> callbacks_;

    void SetupDefaultBindings();
    int ActionToKey(InputAction action) const { return static_cast<int>(action); }
};

} // namespace EOSShooter
