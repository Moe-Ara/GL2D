//
// Created by Mohamad on 21/11/2025.
//

#include "SpriteComponent.hpp"
#include "GameObjects/Sprite.hpp"

SpriteComponent::SpriteComponent(GameObjects::Sprite *sprite, int zIndex)
    : m_sprite(sprite), m_zIndex(zIndex) {}
