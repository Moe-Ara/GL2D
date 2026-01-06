//
// Created by Mohamad on 04/02/2025.
//

#ifndef GL2D_GL2DEXCEPTION_HPP
#define GL2D_GL2DEXCEPTION_HPP
#include <exception>
#include <string>
#include <utility>

namespace Engine {

    class GL2DException : public std::exception {
    public:
        explicit GL2DException(std::string message) : m_message(std::move(message)) {}
        const char* what() const noexcept override {
            return m_message.c_str();
        }
        virtual ~GL2DException() noexcept override = default;

    private:
        std::string m_message{};
    };

} // Exceptions

#endif //GL2D_GL2DEXCEPTION_HPP
