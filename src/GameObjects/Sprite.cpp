//
// Created by Mohamad on 05/02/2025.
//

#include <iostream>
#include "Sprite.hpp"

namespace GameObjects {

    Sprite::Sprite(glm::vec2 position, glm::vec2 size, glm::vec3 color)
            : m_position(position), m_size(size), m_color(color), m_texture(nullptr), m_uvCoords(0, 0, 1, 1) {
        setupMesh();
    }

    Sprite::Sprite(std::shared_ptr<Texture> texture, glm::vec2 position, glm::vec2 size)
            : m_texture(std::move(texture)), m_position(position), m_size(size), m_color(glm::vec3(1.0f)),
              m_uvCoords(0, 0, 1, 1) {
        setupMesh();
    }

    Sprite::Sprite(std::shared_ptr<Texture> texture, glm::vec2 position, glm::vec2 size, int row, int column,
                   int totalRows, int totalCols)
            : m_texture(std::move(texture)), m_position(position), m_size(size), m_color(glm::vec3(1.0f)) {

        float frameWidth = 1.0f / totalCols;
        float frameHeight = 1.0f / totalRows;

        m_uvCoords = glm::vec4(
                column * frameWidth,
                row * frameHeight,
                (column + 1) * frameWidth,
                (row + 1) * frameHeight
        );

        setupMesh();
    }

    Sprite::~Sprite() {
        m_mesh.reset();
    }

    void Sprite::setUVCoords(const glm::vec4 &newUV) {
        if (m_uvCoords != newUV) {
            m_uvCoords = newUV;
            m_dirty = true;
            setupMesh();
        }
    }

    void Sprite::draw() const {
        if (m_mesh) {
            m_mesh->render();
        }
    }

    void Sprite::setupMesh() {
        if (!m_dirty) return;
        float leftU = m_uvCoords.x;
        float rightU = m_uvCoords.z;
        std::vector<Vertex> vertices = {
                {{0.0f,     m_size.y}, m_color, {leftU,  m_uvCoords.w}},  // Top-left
                {{m_size.x, m_size.y}, m_color, {rightU, m_uvCoords.w}},  // Top-right
                {{m_size.x, 0.0f},     m_color, {rightU, m_uvCoords.y}},  // Bottom-right
                {{0.0f,     0.0f},     m_color, {leftU,  m_uvCoords.y}}   // Bottom-left
        };
        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
        if (m_texture) {
            m_mesh = std::make_shared<Mesh>(vertices, indices, m_texture);
        } else {
            m_mesh = std::make_shared<Mesh>(vertices, indices, nullptr);
        }
        m_dirty = false;
    }

    void Sprite::setPosition(const glm::vec2 &newPos) {
        m_position = newPos;
    }

    void Sprite::setSize(const glm::vec2 &newSize) {
        if (m_size != newSize) {
            m_size = newSize;
            m_dirty = true;
        }
    }

    const glm::vec3 &Sprite::getColor() const {
        return m_color;
    }

    const glm::vec4 &Sprite::getUVCoords() const {
        return m_uvCoords;
    }

    const glm::vec2 &Sprite::getSize() const {
        return m_size;
    }

    const glm::vec2 &Sprite::getPosition() const {
        return m_position;
    }

    bool Sprite::hasTexture() const {
        return m_texture != nullptr;
    }

} // namespace GameObjects
