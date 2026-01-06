//
// Created by Mohamad on 09/02/2025.
//

#ifndef GL2D_ANIMATIONEXCEPTION_HPP
#define GL2D_ANIMATIONEXCEPTION_HPP

#include "Gl2DException.hpp"

namespace Graphics {
    class AnimationException : public Engine::GL2DException {
    public:
        explicit AnimationException(const std::string &msg)
                : Engine::GL2DException("AnimationException: " + msg) {}
    };
}
#endif //GL2D_ANIMATIONEXCEPTION_HPP
