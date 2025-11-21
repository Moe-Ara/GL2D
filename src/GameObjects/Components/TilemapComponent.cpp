//
// Created by Codex on 22/11/2025.
//

#include "TilemapComponent.hpp"

TilemapComponent::TilemapComponent(std::shared_ptr<TilemapData> data, int zIndex, bool collision)
    : m_data(std::move(data)), m_zIndex(zIndex), m_collision(collision) {}
