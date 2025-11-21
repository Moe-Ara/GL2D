//
// Created by Mohamad on 21/11/2025.
//

#include "SpriteManager.hpp"

std::unordered_map<std::string, GameObjects::Sprite *> SpriteManager::m_sprites{};

GameObjects::Sprite *SpriteManager::get(const std::string &id) {
    auto it = m_sprites.find(id);
    GameObjects::Sprite *sprite = nullptr;
    if (it != m_sprites.end()) {
        sprite = it->second;
    }
    return sprite;
}


bool SpriteManager::contains(const std::string &id) {
    return m_sprites.contains(id);
}

void SpriteManager::registerSprite(const std::string &id, GameObjects::Sprite *s) {
    m_sprites[id] = s;
}
