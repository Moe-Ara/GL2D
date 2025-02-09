//
// Created by Mohamad on 09/02/2025.
//

#ifndef GL2D_ISTATEMACHINE_HPP
#define GL2D_ISTATEMACHINE_HPP

#include <string>
#include <memory>
#include "IState.hpp"

namespace Utils {
    class IStateMachine {
    public:
        virtual ~IStateMachine() = default;

        virtual void addState(const std::shared_ptr<IState> &state) = 0;

        virtual void setInitialState(const std::shared_ptr<IState> &state) = 0;

        virtual void changeState(const std::shared_ptr<IState> &newState) = 0;

        virtual void update(float deltaTime) = 0;
    };
}
#endif //GL2D_ISTATEMACHINE_HPP
