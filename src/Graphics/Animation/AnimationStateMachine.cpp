//
// Created by Mohamad on 09/02/2025.
//

#include "AnimationStateMachine.hpp"
#include "Exceptions/AnimationException.hpp"

namespace Graphics {
AnimationStateMachine::AnimationStateMachine() {}

AnimationStateMachine::~AnimationStateMachine() { clear(); }

void AnimationStateMachine::addState(
    const std::shared_ptr<Utils::IState> &state) {
  m_states.insert(state);
}

void AnimationStateMachine::setInitialState(
    const std::shared_ptr<Utils::IState> &state) {
  changeState(state);
}

void AnimationStateMachine::changeState(
    const std::shared_ptr<Utils::IState> &newState) {
  if (m_currentState) {
    m_currentState->onExit();
  }
  m_currentState = newState;
  if (m_currentState) {
    m_currentState->onEnter();
  }
}

void AnimationStateMachine::update(float deltaTime) {
  if (!m_currentState) {
    throw Graphics::AnimationException(
        "Animation State Machine Error: There is no current state");
  }
  auto animState = std::dynamic_pointer_cast<AnimationState>(m_currentState);
  if (animState) {
    for (const auto &[targetWeak, condition] : animState->getTransitions()) {
      if (condition()) {
        if (auto target = targetWeak.lock()) {
          changeState(target);
          break;
        }
      }
    }
  }
  m_currentState->update(deltaTime);
}

void AnimationStateMachine::clear() {
  m_states.clear();
  m_currentState.reset();
}
} // namespace Graphics
