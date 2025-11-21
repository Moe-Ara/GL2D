//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_SPRITECOMPONENT_HPP
#define GL2D_SPRITECOMPONENT_HPP


#include "GameObjects/IComponent.hpp"
#include "GameObjects/Sprite.hpp"

class SpriteComponent: public IComponent{
public:
    explicit SpriteComponent(GameObjects::Sprite* sprite = nullptr, int zIndex = 0);
    ~SpriteComponent() override = default;

    SpriteComponent(const SpriteComponent &other) = delete;
    SpriteComponent &operator=(const SpriteComponent &other) = delete;
    SpriteComponent(SpriteComponent &&other) = delete;
    SpriteComponent &operator=(SpriteComponent &&other) = delete;

    GameObjects::Sprite* sprite() const { return m_sprite; }
    int zIndex() const { return m_zIndex; }
    void setZIndex(int z) { m_zIndex = z; }
    void setSprite(GameObjects::Sprite* sprite) { m_sprite = sprite; }

private:
    GameObjects::Sprite* m_sprite{nullptr};
    int m_zIndex{0};
};


#endif //GL2D_SPRITECOMPONENT_HPP
