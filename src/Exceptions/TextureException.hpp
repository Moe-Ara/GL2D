//
// Created by Mohamad on 05/02/2025.
//

#ifndef GL2D_TEXTUREEXCEPTION_HPP
#define GL2D_TEXTUREEXCEPTION_HPP

#include "Gl2DException.hpp"

namespace GameObjects{
    class TextureException : public Engine::GL2DException {
    public:
        explicit TextureException(const std::string &msg) : message("TextureException: " + msg) {}

        const char *what() const noexcept override {
            return message.c_str();
        }

        ~TextureException() noexcept override = default;

    private:
        std::string message{};
    };
}
#endif //GL2D_TEXTUREEXCEPTION_HPP
