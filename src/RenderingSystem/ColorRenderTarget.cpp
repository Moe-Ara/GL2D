#include "RenderingSystem/ColorRenderTarget.hpp"

#include <stdexcept>
#include <string>

namespace Rendering {

ColorRenderTarget::~ColorRenderTarget() {
    destroy();
}

void ColorRenderTarget::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("ColorRenderTarget dimensions must be positive");
    }
    if (m_framebuffer != 0 && width == m_width && height == m_height) {
        return;
    }
    allocate(width, height);
}

void ColorRenderTarget::bind() const {
    if (!m_framebuffer) {
        throw std::logic_error("Cannot bind an uninitialized ColorRenderTarget");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    constexpr GLenum attachment = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &attachment);
}

void ColorRenderTarget::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ColorRenderTarget::destroy() noexcept {
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    if (m_framebuffer) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    m_width = 0;
    m_height = 0;
}

void ColorRenderTarget::allocate(int width, int height) {
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (width > maxTextureSize || height > maxTextureSize) {
        throw std::length_error(
            "ColorRenderTarget dimensions exceed GL_MAX_TEXTURE_SIZE (" +
            std::to_string(maxTextureSize) + ")");
    }

    GLint previousFramebuffer = 0;
    GLint previousTexture = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    GLuint framebuffer = 0;
    GLuint texture = 0;
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &texture);
    if (!framebuffer || !texture) {
        if (texture) glDeleteTextures(1, &texture);
        if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
        throw std::runtime_error(
            "OpenGL failed to allocate ColorRenderTarget resources");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                 GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, texture, 0);

    constexpr GLenum attachment = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &attachment);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glBindFramebuffer(GL_FRAMEBUFFER,
                          static_cast<GLuint>(previousFramebuffer));
        glDeleteTextures(1, &texture);
        glDeleteFramebuffers(1, &framebuffer);
        throw std::runtime_error(
            "Failed to create HDR color framebuffer; OpenGL status: " +
            std::to_string(status));
    }

    const GLuint oldFramebuffer = m_framebuffer;
    const GLuint oldTexture = m_texture;
    m_framebuffer = framebuffer;
    m_texture = texture;
    m_width = width;
    m_height = height;
    const GLuint restoreFramebuffer =
        static_cast<GLuint>(previousFramebuffer) == oldFramebuffer
            ? framebuffer : static_cast<GLuint>(previousFramebuffer);
    const GLuint restoreTexture =
        static_cast<GLuint>(previousTexture) == oldTexture
            ? texture : static_cast<GLuint>(previousTexture);
    if (oldTexture) glDeleteTextures(1, &oldTexture);
    if (oldFramebuffer) glDeleteFramebuffers(1, &oldFramebuffer);
    glBindTexture(GL_TEXTURE_2D, restoreTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, restoreFramebuffer);
}

} // namespace Rendering
