//
// Created by Codex on 22/11/2025.
//

#ifndef GL2D_LEVELBUILDER_HPP
#define GL2D_LEVELBUILDER_HPP

#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/vec2.hpp>
#include "GameObjects/Prefabs/Prefab.hpp"

class Entity;

struct PlacedEntitySpec {
    std::string prefabId;
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 scale{1.0f, 1.0f};
    float rotation{0.0f};
    // Optional component overrides per placement (by component key).
    std::unordered_map<std::string, ComponentSpec> componentOverrides{};
};

class LevelBuilder {
public:
    LevelBuilder() = delete;
    ~LevelBuilder() = delete;
    LevelBuilder(const LevelBuilder&) = delete;
    LevelBuilder& operator=(const LevelBuilder&) = delete;
    LevelBuilder(LevelBuilder&&) = delete;
    LevelBuilder& operator=(LevelBuilder&&) = delete;

    static std::unique_ptr<Entity> instantiate(const PlacedEntitySpec& placement);

    static std::vector<std::unique_ptr<Entity>> build(const std::vector<PlacedEntitySpec>& placements);
};

#endif //GL2D_LEVELBUILDER_HPP
