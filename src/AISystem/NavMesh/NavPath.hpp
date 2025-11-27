#ifndef NAV_PATH_HPP
#define NAV_PATH_HPP
#include <vector>
#include <glm/glm.hpp>
struct NavPath
{
    std::vector<glm::vec2> points;
    bool valid() const { return !points.empty(); }
};


#endif //NAV_PATH_HPP