//
// Created by Mohamad on 19/11/2025.
//

#include "Entity.hpp"
#include "Engine/IController.hpp"
#include "Utils/EntityAttributes.hpp"
Entity::~Entity() = default;

void Entity::setController(IController *controller) {
  if (controller == nullptr) {
    return;
  }
  m_controller = controller;
}
const EntityAttributes &Entity::getAttributes() const { return m_attributes; }
EntityAttributes &Entity::getAttributes() { return m_attributes; }
IController *Entity::getController() const { return m_controller; }

GameObjects::Sprite *Entity::getSprite() const { return m_sprite; }

Graphics::Animator *Entity::getAnimator() const { return m_animator; }

Graphics::AnimationStateMachine *Entity::getAnimationSM() const {
  return m_animSM;
}

void Entity::update(double deltaTime) {
  if (m_controller) {
    m_controller->update(*this, deltaTime);
  }
  if (m_animator) {
    m_animator->update(deltaTime);
  }
}

void Entity::tickComponents(double deltaTime) {
  for (const auto &entry : m_components) {
    if (auto *updatable = dynamic_cast<IUpdatableComponent *>(entry.second.get())) {
      updatable->update(*this, deltaTime);
    }
  }
}

void Entity::render() {
  if (m_sprite)
    m_sprite->draw();
}
