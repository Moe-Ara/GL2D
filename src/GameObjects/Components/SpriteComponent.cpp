//
// Created by Mohamad on 21/11/2025.
//

#include "SpriteComponent.hpp"

#include <memory>

namespace {
std::shared_ptr<GameObjects::Sprite> aliasSprite(GameObjects::Sprite *sprite) {
  if (!sprite) {
    return nullptr;
  }
  return std::shared_ptr<GameObjects::Sprite>(sprite, [](GameObjects::Sprite *) {});
}
} // namespace

SpriteComponent::SpriteComponent(GameObjects::Sprite *sprite, int zIndex, int layer)
    : m_sprite(aliasSprite(sprite)), m_zIndex(zIndex), m_layer(layer) {}

SpriteComponent::SpriteComponent(std::shared_ptr<GameObjects::Sprite> sprite,
                                 int zIndex,
                                 int layer)
    : m_sprite(std::move(sprite)), m_zIndex(zIndex), m_layer(layer) {}

void SpriteComponent::setSprite(GameObjects::Sprite *sprite) {
  m_sprite = aliasSprite(sprite);
}

void SpriteComponent::setSprite(std::shared_ptr<GameObjects::Sprite> sprite) {
  m_sprite = std::move(sprite);
}
