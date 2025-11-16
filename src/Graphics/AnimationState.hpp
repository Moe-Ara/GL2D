//
// Created by Mohamad on 09/02/2025.
//

#ifndef GL2D_ANIMATIONSTATE_HPP
#define GL2D_ANIMATIONSTATE_HPP

#include <memory>
#include <functional>
#include "Utils/IState.hpp"
#include "Animation.hpp"
#include "Managers/Animator.hpp"

namespace Graphics {

    class AnimationState : public Utils::IState {
    public:

        AnimationState(std::shared_ptr<Animation> animation, std::shared_ptr<Managers::Animator> animator);

        virtual ~AnimationState();

        AnimationState(const AnimationState &other) = delete;

        AnimationState &operator=(const AnimationState &other) = delete;

        AnimationState(AnimationState &&other) = delete;

        AnimationState &operator=(AnimationState &&other) = delete;

        void onEnter() override;

        void onExit() override;

        void update(float deltaTime) override;

        void addTransition(const std::shared_ptr<Utils::IState> &targetState, std::function<bool()> condition);

        const std::vector<std::pair<std::weak_ptr<Utils::IState>, std::function<bool()>>> &getTransitions() const;

        std::shared_ptr<Animation> getAnimation() const { return m_animation; }

    private:
        std::shared_ptr<Animation> m_animation;
        std::shared_ptr<Managers::Animator> m_animator;
        std::vector<std::pair<std::weak_ptr<IState>, std::function<bool()>>> m_transitions;
    };

} // Graphics

#endif //GL2D_ANIMATIONSTATE_HPP
