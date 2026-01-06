//
// Created by Codex on 27/11/2025.
//

#ifndef GL2D_ROPEPREFAB_HPP
#define GL2D_ROPEPREFAB_HPP

#include <glm/vec2.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <string>

class Scene;
class Entity;

namespace Prefabs {

struct RopePrefabConfig {
    glm::vec2 anchorPosition{0.0f, 0.0f};
    glm::vec2 direction{0.0f, -1.0f};
    std::string segmentPrefabId{"rope_segment"};
    float segmentLength{24.0f};
    float segmentThickness{4.0f};
    float segmentSpacing{0.0f};
    int segmentCount{10};
    float segmentMass{0.2f};
    float segmentLinearDamping{2.5f};
    float segmentAngularDamping{3.5f};
    bool limitEnabled{true};
    float lowerLimit{-glm::half_pi<float>() * 0.5f};
    float upperLimit{glm::half_pi<float>() * 0.5f};
    float limitStiffness{12.0f};
    float limitDamping{2.0f};
    float maxLimitTorque{20.0f};
    Entity* startAnchor{nullptr};
    Entity* endAnchor{nullptr};
    glm::vec2 startAnchorOffset{0.0f, 0.0f};
    glm::vec2 endAnchorOffset{0.0f, 0.0f};
    bool useAnchorEntity{true};
    float maxDrop{0.0f};
    bool clampDrop{false};
};

struct RopePrefabResult {
    std::vector<Entity*> segments;
};

class RopePrefab {
public:
    static RopePrefabResult instantiate(Scene& scene, const RopePrefabConfig& config);
};

} // namespace Prefabs

#endif // GL2D_ROPEPREFAB_HPP
