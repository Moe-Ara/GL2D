//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_ANIMATORMANAGER_HPP
#define GL2D_ANIMATORMANAGER_HPP


#include "Graphics/Animation/Animator.hpp"

class AnimatorManager {
public:
    AnimatorManager() = delete;

    virtual ~AnimatorManager() = delete;

    AnimatorManager(const AnimatorManager &other) = delete;

    AnimatorManager &operator=(const AnimatorManager &other) = delete;

    AnimatorManager(AnimatorManager &&other) = delete;

    AnimatorManager &operator=(AnimatorManager &&other) = delete;

    static void registerAnimator(const std::string &id, Graphics::Animator *animator);

    static Graphics::Animator *get(const std::string &id);

    static bool contains(const std::string &id);

private:
    static std::unordered_map<std::string, Graphics::Animator *> s_animators;
};


#endif //GL2D_ANIMATORMANAGER_HPP
