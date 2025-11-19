//
// Created by Mohamad on 09/02/2025.
//

#include "AnimationState.hpp"

namespace Graphics {

AnimationState::AnimationState(std::shared_ptr<Animation> animation,
                               std::shared_ptr<Animator> animator)
    : m_animation(std::move(animation)), m_animator(std::move(animator)) {}

AnimationState::~AnimationState() {}

void AnimationState::onEnter() { m_animator->play(m_animation); }

void AnimationState::addTransition(
    const std::shared_ptr<Utils::IState> &targetState,
    std::function<bool()> condition) {
  m_transitions.emplace_back(targetState, std::move(condition));
}

const std::vector<
    std::pair<std::weak_ptr<Utils::IState>, std::function<bool()>>> &
AnimationState::getTransitions() const {
  return m_transitions;
}

void AnimationState::onExit() {
  m_animator->stop();
  m_animator->reset();
}

void AnimationState::update(float deltaTime) { m_animator->update(deltaTime); }
} // namespace Graphics
