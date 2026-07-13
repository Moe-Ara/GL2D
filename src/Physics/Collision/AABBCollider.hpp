#ifndef AABBCOLLIDER_HPP
#define AABBCOLLIDER_HPP

#include "ACollider.hpp"

#include <glm/vec2.hpp>

// Exact world-space representation of an AABBCollider after its Transform is
// applied. The collider remains axis-aligned in local space; rotation makes it
// an oriented box in world space. axisX and axisY are unit vectors.
struct OrientedBounds2D {
  glm::vec2 center{0.0f};
  glm::vec2 axisX{1.0f, 0.0f};
  glm::vec2 axisY{0.0f, 1.0f};
  glm::vec2 halfExtents{0.0f};
};

class AABBCollider : public ACollider {
public:
  AABBCollider(glm::vec2 min, glm::vec2 max);
  ~AABBCollider() override = default;

  ColliderType getType() const override;
  AABB getAABB() const override;
  OrientedBounds2D getOrientedBounds() const;

  void setLocalBounds(const glm::vec2 &min, const glm::vec2 &max);
private:
  glm::vec2 m_min{};
  glm::vec2 m_max{};
};
#endif // !AABBCOLLIDER_HPP
