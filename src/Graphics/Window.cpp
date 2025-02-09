//
// Created by Mohamad on 04/02/2025.
//

#include "Window.hpp"
#include "Exceptions/WindowException.hpp"

namespace Graphics {
    Window::Window(int width, int height, const std::string title) {
        init(width, height, title);
        setResizeCallback(framebuffer_size_callback);
    }

    Window::~Window() {
        cleanUp();
    }

    void Window::pollEvents() const {
        glfwPollEvents();
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void Window::swapBuffers() const {
        glfwSwapBuffers(m_window);
    }

    void Window::setResizeCallback(void (*callback)(GLFWwindow *, int, int)) {
        glfwSetFramebufferSizeCallback(m_window, callback);
    }

    GLFWwindow *Window::getNativeHandle() const {
        return m_window;
    }

    void Window::init(int width, int height, const std::string &title) {
        if (!glfwInit()) {
            throw WindowException("Couldn't init a GL window");
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw WindowException("Couldn't create window instance");
        }
        glfwMakeContextCurrent(m_window);
    }

    void Window::cleanUp() {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void Window::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
        glViewport(0, 0, width, height);
    }
} // Graphics