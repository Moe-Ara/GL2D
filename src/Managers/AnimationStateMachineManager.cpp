//
// Created by Mohamad on 21/11/2025.
//

#include "AnimationStateMachineManager.hpp"

#include <stdexcept>

std::unordered_map<std::string, std::weak_ptr<Graphics::AnimationStateMachine>>
    AnimationStateMachineManager::s_stateMachines{};

void AnimationStateMachineManager::registerStateMachine(
        const std::string& id,
        const std::shared_ptr<Graphics::AnimationStateMachine>& stateMachine) {
    if (id.empty()) {
        throw std::invalid_argument(
            "AnimationStateMachineManager asset id cannot be empty");
    }
    if (!stateMachine) {
        throw std::invalid_argument(
            "AnimationStateMachineManager cannot register a null state machine for id: " +
            id);
    }
    if (auto existing = get(id); existing && existing != stateMachine) {
        throw std::invalid_argument(
            "AnimationStateMachineManager asset id is already registered: " + id);
    }
    s_stateMachines[id] = stateMachine;
}

std::shared_ptr<Graphics::AnimationStateMachine>
AnimationStateMachineManager::get(const std::string& id) {
    auto it = s_stateMachines.find(id);
    if (it == s_stateMachines.end()) {
        return nullptr;
    }
    auto stateMachine = it->second.lock();
    if (!stateMachine) {
        s_stateMachines.erase(it);
    }
    return stateMachine;
}

bool AnimationStateMachineManager::contains(const std::string &id) {
    return static_cast<bool>(get(id));
}

bool AnimationStateMachineManager::unregisterStateMachine(
        const std::string& id) {
    return s_stateMachines.erase(id) != 0u;
}

void AnimationStateMachineManager::clear() noexcept {
    s_stateMachines.clear();
}
