//
// RenderTarget.cpp
//

#include "RenderTarget.hpp"

#include <GL/glew.h>
#include <stdexcept>
#include <string>

namespace Rendering {

RenderTarget::~RenderTarget() {
    destroy();
}

void RenderTarget::initialize(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("RenderTarget dimensions must be positive");
    }
    allocate(width, height);
}

void RenderTarget::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("RenderTarget dimensions must be positive");
    }
    if (!m_fbo) {
        allocate(width, height);
        return;
    }
    if (width == m_width && height == m_height) {
        return;
    }
    allocate(width, height);
}

void RenderTarget::bind() {
    if (!m_fbo) {
        throw std::logic_error("Cannot bind an uninitialized RenderTarget");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    const GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBuffers);
}

void RenderTarget::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::destroy() {
    if (m_colorTex) {
        glDeleteTextures(1, &m_colorTex);
        m_colorTex = 0;
    }
    if (m_normalTex) {
        glDeleteTextures(1, &m_normalTex);
        m_normalTex = 0;
    }
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    m_width = 0;
    m_height = 0;
}

void RenderTarget::allocate(int width, int height) {
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if (width > maxTextureSize || height > maxTextureSize) {
        throw std::length_error(
            "RenderTarget dimensions exceed GL_MAX_TEXTURE_SIZE (" +
            std::to_string(maxTextureSize) + ")");
    }

    GLint previousFramebuffer = 0;
    GLint previousTexture = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    GLuint framebuffer = 0;
    GLuint colorTexture = 0;
    GLuint normalTexture = 0;
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &colorTexture);
    glGenTextures(1, &normalTexture);
    if (!framebuffer || !colorTexture || !normalTexture) {
        if (colorTexture) glDeleteTextures(1, &colorTexture);
        if (normalTexture) glDeleteTextures(1, &normalTexture);
        if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
        throw std::runtime_error("OpenGL failed to allocate RenderTarget resources");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                 GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           colorTexture, 0);

    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           normalTexture, 0);

    const GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBuffers);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glBindFramebuffer(GL_FRAMEBUFFER,
                          static_cast<GLuint>(previousFramebuffer));
        glDeleteTextures(1, &colorTexture);
        glDeleteTextures(1, &normalTexture);
        glDeleteFramebuffers(1, &framebuffer);
        throw std::runtime_error(
            "Failed to create RenderTarget framebuffer; OpenGL status: " +
            std::to_string(status));
    }

    const GLuint oldFramebuffer = m_fbo;
    const GLuint oldColorTexture = m_colorTex;
    const GLuint oldNormalTexture = m_normalTex;
    m_fbo = framebuffer;
    m_colorTex = colorTexture;
    m_normalTex = normalTexture;
    m_width = width;
    m_height = height;

    const GLuint restoreFramebuffer =
        static_cast<GLuint>(previousFramebuffer) == oldFramebuffer
            ? framebuffer : static_cast<GLuint>(previousFramebuffer);
    GLuint restoreTexture = static_cast<GLuint>(previousTexture);
    if (restoreTexture == oldColorTexture) restoreTexture = colorTexture;
    if (restoreTexture == oldNormalTexture) restoreTexture = normalTexture;
    if (oldColorTexture) glDeleteTextures(1, &oldColorTexture);
    if (oldNormalTexture) glDeleteTextures(1, &oldNormalTexture);
    if (oldFramebuffer) glDeleteFramebuffers(1, &oldFramebuffer);
    glBindTexture(GL_TEXTURE_2D, restoreTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, restoreFramebuffer);
}

} // namespace Rendering
