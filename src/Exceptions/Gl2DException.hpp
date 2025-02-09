//
// Created by Mohamad on 04/02/2025.
//

#ifndef GL2D_GL2DEXCEPTION_HPP
#define GL2D_GL2DEXCEPTION_HPP
#include <exception>
#include <string>
namespace Engine {

    class GL2DException :public std::exception{
    public:
        virtual const char* what() const noexcept = 0;
        virtual ~GL2DException() noexcept = default;
    };

} // Exceptions

#endif //GL2D_GL2DEXCEPTION_HPP
