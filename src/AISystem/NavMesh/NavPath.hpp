#ifndef NAV_PATH_HPP
#define NAV_PATH_HPP

#include <vector>

#include <glm/vec2.hpp>

struct NavPath {
    std::vector<glm::vec2> points;
    [[nodiscard]] bool valid() const { return !points.empty(); }
};

#endif // NAV_PATH_HPP
