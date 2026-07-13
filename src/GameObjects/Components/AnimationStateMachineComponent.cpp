//
// Created by Mohamad on 21/11/2025.
//

#include "AnimationStateMachineComponent.hpp"
#include "Graphics/Animation/AnimationStateMachine.hpp"

#include <utility>

AnimationStateMachineComponent::AnimationStateMachineComponent(
        std::shared_ptr<Graphics::AnimationStateMachine> stateMachine)
    : m_stateMachine(std::move(stateMachine)) {}

void AnimationStateMachineComponent::update(Entity &/*owner*/, double dt) {
    if (m_stateMachine) {
        m_stateMachine->update(static_cast<float>(dt));
    }
}
