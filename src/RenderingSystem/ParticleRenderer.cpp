//
// Created by Mohamad on 25/11/2025.
//

#include "ParticleRenderer.hpp"
#include <GL/glew.h>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "Graphics/Shader.hpp"
#include "GameObjects/Texture.hpp"

namespace {
struct ParticleVertex {
    glm::vec2 position;
    glm::vec4 color;
    glm::vec2 uv;
};

GLuint createDefaultTexture() {
    // Build a soft radial falloff texture procedurally to give particles a high-res glow.
    constexpr int res = 64;
    std::vector<unsigned char> data(static_cast<size_t>(res * res * 4), 0);
    const float invRes = 1.0f / static_cast<float>(res);
    for (int y = 0; y < res; ++y) {
        for (int x = 0; x < res; ++x) {
            const float fx = (static_cast<float>(x) + 0.5f) * invRes;
            const float fy = (static_cast<float>(y) + 0.5f) * invRes;
            const float dx = fx - 0.5f;
            const float dy = fy - 0.5f;
            const float r = std::sqrt(dx * dx + dy * dy) * 2.0f; // map to 0..1 at radius ~0.5
            const float falloff = std::clamp(std::exp(-3.5f * r * r), 0.0f, 1.0f);
            const unsigned char alpha = static_cast<unsigned char>(falloff * 255.0f);
            const size_t idx = static_cast<size_t>((y * res + x) * 4);
            data[idx + 0] = 255;
            data[idx + 1] = 255;
            data[idx + 2] = 255;
            data[idx + 3] = alpha;
        }
    }

    GLuint tex = 0;
    GLint previousActiveTexture = 0;
    GLint previousTexture = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGenTextures(1, &tex);
    if (!tex) {
        throw std::runtime_error(
            "OpenGL failed to allocate the default particle texture");
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, res, res, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    return tex;
}

bool finite(const glm::mat4& matrix) {
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (!std::isfinite(matrix[column][row])) return false;
        }
    }
    return true;
}

bool validViewBounds(const glm::vec4& bounds) {
    return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
           std::isfinite(bounds.z) && std::isfinite(bounds.w) &&
           bounds.x <= bounds.z && bounds.y <= bounds.w;
}
} // namespace

Rendering::ParticleRenderer::ParticleRenderer()
: m_shader(std::make_shared<Graphics::Shader>("Shaders/particle.vert", "Shaders/particle.frag")) {
    try {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);
        if (!m_vao || !m_vbo || !m_ebo) {
            throw std::runtime_error(
                "OpenGL failed to allocate particle renderer buffers");
        }

        GLint previousVertexArray = 0;
        GLint previousArrayBuffer = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                              reinterpret_cast<void*>(offsetof(ParticleVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                              reinterpret_cast<void*>(offsetof(ParticleVertex, color)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                              reinterpret_cast<void*>(offsetof(ParticleVertex, uv)));
        glBindVertexArray(static_cast<GLuint>(previousVertexArray));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));

        m_defaultTexture = createDefaultTexture();
    } catch (...) {
        destroyResources();
        throw;
    }
}

Rendering::ParticleRenderer::~ParticleRenderer() {
    if (m_frameActive) {
        restoreBlendState();
    }
    destroyResources();
}

void Rendering::ParticleRenderer::destroyResources() noexcept {
    if (m_defaultTexture) glDeleteTextures(1, &m_defaultTexture);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_defaultTexture = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_vao = 0;
}

void Rendering::ParticleRenderer::begin(
    const glm::mat4 &viewProjection, std::optional<glm::vec4> viewBounds) {
    if (m_frameActive) {
        throw std::logic_error("ParticleRenderer::begin called before end");
    }
    if (!finite(viewProjection) ||
        (viewBounds && !validViewBounds(*viewBounds))) {
        throw std::invalid_argument(
            "ParticleRenderer view projection and bounds must be finite; bounds must be ordered");
    }
    m_viewProj = viewProjection;
    m_viewBounds = viewBounds;
    m_batch.clear();
    m_blendMode = ParticleBlendMode::Alpha;
    m_currentTexture = m_defaultTexture;
    m_useRadialMask = true;
    m_blendWasEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
    glGetIntegerv(GL_BLEND_SRC_RGB, &m_previousBlendSourceRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &m_previousBlendDestinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &m_previousBlendSourceAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &m_previousBlendDestinationAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &m_previousBlendEquationRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &m_previousBlendEquationAlpha);
    glEnable(GL_BLEND);
    m_frameActive = true;
}

void Rendering::ParticleRenderer::submit(const Rendering::ParticleRenderData &p) {
    if (!m_frameActive) {
        throw std::logic_error("ParticleRenderer::submit requires an active frame");
    }
    if (!std::isfinite(p.position.x) || !std::isfinite(p.position.y) ||
        !std::isfinite(p.size.x) || !std::isfinite(p.size.y) ||
        !std::isfinite(p.rotation) || !std::isfinite(p.color.x) ||
        !std::isfinite(p.color.y) || !std::isfinite(p.color.z) ||
        !std::isfinite(p.color.w) || p.size.x <= 0.0f || p.size.y <= 0.0f ||
        p.color.x < 0.0f || p.color.y < 0.0f || p.color.z < 0.0f ||
        p.color.w < 0.0f || p.color.w > 1.0f) {
        throw std::invalid_argument(
            "ParticleRenderData requires positive finite size, non-negative RGB, and alpha in [0, 1]");
    }
    if (m_viewBounds) {
        const float radius = glm::length(p.size * 0.5f);
        if (p.position.x + radius < m_viewBounds->x ||
            p.position.y + radius < m_viewBounds->y ||
            p.position.x - radius > m_viewBounds->z ||
            p.position.y - radius > m_viewBounds->w) {
            return;
        }
    }
    Rendering::ParticleRenderData tinted = p;
    tinted.color *= m_globalTint;
    if (!std::isfinite(tinted.color.x) || !std::isfinite(tinted.color.y) ||
        !std::isfinite(tinted.color.z) || !std::isfinite(tinted.color.w)) {
        throw std::overflow_error("Particle tint multiplication overflowed");
    }
    constexpr std::size_t maxParticles = std::min(
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max() / 8U),
        static_cast<std::size_t>(std::numeric_limits<GLsizei>::max() / 12));
    if (m_batch.size() >= maxParticles) {
        throw std::length_error("Particle batch exceeds OpenGL index limits");
    }
    m_batch.push_back(tinted);
}

void Rendering::ParticleRenderer::end() {
    if (!m_frameActive) {
        throw std::logic_error("ParticleRenderer::end called without begin");
    }
    try {
        flush();
    } catch (...) {
        restoreBlendState();
        throw;
    }
    restoreBlendState();
}

void Rendering::ParticleRenderer::setBlendMode(ParticleBlendMode mode) {
    if (!m_frameActive) {
        throw std::logic_error(
            "ParticleRenderer::setBlendMode requires an active frame");
    }
    if (mode == m_blendMode) {
        return;
    }
    flush();
    m_blendMode = mode;
}

void Rendering::ParticleRenderer::setTexture(
    const GameObjects::Texture* texture) {
    if (!m_frameActive) {
        throw std::logic_error(
            "ParticleRenderer::setTexture requires an active frame");
    }
    const GLuint textureId = texture ? texture->getID() : m_defaultTexture;
    const bool radialMask = texture == nullptr;
    if (textureId == m_currentTexture && radialMask == m_useRadialMask) {
        return;
    }
    flush();
    m_currentTexture = textureId ? textureId : m_defaultTexture;
    m_useRadialMask = radialMask || textureId == 0;
}

void Rendering::ParticleRenderer::restoreBlendState() noexcept {
    glBlendFuncSeparate(static_cast<GLenum>(m_previousBlendSourceRgb),
                        static_cast<GLenum>(m_previousBlendDestinationRgb),
                        static_cast<GLenum>(m_previousBlendSourceAlpha),
                        static_cast<GLenum>(m_previousBlendDestinationAlpha));
    glBlendEquationSeparate(static_cast<GLenum>(m_previousBlendEquationRgb),
                            static_cast<GLenum>(m_previousBlendEquationAlpha));
    if (!m_blendWasEnabled) {
        glDisable(GL_BLEND);
    }
    m_frameActive = false;
}

void Rendering::ParticleRenderer::setBorder(const glm::vec4 &color, float thickness) {
    if (!std::isfinite(color.x) || !std::isfinite(color.y) ||
        !std::isfinite(color.z) || !std::isfinite(color.w) ||
        !std::isfinite(thickness) || thickness < 0.0f || color.x < 0.0f ||
        color.y < 0.0f || color.z < 0.0f || color.w < 0.0f || color.w > 1.0f) {
        throw std::invalid_argument(
            "Particle border color and thickness must be finite and non-negative");
    }
    if (color == m_borderColor && thickness == m_borderThickness) return;
    if (m_frameActive) flush();
    m_borderColor = color;
    m_borderThickness = thickness;
}

void Rendering::ParticleRenderer::applyFeeling(const FeelingsSystem::FeelingSnapshot &snapshot) {
    // colorGrade is already applied once by the HDR composite. ambientLight is
    // the particle-specific tint carried by the legacy feeling schema.
    const glm::vec4 tint = snapshot.ambientLight.value_or(glm::vec4{1.0f});
    if (!std::isfinite(tint.x) || !std::isfinite(tint.y) ||
        !std::isfinite(tint.z) || !std::isfinite(tint.w) ||
        tint.x < 0.0f || tint.y < 0.0f || tint.z < 0.0f ||
        tint.w < 0.0f || tint.w > 1.0f) {
        throw std::invalid_argument(
            "Particle feeling tint requires non-negative finite RGB and alpha in [0, 1]");
    }
    m_globalTint = tint;
}

void Rendering::ParticleRenderer::flush() {
    if (m_batch.empty()) return;

    const bool useBorder = m_borderThickness > 0.0f && (m_borderColor.a > 0.0f);

    std::vector<ParticleVertex> vertices;
    std::vector<uint32_t> indices;
    const size_t quadCount = m_batch.size() * (useBorder ? 2 : 1);
    vertices.reserve(quadCount * 4);
    indices.reserve(quadCount * 6);

    const std::array<glm::vec2, 4> baseUV{
            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, 0.0f)
    };

    auto emitQuad = [&](const Rendering::ParticleRenderData& p, const glm::vec2& size, const glm::vec4& color) {
        const glm::vec2 halfSize = size * 0.5f;
        const float c = std::cos(p.rotation);
        const float s = std::sin(p.rotation);

        const std::array<glm::vec2, 4> corners{
                glm::vec2(-halfSize.x, -halfSize.y),
                glm::vec2(halfSize.x, -halfSize.y),
                glm::vec2(halfSize.x, halfSize.y),
                glm::vec2(-halfSize.x, halfSize.y)
        };

        const uint32_t baseIndex = static_cast<uint32_t>(vertices.size());
        for (size_t i = 0; i < corners.size(); ++i) {
            const glm::vec2 rotated{
                    corners[i].x * c - corners[i].y * s,
                    corners[i].x * s + corners[i].y * c
            };
            vertices.push_back(
                    ParticleVertex{p.position + rotated, color, baseUV[i]}
            );
        }
        indices.insert(indices.end(),
                       {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
    };

    for (const auto &p: m_batch) {
        if (useBorder) {
            const glm::vec2 borderSize = p.size * (1.0f + m_borderThickness * 2.0f);
            emitQuad(p, borderSize, m_borderColor);
        }
        emitQuad(p, p.size, p.color);
    }

    m_shader->enable();
    m_shader->setUniformMat4("projection", m_viewProj);
    m_shader->setUniformMat4("transform", glm::mat4(1.0f));
    m_shader->setUniformInt1("spriteTexture", 0);
    m_shader->setUniformInt1("uUseRadialMask", m_useRadialMask ? 1 : 0);

    glBlendEquation(GL_FUNC_ADD);
    if (m_blendMode == ParticleBlendMode::Additive) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(ParticleVertex)),
                 vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_DYNAMIC_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_currentTexture);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_batch.clear();
}
