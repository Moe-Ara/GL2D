//
// Created by Mohamad on 05/02/2025.
//

#ifndef GL2D_TEXTUREEXCEPTION_HPP
#define GL2D_TEXTUREEXCEPTION_HPP

#include "Gl2DException.hpp"

namespace GameObjects{
    class TextureException : public Engine::GL2DException {
    public:
        explicit TextureException(const std::string &msg)
                : Engine::GL2DException("TextureException: " + msg) {}
    };
}
#endif //GL2D_TEXTUREEXCEPTION_HPP
