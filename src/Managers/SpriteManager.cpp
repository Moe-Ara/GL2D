//
// Created by Mohamad on 21/11/2025.
//

#include "SpriteManager.hpp"

#include <stdexcept>

std::unordered_map<std::string, std::weak_ptr<GameObjects::Sprite>>
    SpriteManager::m_sprites{};

std::shared_ptr<GameObjects::Sprite> SpriteManager::get(const std::string &id) {
    auto it = m_sprites.find(id);
    if (it == m_sprites.end()) {
        return nullptr;
    }
    auto sprite = it->second.lock();
    if (!sprite) {
        m_sprites.erase(it);
    }
    return sprite;
}

bool SpriteManager::contains(const std::string &id) {
    return static_cast<bool>(get(id));
}

void SpriteManager::registerSprite(
        const std::string& id,
        const std::shared_ptr<GameObjects::Sprite>& sprite) {
    if (id.empty()) {
        throw std::invalid_argument("SpriteManager asset id cannot be empty");
    }
    if (!sprite) {
        throw std::invalid_argument(
            "SpriteManager cannot register a null sprite for id: " + id);
    }
    if (auto existing = get(id); existing && existing != sprite) {
        throw std::invalid_argument(
            "SpriteManager asset id is already registered: " + id);
    }
    m_sprites[id] = sprite;
}

bool SpriteManager::unregisterSprite(const std::string& id) {
    return m_sprites.erase(id) != 0u;
}

void SpriteManager::clear() noexcept {
    m_sprites.clear();
}
