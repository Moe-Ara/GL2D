#ifndef NAV_POLY_HPP
#define NAV_POLY_HPP
#include <vector>
#include <glm/vec2.hpp>
#include "Physics/Collision/AABB.hpp"

struct NavPortal {
    int neighbor{-1};
    glm::vec2 left{0.0f};
    glm::vec2 right{0.0f};
};

struct NavPoly
{
    int id;
    std::vector<glm::vec2> vertices;
    std::vector<int> neighbors;   // indices into a navpoly array
    std::vector<NavPortal> portals;
    AABB bounds;    
};


#endif //NAV_POLY_HPP
