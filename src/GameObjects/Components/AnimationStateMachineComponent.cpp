//
// Created by Mohamad on 21/11/2025.
//

#include "AnimationStateMachineComponent.hpp"
#include "Graphics/Animation/AnimationStateMachine.hpp"

AnimationStateMachineComponent::AnimationStateMachineComponent(Graphics::AnimationStateMachine *stateMachine)
    : m_stateMachine(stateMachine) {}

void AnimationStateMachineComponent::update(Entity &/*owner*/, double dt) {
    if (m_stateMachine) {
        m_stateMachine->update(static_cast<float>(dt));
    }
}
