//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_SPRITEMANAGER_HPP
#define GL2D_SPRITEMANAGER_HPP


#include <memory>
#include <string>
#include <unordered_map>
#include "GameObjects/Sprite.hpp"

class SpriteManager {
public:
    SpriteManager()=delete;

    virtual ~SpriteManager()=delete;

    SpriteManager(const SpriteManager &other) = delete;

    SpriteManager &operator=(const SpriteManager &other) = delete;

    SpriteManager(SpriteManager &&other) = delete;

    SpriteManager &operator=(SpriteManager &&other) = delete;

    // Registries are non-owning caches. Callers receive shared ownership when
    // the registered asset is still alive; expired entries are removed lazily.
    static std::shared_ptr<GameObjects::Sprite> get(const std::string& id);
    static void registerSprite(
        const std::string& id, const std::shared_ptr<GameObjects::Sprite>& sprite);
    static bool contains(const std::string& id);
    static bool unregisterSprite(const std::string& id);
    static void clear() noexcept;
private:
    static std::unordered_map<std::string, std::weak_ptr<GameObjects::Sprite>> m_sprites;
};


#endif //GL2D_SPRITEMANAGER_HPP
