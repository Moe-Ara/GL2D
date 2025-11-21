//
// Created by Mohamad on 21/11/2025.
//

#include "SpriteComponent.hpp"
#include "GameObjects/Sprite.hpp"

SpriteComponent::SpriteComponent(GameObjects::Sprite *sprite)
    : m_sprite(sprite) {}

void SpriteComponent::render(Entity &/*owner*/) {
    if (m_sprite) {
        m_sprite->draw();
    }
}
