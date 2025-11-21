//
// Created by Mohamad on 05/02/2025.
//

#include "Sprite.hpp"
namespace GameObjects {

Sprite::Sprite(glm::vec2 position, glm::vec2 size, glm::vec3 color)
    : m_position(position), m_size(size), m_color(color), m_texture(nullptr),
      m_uvCoords(0, 0, 1, 1) {}

Sprite::Sprite(std::shared_ptr<Texture> texture, glm::vec2 position,
               glm::vec2 size)
    : m_texture(std::move(texture)), m_position(position), m_size(size),
      m_color(glm::vec3(1.0f)), m_uvCoords(0, 0, 1, 1) {}

Sprite::Sprite(std::shared_ptr<Texture> texture, glm::vec2 position,
               glm::vec2 size, int row, int column, int totalRows,
               int totalCols)
    : m_texture(std::move(texture)), m_position(position), m_size(size),
      m_color(glm::vec3(1.0f)) {

  float frameWidth = 1.0f / totalCols;
  float frameHeight = 1.0f / totalRows;

  m_uvCoords = glm::vec4(column * frameWidth, row * frameHeight,
                         (column + 1) * frameWidth, (row + 1) * frameHeight);
}

void Sprite::setUVCoords(const glm::vec4 &newUV) {
  m_uvCoords = newUV;
}

void Sprite::setPosition(const glm::vec2 &newPos) { m_position = newPos; }

void Sprite::setSize(const glm::vec2 &newSize) {
  m_size = newSize;
}

void Sprite::setTexture(const std::shared_ptr<Texture> &newTexture) {
  if (m_texture == newTexture)
    return;
  m_texture = newTexture;
}

const glm::vec3 &Sprite::getColor() const { return m_color; }

const glm::vec4 &Sprite::getUVCoords() const { return m_uvCoords; }

const glm::vec2 &Sprite::getSize() const { return m_size; }

const glm::vec2 &Sprite::getPosition() const { return m_position; }

bool Sprite::hasTexture() const { return m_texture != nullptr; }

} // namespace GameObjects
