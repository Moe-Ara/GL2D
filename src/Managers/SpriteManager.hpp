//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_SPRITEMANAGER_HPP
#define GL2D_SPRITEMANAGER_HPP


#include <unordered_map>
#include <string>
#include "GameObjects/Sprite.hpp"

class SpriteManager {
public:
    SpriteManager()=delete;

    virtual ~SpriteManager()=delete;

    SpriteManager(const SpriteManager &other) = delete;

    SpriteManager &operator=(const SpriteManager &other) = delete;

    SpriteManager(SpriteManager &&other) = delete;

    SpriteManager &operator=(SpriteManager &&other) = delete;

    static GameObjects::Sprite* get(const std::string& id);
    static void registerSprite(const std::string& id, GameObjects::Sprite* s);
    static bool contains(const std::string& id);
private:
    static std::unordered_map<std::string, GameObjects::Sprite*> m_sprites;
};


#endif //GL2D_SPRITEMANAGER_HPP
