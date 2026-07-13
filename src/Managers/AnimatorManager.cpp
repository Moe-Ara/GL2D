//
// Created by Mohamad on 21/11/2025.
//

#include "AnimatorManager.hpp"
#include <stdexcept>

std::unordered_map<std::string, std::weak_ptr<Graphics::Animator>>
    AnimatorManager::s_animators{};

void AnimatorManager::registerAnimator(
        const std::string& id,
        const std::shared_ptr<Graphics::Animator>& animator) {
    if (id.empty()) {
        throw std::invalid_argument("AnimatorManager asset id cannot be empty");
    }
    if (!animator) {
        throw std::invalid_argument(
            "AnimatorManager cannot register a null animator for id: " + id);
    }
    if (auto existing = get(id); existing && existing != animator) {
        throw std::invalid_argument(
            "AnimatorManager asset id is already registered: " + id);
    }
    s_animators[id] = animator;
}

std::shared_ptr<Graphics::Animator> AnimatorManager::get(const std::string& id) {
    auto it = s_animators.find(id);
    if (it == s_animators.end()) {
        return nullptr;
    }
    auto animator = it->second.lock();
    if (!animator) {
        s_animators.erase(it);
    }
    return animator;
}

bool AnimatorManager::contains(const std::string& id) {
    return static_cast<bool>(get(id));
}

bool AnimatorManager::unregisterAnimator(const std::string& id) {
    return s_animators.erase(id) != 0u;
}

void AnimatorManager::clear() noexcept {
    s_animators.clear();
}
