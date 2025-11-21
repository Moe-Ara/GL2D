//
// Level.hpp
//

#ifndef GL2D_LEVEL_HPP
#define GL2D_LEVEL_HPP

#include <memory>
#include <vector>
#include <string>
#include "LevelSchema.hpp"

class Entity;

struct Level {
    std::string id;
    std::vector<std::unique_ptr<Entity>> entities;
    CameraSettings camera{};
    std::vector<LevelData::Region> regions{};
};

#endif //GL2D_LEVEL_HPP
