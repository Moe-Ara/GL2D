//
// Created by Mohamad on 04/02/2025.
//

#ifndef GL2D_SHADEREXCEPTION_HPP
#define GL2D_SHADEREXCEPTION_HPP

#include "Gl2DException.hpp"

namespace Graphics{
    class ShaderException : public Engine::GL2DException {
    public:
        explicit ShaderException(const std::string &msg)
                : Engine::GL2DException("ShaderException: " + msg) {}
    };
}
#endif //GL2D_SHADEREXCEPTION_HPP
