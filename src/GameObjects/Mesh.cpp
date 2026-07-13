#include "Mesh.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace GameObjects {
namespace {
void validateVertices(const std::vector<Vertex>& vertices) {
    if (vertices.empty()) {
        throw std::invalid_argument("Mesh requires at least one vertex");
    }
    if (vertices.size() > std::numeric_limits<uint32_t>::max() ||
        vertices.size() > static_cast<std::size_t>(
            std::numeric_limits<GLsizeiptr>::max() / static_cast<GLsizeiptr>(sizeof(Vertex)))) {
        throw std::length_error("Mesh vertex data exceeds OpenGL buffer limits");
    }
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const Vertex& vertex = vertices[i];
        if (!std::isfinite(vertex.position.x) || !std::isfinite(vertex.position.y) ||
            !std::isfinite(vertex.color.x) || !std::isfinite(vertex.color.y) ||
            !std::isfinite(vertex.color.z) || !std::isfinite(vertex.color.w) ||
            !std::isfinite(vertex.uv.x) || !std::isfinite(vertex.uv.y)) {
            throw std::invalid_argument("Mesh vertex " + std::to_string(i) +
                                        " contains a non-finite value");
        }
    }
}
}

    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
               std::shared_ptr<Texture> texture)
            : m_texture(std::move(texture))
    {
        validateVertices(vertices);
        if (indices.empty() || indices.size() % 3U != 0U) {
            throw std::invalid_argument(
                "Triangle mesh indices must be non-empty and divisible by three");
        }
        if (indices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) ||
            indices.size() > static_cast<std::size_t>(
                std::numeric_limits<GLsizeiptr>::max() /
                static_cast<GLsizeiptr>(sizeof(uint32_t)))) {
            throw std::length_error("Mesh index data exceeds OpenGL draw limits");
        }
        m_maxIndex = *std::max_element(indices.begin(), indices.end());
        if (static_cast<std::size_t>(m_maxIndex) >= vertices.size()) {
            throw std::out_of_range("Mesh index " + std::to_string(m_maxIndex) +
                                    " exceeds vertex count " +
                                    std::to_string(vertices.size()));
        }
        m_indexCount = static_cast<GLsizei>(indices.size());

        GLint previousVertexArray = 0;
        GLint previousArrayBuffer = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ibo);
        try {
            if (!m_vao || !m_vbo || !m_ibo) {
                throw std::runtime_error("OpenGL failed to allocate mesh buffers");
            }
            glBindVertexArray(m_vao);
            createVertexBuffer(vertices);
            createIndexBuffer(indices);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, position)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, color)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, uv)));
            glBindVertexArray(static_cast<GLuint>(previousVertexArray));
            glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
        } catch (...) {
            glBindVertexArray(static_cast<GLuint>(previousVertexArray));
            glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
            releaseGpuResources();
            throw;
        }
    }

    Mesh::~Mesh() {
        releaseGpuResources();
    }

    void Mesh::releaseGpuResources() noexcept {
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_ibo) glDeleteBuffers(1, &m_ibo);
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        m_vbo = 0;
        m_ibo = 0;
        m_vao = 0;
    }

    void Mesh::bind() const {
        glBindVertexArray(m_vao);
        if (m_texture)
            m_texture->bind();
    }

    void Mesh::unbind() const {
        glBindVertexArray(0);
        if (m_texture)
            m_texture->unbind();
    }

    void Mesh::render() const {
        bind();
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
        unbind();
    }

    void Mesh::createVertexBuffer(const std::vector<Vertex>& vertices) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        m_vertexBufferSize = static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex));
        glBufferData(GL_ARRAY_BUFFER, m_vertexBufferSize, vertices.data(), GL_DYNAMIC_DRAW);
    }

    void Mesh::createIndexBuffer(const std::vector<uint32_t>& indices) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                     indices.data(), GL_STATIC_DRAW);
    }

    void Mesh::updateVertices(const std::vector<Vertex> &vertices) {
        validateVertices(vertices);
        if (static_cast<std::size_t>(m_maxIndex) >= vertices.size()) {
            throw std::out_of_range("Updated mesh vertices do not cover index " +
                                    std::to_string(m_maxIndex));
        }
        GLint previousArrayBuffer = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        auto size = static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex));
        if (size > m_vertexBufferSize) {
            m_vertexBufferSize = size;
            glBufferData(GL_ARRAY_BUFFER, size, vertices.data(), GL_DYNAMIC_DRAW);
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices.data());
        }
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    }

    void Mesh::setTexture(std::shared_ptr<Texture> texture) {
        m_texture = std::move(texture);
    }

} // namespace GameObjects
