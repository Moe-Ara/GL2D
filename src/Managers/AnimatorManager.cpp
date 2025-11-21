//
// Created by Mohamad on 21/11/2025.
//

#include "AnimatorManager.hpp"
#include <string>
#include <unordered_map>
std::unordered_map<std::string, Graphics::Animator*> AnimatorManager::s_animators{};

void AnimatorManager::registerAnimator(const std::string& id, Graphics::Animator* animator) {
    s_animators[id] = animator;
}

Graphics::Animator* AnimatorManager::get(const std::string& id) {
    auto it = s_animators.find(id);
    if (it != s_animators.end()) {
        return it->second;
    }
    return nullptr;
}

bool AnimatorManager::contains(const std::string& id) {
#if __cplusplus >= 202002L
    return s_animators.contains(id);
#else
    return s_animators.find(id) != s_animators.end();
#endif
}