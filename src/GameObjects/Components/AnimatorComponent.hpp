//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_ANIMATORCOMPONENT_HPP
#define GL2D_ANIMATORCOMPONENT_HPP


#include "GameObjects/IComponent.hpp"
#include "Graphics/Animation/Animator.hpp"

class AnimatorComponent : public IUpdatableComponent{
public:
    explicit AnimatorComponent(Graphics::Animator* animator = nullptr);
    ~AnimatorComponent() override = default;

    AnimatorComponent(const AnimatorComponent &other) = delete;
    AnimatorComponent &operator=(const AnimatorComponent &other) = delete;
    AnimatorComponent(AnimatorComponent &&other) = delete;
    AnimatorComponent &operator=(AnimatorComponent &&other) = delete;

    void update(Entity& owner, double dt) override;

private:
    Graphics::Animator* m_animator{nullptr};
};


#endif //GL2D_ANIMATORCOMPONENT_HPP
