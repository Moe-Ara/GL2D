//
// Created by Mohamad on 04/02/2025.
//

#include "Window.hpp"

#include <GL/glew.h>

#include <cstddef>

#include "Exceptions/WindowException.hpp"

namespace Graphics {
namespace {
std::size_t windowCount = 0;

bool acquireGlfwRuntime() {
    if (windowCount == 0 && glfwInit() != GLFW_TRUE) {
        return false;
    }
    ++windowCount;
    return true;
}

void releaseGlfwRuntime() noexcept {
    if (windowCount == 0) return;
    --windowCount;
    if (windowCount == 0) glfwTerminate();
}
}

    Window::Window(int width, int height, const std::string& title) {
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
        if (width <= 0 || height <= 0) {
            throw WindowException("Window dimensions must be positive (received " +
                                  std::to_string(width) + "x" + std::to_string(height) + ")");
        }
        if (!acquireGlfwRuntime()) {
            throw WindowException("GLFW initialization failed while creating '" + title + "'");
        }
        m_runtimeAcquired = true;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!m_window) {
            releaseGlfwRuntime();
            m_runtimeAcquired = false;
            throw WindowException("Failed to create OpenGL 4.3 core window '" + title +
                                  "' at " + std::to_string(width) + "x" +
                                  std::to_string(height));
        }
        glfwMakeContextCurrent(m_window);

        glewExperimental = GL_TRUE;
        const GLenum glewStatus = glewInit();
        bool glewUsable = glewStatus == GLEW_OK;
#ifdef GLEW_ERROR_NO_GLX_DISPLAY
        glewUsable = glewUsable ||
                     (glewStatus == GLEW_ERROR_NO_GLX_DISPLAY && glGetString(GL_VERSION));
#endif
        if (!glewUsable) {
            const char* detail = reinterpret_cast<const char*>(glewGetErrorString(glewStatus));
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            releaseGlfwRuntime();
            m_runtimeAcquired = false;
            throw WindowException("Failed to initialize OpenGL entry points for '" + title +
                                  "': " + (detail ? detail : "unknown GLEW error") +
                                  " (code " + std::to_string(glewStatus) + ")");
        }
        // Some GLEW/core-profile combinations report a harmless error while
        // probing extensions. A newly-created context has no caller GL state.
        while (glGetError() != GL_NO_ERROR) {}
    }

    void Window::cleanUp() {
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        if (m_runtimeAcquired) {
            releaseGlfwRuntime();
            m_runtimeAcquired = false;
        }
    }

    void Window::framebuffer_size_callback(GLFWwindow*, int width, int height) {
        glViewport(0, 0, width, height);
    }
} // Graphics
