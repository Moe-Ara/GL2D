//
// Created by Codex on 22/11/2025.
//

#include "TilemapComponent.hpp"

TilemapComponent::TilemapComponent(std::shared_ptr<TilemapData> data, int zIndex, bool collision)
    : m_data(std::move(data)), m_zIndex(zIndex), m_collision(collision) {}

void TilemapComponent::render(Entity &/*owner*/) {
    // TODO: Batch draw tiles using the owner's transform and tile atlas.
    // For now this is a stub to keep component wiring intact.
}
