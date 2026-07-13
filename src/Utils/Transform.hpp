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
    // Public fields are retained for API compatibility, so direct writes cannot
    // reliably set the dirty flag. Comparing against the inputs the cached
    // matrix was built from keeps both access styles safe while skipping the
    // rebuild (translate/rotate/scale with trig) on the hot unchanged path.
    if (dirty || cachedPosition != Position || cachedScale != Scale ||
        cachedRotation != Rotation) {
      glm::mat4 m(1.0f);
      m = glm::translate(m, glm::vec3(Position, 0.0f));
      m = glm::rotate(m, glm::radians(Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      m = glm::scale(m, glm::vec3(Scale, 1.0f));
      modelMatrix = m;
      cachedPosition = Position;
      cachedScale = Scale;
      cachedRotation = Rotation;
      dirty = false;
    }
    return modelMatrix;
  }

private:
  mutable glm::vec2 cachedPosition{0.0f, 0.0f};
  mutable glm::vec2 cachedScale{0.0f, 0.0f}; // deliberately != default Scale
  mutable float cachedRotation = 0.0f;
};
