//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_ANIMATORCOMPONENT_HPP
#define GL2D_ANIMATORCOMPONENT_HPP


#include "GameObjects/IComponent.hpp"
#include "Graphics/Animation/Animator.hpp"

#include <memory>

namespace GameObjects {
    class Sprite;
}

class AnimatorComponent : public IUpdatableComponent{
public:
    explicit AnimatorComponent(std::shared_ptr<Graphics::Animator> animator = nullptr);
    explicit AnimatorComponent(Graphics::Animator* animator);
    ~AnimatorComponent() override = default;

    AnimatorComponent(const AnimatorComponent &other) = delete;
    AnimatorComponent &operator=(const AnimatorComponent &other) = delete;
    AnimatorComponent(AnimatorComponent &&other) = delete;
    AnimatorComponent &operator=(AnimatorComponent &&other) = delete;

    void setAnimator(std::shared_ptr<Graphics::Animator> animator);
    void setSprite(std::shared_ptr<GameObjects::Sprite> sprite);
    void play(const std::shared_ptr<Graphics::Animation> &animation, float crossfadeDuration = 0.0f);
    Graphics::Animator* getAnimator() const { return m_animator.get(); }

    void update(Entity& owner, double dt) override;

private:
    void ensureAnimator();

    std::shared_ptr<Graphics::Animator> m_animator{nullptr};
    std::shared_ptr<GameObjects::Sprite> m_sprite{nullptr};
};


#endif //GL2D_ANIMATORCOMPONENT_HPP
