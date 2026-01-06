//
// Created by Mohamad on 05/02/2025.
//

#ifndef GL2D_SPRITE_HPP
#define GL2D_SPRITE_HPP

#include "Mesh.hpp"
#include "Texture.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>

namespace GameObjects {

class Sprite {
public:
  Sprite(glm::vec2 position, glm::vec2 size, glm::vec3 color);
  Sprite(std::shared_ptr<Texture> texture, glm::vec2 position, glm::vec2 size);
  Sprite(std::shared_ptr<Texture> texture, glm::vec2 position, glm::vec2 size,
         int row, int column, int totalRows, int totalCols);

  virtual ~Sprite() = default;

  // Delete copy & move constructors
  Sprite(const Sprite &other) = delete;
  Sprite &operator=(const Sprite &other) = delete;
  Sprite(Sprite &&other) = delete;
  Sprite &operator=(Sprite &&other) = delete;

  // Setters
  void setPosition(const glm::vec2 &newPos);
  void setSize(const glm::vec2 &newSize);
  void setUVCoords(const glm::vec4 &newUV);
  void setTexture(const std::shared_ptr<Texture> &newTexture);
  void setNormalTexture(const std::shared_ptr<Texture> &newTexture);
  void setFlipX(bool flip);
  std::shared_ptr<Sprite> clone() const;

  // Getters
  const glm::vec4 &getColor() const;
  void setColor(const glm::vec4 &color);
  const glm::vec4 &getUVCoords() const;
  const glm::vec2 &getSize() const;
  const glm::vec2 &getPosition() const;
  const std::shared_ptr<Texture>& getTexture() const { return m_texture; }
  const std::shared_ptr<Texture>& getNormalTexture() const { return m_normalTexture; }

  bool hasTexture() const;
  bool hasNormalTexture() const { return m_normalTexture != nullptr; }
  bool isFlipX() const;

private:
  std::shared_ptr<Texture> m_texture;
  std::shared_ptr<Texture> m_normalTexture;
  glm::vec2 m_position{};
  glm::vec2 m_size{};
  glm::vec4 m_uvCoords{};
  glm::vec4 m_color{};
  bool m_flipX{false};
};

} // namespace GameObjects

#endif // GL2D_SPRITE_HPP
