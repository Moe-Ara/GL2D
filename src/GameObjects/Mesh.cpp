#include "Mesh.hpp"
#include <cstddef>
#include <utility>

namespace GameObjects {

    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
               std::shared_ptr<Texture> texture)
            : m_texture(std::move(texture)), m_vertices(vertices), m_indices(indices),
              m_indexCount(static_cast<GLsizei>(indices.size()))
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ibo);
        glBindVertexArray(m_vao);
        createVertexBuffer(vertices);
        createIndexBuffer(indices);
        glEnableVertexAttribArray(0); // Position
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        glEnableVertexAttribArray(1); // Color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

        glEnableVertexAttribArray(2); // UV
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glBindVertexArray(0);
    }

    Mesh::~Mesh() {
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ibo);
        glDeleteVertexArrays(1, &m_vao);
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);
    }

    void Mesh::updateVertices(const std::vector<Vertex> &vertices) {
        m_vertices = vertices;
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        auto size = static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex));
        if (size > m_vertexBufferSize) {
            m_vertexBufferSize = size;
            glBufferData(GL_ARRAY_BUFFER, size, vertices.data(), GL_DYNAMIC_DRAW);
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices.data());
        }
    }

    void Mesh::setTexture(std::shared_ptr<Texture> texture) {
        m_texture = std::move(texture);
    }

} // namespace GameObjects
