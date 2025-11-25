//
// Created by Mohamad on 25/11/2025.
//

#include "ParticleRenderer.hpp"
#include <GL/glew.h>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <algorithm>
#include "Graphics/Shader.hpp"

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
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res, res, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
} // namespace

Rendering::ParticleRenderer::ParticleRenderer()
: m_shader(std::make_shared<Graphics::Shader>("Shaders/particle.vert", "Shaders/particle.frag")) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                          (void *) offsetof(ParticleVertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                          (void *) offsetof(ParticleVertex, color));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                          (void *) offsetof(ParticleVertex, uv));
    glBindVertexArray(0);

    m_defaultTexture = createDefaultTexture();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Rendering::ParticleRenderer::~ParticleRenderer() {
    if (m_defaultTexture) {
        glDeleteTextures(1, &m_defaultTexture);
        m_defaultTexture = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void Rendering::ParticleRenderer::begin(const glm::mat4 &viewProjection) {
    m_viewProj = viewProjection;
    m_batch.clear();
}

void Rendering::ParticleRenderer::submit(const Rendering::ParticleRenderData &p) {
    m_batch.push_back(p);
}

void Rendering::ParticleRenderer::end() {
    flush();
}

void Rendering::ParticleRenderer::setBorder(const glm::vec4 &color, float thickness) {
    m_borderColor = color;
    m_borderThickness = std::max(0.0f, thickness);
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
    glBindTexture(GL_TEXTURE_2D, m_defaultTexture);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_batch.clear();
}
