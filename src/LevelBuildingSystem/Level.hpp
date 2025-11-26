//
// Level.hpp
//

#ifndef GL2D_LEVEL_HPP
#define GL2D_LEVEL_HPP

#include <memory>
#include <vector>
#include <string>
#include "LevelSchema.hpp"
#include "GameObjects/Entity.hpp"

struct Level {
    std::string id;
    std::vector<std::unique_ptr<Entity>> entities;
    CameraSettings camera{};
    std::vector<LevelData::Region> regions{};
    std::vector<LevelLight> lights{};
};

#endif //GL2D_LEVEL_HPP
