//
// Created by Mohamad on 09/02/2025.
//

#ifndef GL2D_ANIMATIONSTATEMACHINE_HPP
#define GL2D_ANIMATIONSTATEMACHINE_HPP

#include <unordered_set>
#include "Utils/IStateMachine.hpp"

namespace Managers {

    class AnimationStateMachine : public Utils::IStateMachine {
    public:
        AnimationStateMachine();

        virtual ~AnimationStateMachine() override;

        void addState(const std::shared_ptr<Utils::IState> &state) override;

        void setInitialState(const std::shared_ptr<Utils::IState> &state) override;

        void changeState(const std::shared_ptr<Utils::IState> &newState) override;

        void update(float deltaTime) override;

        void clear();

    private:
        std::unordered_set<std::shared_ptr<Utils::IState>> m_states;
        std::shared_ptr<Utils::IState> m_currentState;

        AnimationStateMachine(const AnimationStateMachine &other) = delete;

        AnimationStateMachine &operator=(const AnimationStateMachine &other) = delete;

        AnimationStateMachine(AnimationStateMachine &&other) = delete;

        AnimationStateMachine &operator=(AnimationStateMachine &&other) = delete;
    };

} // Managers

#endif //GL2D_ANIMATIONSTATEMACHINE_HPP
