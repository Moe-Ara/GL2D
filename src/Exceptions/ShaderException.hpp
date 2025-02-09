//
// Created by Mohamad on 04/02/2025.
//

#ifndef GL2D_SHADEREXCEPTION_HPP
#define GL2D_SHADEREXCEPTION_HPP

#include "Gl2DException.hpp"

namespace Graphics{
    class ShaderException : public Engine::GL2DException {
    public:
        explicit ShaderException(const std::string &msg) : message("ShaderException: " + msg) {}

        const char *what() const noexcept override {
            return message.c_str();
        }

        ~ShaderException() noexcept override = default;

    private:
        std::string message{};
    };
}
#endif //GL2D_SHADEREXCEPTION_HPP
