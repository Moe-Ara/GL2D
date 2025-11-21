//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_SPRITECOMPONENT_HPP
#define GL2D_SPRITECOMPONENT_HPP


#include "GameObjects/IComponent.hpp"
#include <memory>

namespace GameObjects {
class Sprite;
}

class SpriteComponent: public IRenderableComponent{
public:
    explicit SpriteComponent(GameObjects::Sprite* sprite = nullptr);
    ~SpriteComponent() override = default;

    SpriteComponent(const SpriteComponent &other) = delete;
    SpriteComponent &operator=(const SpriteComponent &other) = delete;
    SpriteComponent(SpriteComponent &&other) = delete;
    SpriteComponent &operator=(SpriteComponent &&other) = delete;

    void render(Entity& owner) override;

private:
    GameObjects::Sprite* m_sprite{nullptr};
};


#endif //GL2D_SPRITECOMPONENT_HPP
