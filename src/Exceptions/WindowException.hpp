//
// Created by Mohamad on 04/02/2025.
//

#ifndef GL2D_WINDOWEXCEPTION_HPP
#define GL2D_WINDOWEXCEPTION_HPP

#include "Gl2DException.hpp"

namespace Graphics {
    class WindowException : public Engine::GL2DException {
    public:
        explicit WindowException(const std::string &msg) : message("WindowException: " + msg) {}

        const char *what() const noexcept override {
            return message.c_str();
        }

        ~WindowException() noexcept override = default;

    private:
        std::string message{};
    };
}//namespace Graphics
#endif //GL2D_WINDOWEXCEPTION_HPP
