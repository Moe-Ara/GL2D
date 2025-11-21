//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_ANIMATIONSTATEMACHINECOMPONENT_HPP
#define GL2D_ANIMATIONSTATEMACHINECOMPONENT_HPP


#include "GameObjects/IComponent.hpp"
#include "Graphics/Animation/AnimationStateMachine.hpp"

class AnimationStateMachineComponent: public IUpdatableComponent{
public:
    explicit AnimationStateMachineComponent(Graphics::AnimationStateMachine* stateMachine = nullptr);
    ~AnimationStateMachineComponent() override = default;

    AnimationStateMachineComponent(const AnimationStateMachineComponent &other) = delete;
    AnimationStateMachineComponent &operator=(const AnimationStateMachineComponent &other) = delete;
    AnimationStateMachineComponent(AnimationStateMachineComponent &&other) = delete;
    AnimationStateMachineComponent &operator=(AnimationStateMachineComponent &&other) = delete;

    void update(Entity& owner, double dt) override;

private:
    Graphics::AnimationStateMachine* m_stateMachine{nullptr};
};


#endif //GL2D_ANIMATIONSTATEMACHINECOMPONENT_HPP
