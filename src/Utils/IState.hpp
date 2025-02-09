//
// Created by Mohamad on 09/02/2025.
//

#ifndef GL2D_ISTATE_HPP
#define GL2D_ISTATE_HPP

namespace Utils {

    class IState {
    public:
        IState()=default;

        virtual ~IState()=default;

        IState(const IState &other) = delete;

        IState &operator=(const IState &other) = delete;

        IState(IState &&other) = delete;

        IState &operator=(IState &&other) = delete;

        virtual void onEnter()=0;
        virtual void onExit()=0;
        virtual void update(float deltaTime)=0;
    };

} // Utils

#endif //GL2D_ISTATE_HPP
