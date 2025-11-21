//
// Created by Mohamad on 21/11/2025.
//

#include "AnimatorComponent.hpp"
#include "Graphics/Animation/Animator.hpp"

AnimatorComponent::AnimatorComponent(Graphics::Animator *animator)
    : m_animator(animator) {}

void AnimatorComponent::update(Entity &/*owner*/, double dt) {
    if (m_animator) {
        // dt is double in components; Animator expects float
        m_animator->update(static_cast<float>(dt));
    }
}
