//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_SPRITECOMPONENT_HPP
#define GL2D_SPRITECOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "GameObjects/Sprite.hpp"
#include "RenderingSystem/RenderLayers.hpp"
#include <memory>

class SpriteComponent : public IComponent {
public:
    explicit SpriteComponent(GameObjects::Sprite* sprite = nullptr,
                             int zIndex = 0,
                             int layer = static_cast<int>(Rendering::RenderLayer::Gameplay));
    explicit SpriteComponent(std::shared_ptr<GameObjects::Sprite> sprite,
                             int zIndex = 0,
                             int layer = static_cast<int>(Rendering::RenderLayer::Gameplay));
    ~SpriteComponent() override = default;

    SpriteComponent(const SpriteComponent &other) = delete;
    SpriteComponent &operator=(const SpriteComponent &other) = delete;
    SpriteComponent(SpriteComponent &&other) = delete;
    SpriteComponent &operator=(SpriteComponent &&other) = delete;

    GameObjects::Sprite* sprite() const { return m_sprite.get(); }
    std::shared_ptr<GameObjects::Sprite> sharedSprite() const { return m_sprite; }
    int zIndex() const { return m_zIndex; }
    int layer() const { return m_layer; }
    void setZIndex(int z) { m_zIndex = z; }
    void setLayer(int layer) { m_layer = layer; }
    void setSprite(GameObjects::Sprite* sprite);
    void setSprite(std::shared_ptr<GameObjects::Sprite> sprite);

private:

    std::shared_ptr<GameObjects::Sprite> m_sprite{nullptr};
    int m_zIndex{0};
    int m_layer{static_cast<int>(Rendering::RenderLayer::Gameplay)};
};

#endif // GL2D_SPRITECOMPONENT_HPP
