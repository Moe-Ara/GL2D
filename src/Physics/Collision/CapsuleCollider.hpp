#ifndef CAPSULE_COLLIDER_HPP
#define CAPSULE_COLLIDER_HPP

#include "AABB.hpp"
#include "ACollider.hpp"
#include "Utils/Transform.hpp"

#include <glm/vec2.hpp>
#include <memory>

class CapsuleCollider : public ACollider {
public:
  CapsuleCollider(const glm::vec2 &localA, const glm::vec2 &localB, float radius,
                  const glm::vec2 &localOffset = glm::vec2{0.0f});
  ~CapsuleCollider() override = default;
  float getRadius() const;
  void setRadius(float radius);
  ColliderType getType() const override;
  AABB getAABB() const override;
  const glm::vec2 &getLocalA() const;
  const glm::vec2 &getLocalB() const;
  void setLocalA(const glm::vec2 &pointA);
  void setLocalB(const glm::vec2 &pointB);
  const glm::vec2 &getLocalOffset() const;
  void setLocalOffset(const glm::vec2 &localOffset);
  std::unique_ptr<Hit> hit(const ICollider &other) const override;
  glm::vec2 getWorldA() const;
  glm::vec2 getWorldB() const;

private:
  glm::vec2 m_localA{};
  glm::vec2 m_localB{};
  float m_radius{};
  glm::vec2 m_localOffset{0.0f};
};
#endif // !CAPSULE_COLLIDER_HPP
