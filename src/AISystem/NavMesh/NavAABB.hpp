#ifndef NAV_AABB_HPP
#define NAV_AABB_HPP

#include <glm/vec2.hpp>

struct NavAABB {
    glm::vec2 min{0.0f};
    glm::vec2 max{0.0f};
    bool contains(const glm::vec2& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }
    bool overlaps(const NavAABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y);
    }
};

#endif // NAV_AABB_HPP
