//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_PREFAB_HPP
#define GL2D_PREFAB_HPP

#include <string>
#include <glm/vec2.hpp>
#include "GameObjects/IComponent.hpp"
#include "GameObjects/Texture.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

struct ComponentSpec {
    std::string type;

    std::unordered_map<std::string, float> numbers;
    std::unordered_map<std::string, std::string> strings;
};

struct Prefab {
    std::string id;
    std::shared_ptr<GameObjects::Texture> texture{};
    std::string atlasRegion{};

    glm::vec2 defaultSize{1.0f, 1.0f};
    bool hasCollider{false};

    std::unordered_map<std::string, ComponentSpec> components;
    std::vector<std::string> allowedOverrides{};
};
#endif //GL2D_PREFAB_HPP
