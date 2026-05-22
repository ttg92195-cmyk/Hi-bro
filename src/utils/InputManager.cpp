// ============================================================================
// EOS Shooter - InputManager.cpp
// ============================================================================

#include "InputManager.h"

namespace EOSShooter {

void InputManager::Initialize() {
    SetupDefaultBindings();
}

void InputManager::Update() {
    // Store previous frame state for just-pressed/just-released detection
    for (auto& [key, binding] : bindings_) {
        previousState_[key] = IsActionPressed(binding.action);
    }
}

void InputManager::Shutdown() {
    bindings_.clear();
    previousState_.clear();
    callbacks_.clear();
}

void InputManager::SetupDefaultBindings() {
    bindings_[ActionToKey(InputAction::MOVE_FORWARD)]   = {InputAction::MOVE_FORWARD,   KEY_W,    KEY_UP};
    bindings_[ActionToKey(InputAction::MOVE_BACKWARD)]  = {InputAction::MOVE_BACKWARD,  KEY_S,    KEY_DOWN};
    bindings_[ActionToKey(InputAction::MOVE_LEFT)]      = {InputAction::MOVE_LEFT,      KEY_A,    KEY_LEFT};
    bindings_[ActionToKey(InputAction::MOVE_RIGHT)]     = {InputAction::MOVE_RIGHT,     KEY_D,    KEY_RIGHT};
    bindings_[ActionToKey(InputAction::JUMP)]           = {InputAction::JUMP,           KEY_SPACE};
    bindings_[ActionToKey(InputAction::SPRINT)]         = {InputAction::SPRINT,         KEY_LEFT_SHIFT, -1, -1, true};
    bindings_[ActionToKey(InputAction::CROUCH)]         = {InputAction::CROUCH,         KEY_LEFT_CONTROL, -1, -1, true};
    bindings_[ActionToKey(InputAction::SHOOT)]          = {InputAction::SHOOT,          -1, -1, MOUSE_BUTTON_LEFT, true};
    bindings_[ActionToKey(InputAction::AIM)]            = {InputAction::AIM,            -1, -1, MOUSE_BUTTON_RIGHT, true};
    bindings_[ActionToKey(InputAction::RELOAD)]         = {InputAction::RELOAD,         KEY_R};
    bindings_[ActionToKey(InputAction::SWITCH_WEAPON_1)] = {InputAction::SWITCH_WEAPON_1, KEY_ONE};
    bindings_[ActionToKey(InputAction::SWITCH_WEAPON_2)] = {InputAction::SWITCH_WEAPON_2, KEY_TWO};
    bindings_[ActionToKey(InputAction::SWITCH_WEAPON_3)] = {InputAction::SWITCH_WEAPON_3, KEY_THREE};
    bindings_[ActionToKey(InputAction::INTERACT)]       = {InputAction::INTERACT,       KEY_E};
    bindings_[ActionToKey(InputAction::MELEE)]          = {InputAction::MELEE,          KEY_V};
    bindings_[ActionToKey(InputAction::GRENADE)]        = {InputAction::GRENADE,        KEY_G};
    bindings_[ActionToKey(InputAction::SCOREBOARD)]     = {InputAction::SCOREBOARD,     KEY_TAB, -1, -1, true};
    bindings_[ActionToKey(InputAction::CHAT)]           = {InputAction::CHAT,           KEY_T};
    bindings_[ActionToKey(InputAction::PAUSE)]          = {InputAction::PAUSE,          KEY_ESCAPE};
    bindings_[ActionToKey(InputAction::MAP)]            = {InputAction::MAP,            KEY_M};
}

bool InputManager::IsActionPressed(InputAction action) const {
    auto it = bindings_.find(ActionToKey(action));
    if (it == bindings_.end()) return false;

    const auto& binding = it->second;
    if (binding.primaryKey >= 0 && IsKeyDown(binding.primaryKey)) return true;
    if (binding.secondaryKey >= 0 && IsKeyDown(binding.secondaryKey)) return true;
    if (binding.mouseButton >= 0 && IsMouseButtonDown(binding.mouseButton)) return true;
    return false;
}

bool InputManager::IsActionJustPressed(InputAction action) const {
    auto it = bindings_.find(ActionToKey(action));
    if (it == bindings_.end()) return false;

    const auto& binding = it->second;
    bool current = false;

    if (binding.primaryKey >= 0 && IsKeyPressed(binding.primaryKey)) current = true;
    if (binding.secondaryKey >= 0 && IsKeyPressed(binding.secondaryKey)) current = true;
    if (binding.mouseButton >= 0 && IsMouseButtonPressed(binding.mouseButton)) current = true;

    return current;
}

bool InputManager::IsActionJustReleased(InputAction action) const {
    auto it = bindings_.find(ActionToKey(action));
    if (it == bindings_.end()) return false;

    const auto& binding = it->second;
    if (binding.primaryKey >= 0 && IsKeyReleased(binding.primaryKey)) return true;
    if (binding.secondaryKey >= 0 && IsKeyReleased(binding.secondaryKey)) return true;
    if (binding.mouseButton >= 0 && IsMouseButtonReleased(binding.mouseButton)) return true;
    return false;
}

float InputManager::GetActionValue(InputAction action) const {
    return IsActionPressed(action) ? 1.0f : 0.0f;
}

Vector2 InputManager::GetMouseDelta() const {
    return ::GetMouseDelta(); // Call raylib's global GetMouseDelta()
}

float InputManager::GetMouseScroll() const {
    return GetMouseWheelMove();
}

void InputManager::BindAction(InputAction action, int key, int secondaryKey, int mouseBtn) {
    bindings_[ActionToKey(action)] = {action, key, secondaryKey, mouseBtn};
}

void InputManager::ResetBindings() {
    bindings_.clear();
    SetupDefaultBindings();
}

const InputBinding& InputManager::GetBinding(InputAction action) const {
    return bindings_.at(ActionToKey(action));
}

void InputManager::AddActionCallback(InputAction action, ActionCallback callback) {
    callbacks_[ActionToKey(action)].push_back(callback);
}

} // namespace EOSShooter
