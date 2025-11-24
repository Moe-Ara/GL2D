#ifndef AABBCOLLIDER_HPP
#define AABBCOLLIDER_HPP

#include "ACollider.hpp"

#include <glm/vec2.hpp>
class AABBCollider : public ACollider {
public:
  AABBCollider(glm::vec2 min, glm::vec2 max) : m_min(min), m_max(max) {}
  ~AABBCollider() override = default;

  ColliderType getType() const override;
  AABB getAABB() const override;

  void setLocalBounds(const glm::vec2 &min, const glm::vec2 &max);
private:
  glm::vec2 m_min{};
  glm::vec2 m_max{};
};
#endif // !AABBCOLLIDER_HPP
