//
// Created by Mohamad on 04/02/2025.
//

#ifndef GL2D_WINDOWEXCEPTION_HPP
#define GL2D_WINDOWEXCEPTION_HPP

#include "Gl2DException.hpp"

namespace Graphics {
    class WindowException : public Engine::GL2DException {
    public:
        explicit WindowException(const std::string &msg)
                : Engine::GL2DException("WindowException: " + msg) {}
    };
} // namespace Graphics
#endif //GL2D_WINDOWEXCEPTION_HPP
