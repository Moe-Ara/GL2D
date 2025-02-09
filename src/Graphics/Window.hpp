//
// Created by Mohamad on 04/02/2025.
//

#ifndef GL2D_WINDOW_HPP
#define GL2D_WINDOW_HPP
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
namespace Graphics {

    class Window {
    public:
        Window(int width=800, int height=600, const std::string title="Window");

        virtual ~Window();

        Window(const Window &other) = delete;

        Window &operator=(const Window &other) = delete;

        Window(Window &&other) = delete;

        Window &operator=(Window &&other) = delete;
        void pollEvents() const;
        bool shouldClose() const;
        void swapBuffers() const;
        void setResizeCallback(void (*callback)(GLFWwindow*, int, int));
        GLFWwindow* getNativeHandle() const;
    private:
        void init(int width, int height, const std::string& title);
        void cleanUp();
        GLFWwindow* m_window;
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    };

} // Graphics

#endif //GL2D_WINDOW_HPP
