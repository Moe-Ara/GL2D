#ifndef CIRCLE_COLLIDER_HPP
#define CIRCLE_COLLIDER_HPP
#include "AABB.hpp"
#include "ACollider.hpp"

#include <glm/vec2.hpp>
class CircleCollider : public ACollider {
public:
  CircleCollider(float radius) : m_radius(radius) {}
  ~CircleCollider() override = default;
  float getRadius() const;
  void setRadius(float radius);
  ColliderType getType() const override;
  AABB getAABB() const override;
  void setLocalOffset(const glm::vec2 &offset) { m_localOffset = offset; }
  const glm::vec2 &getLocalOffset() const { return m_localOffset; }
private:
  float m_radius{};
  glm::vec2 m_localOffset{0.0f};
};
#endif // !CIRCLE_COLLIDER_HPP
