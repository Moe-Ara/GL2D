//
// Created by Mohamad on 21/11/2025.
//

#include "SpriteComponent.hpp"

#include <utility>

SpriteComponent::SpriteComponent(std::shared_ptr<GameObjects::Sprite> sprite,
                                 int zIndex,
                                 int layer)
    : m_sprite(std::move(sprite)), m_zIndex(zIndex), m_layer(layer) {}

void SpriteComponent::setSprite(std::shared_ptr<GameObjects::Sprite> sprite) {
  m_sprite = std::move(sprite);
}
