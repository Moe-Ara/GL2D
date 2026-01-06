#pragma once
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/ext/matrix_transform.hpp>

struct Transform {
  glm::vec2 Position{0.0f, 0.0f};
  glm::vec2 Scale{1.0f, 1.0f};
  float Rotation = 0.0f;

  mutable glm::mat4 modelMatrix{1.0f};
  mutable bool dirty = true;

  void setPos(const glm::vec2 &pos) {
    if (Position != pos) {
      Position = pos;
      dirty = true;
    }
  }
  void setScale(const glm::vec2 &scale) {
    if (Scale != scale) {
      Scale = scale;
      dirty = true;
    }
  }
  void setRotation(float r) {
    if (Rotation != r) {
      Rotation = r;
      dirty = true;
    }
  }

  const glm::mat4 &getModelMatrix() const {
    if (dirty) {
      glm::mat4 m(1.0f);
      m = glm::scale(m, glm::vec3(Scale, 1.0f));
      m = glm::rotate(m, glm::radians(Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      m = glm::translate(m, glm::vec3(Position, 0.0f));
      modelMatrix = m;
      dirty = false;
    }
    return modelMatrix;
  }
};
