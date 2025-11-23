#ifndef AABB_H
#define AABB_H

#include <glm/vec2.hpp>
class AABB {
public:
  AABB() : m_min(0.0f), m_max(0.0f) {}
  virtual ~AABB() = default;
  AABB(const glm::vec2 &minPoint, const glm::vec2 &maxPoint)
      : m_min(minPoint), m_max(maxPoint) {}
  float width() const;
  float height() const;
  glm::vec2 getMin() const;
  glm::vec2 getMax() const;
  glm::vec2 center() const;
  AABB expanded(float amount) const;
  bool contains(const glm::vec2 &point) const;
  bool overlaps(const AABB &other) const;
  AABB intersection(const AABB &other) const;

private:
  glm::vec2 m_min;
  glm::vec2 m_max;
};
#endif // !AABB
