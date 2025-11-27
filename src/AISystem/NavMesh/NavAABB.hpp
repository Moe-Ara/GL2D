#ifndef NAV_AABB_HPP
#define NAV_AABB_HPP
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>

struct NavAABB
{
    glm::vec2 min;
    glm::vec2 max;
    bool contains(const glm::vec2& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }
    bool overlaps(const NavAABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y);
    }

};

#endif //NAV_AABB_HPP