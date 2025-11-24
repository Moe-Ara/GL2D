#include "AABB.hpp"

#include <algorithm>
#include <glm/vec2.hpp>
float AABB::width() const { return m_max.x - m_min.x; }
float AABB::height() const { return m_max.y - m_min.y; }
glm::vec2 AABB::center() const { return (m_min + m_max) * 0.5f; }
glm::vec2 AABB::getMin() const { return m_min; }
glm::vec2 AABB::getMax() const { return m_max; }
AABB AABB::expanded(float amount) const {
  return AABB{glm::vec2(m_min.x - amount, m_min.y - amount),
              glm::vec2(m_max.x + amount, m_max.y + amount)};
}

bool AABB::contains(const glm::vec2 &point) const {
  return (point.x >= m_min.x && point.x <= m_max.x && point.y >= m_min.y &&
          point.y <= m_max.y);
}

bool AABB::overlaps(const AABB &other) const {
  return !(m_max.x < other.getMin().x || m_min.x > other.getMax().x ||
           m_max.y < other.getMin().y || m_min.y > other.getMax().y);
}

AABB AABB::intersection(const AABB &other) const {
  if (!overlaps(other)) {
    return AABB{};
  }

  glm::vec2 newMin{std::max(m_min.x, other.getMin().x),
                   std::max(m_min.y, other.getMin().y)};
  glm::vec2 newMax{std::min(m_max.x, other.getMax().x),
                   std::min(m_max.y, other.getMax().y)};

  return AABB{newMin, newMax};
}
