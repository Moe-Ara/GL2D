//
// LevelSchema.hpp
//

#ifndef GL2D_LEVELSCHEMA_HPP
#define GL2D_LEVELSCHEMA_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include "GameObjects/Prefabs/Prefab.hpp"

struct LevelMetadata {
    std::string name;
    std::string author;
    std::string build;
    int schemaVersion{1};
};

struct CameraSettings {
    glm::vec4 bounds{0, 0, 0, 0}; // minX, minY, maxX, maxY
    std::string followMode{"None"};
    glm::vec2 deadZone{0.0f, 0.0f};
    glm::vec2 lookAhead{0.0f, 0.0f};
};

struct LevelLayer {
    std::string id;
    std::string type;
    std::string texture;
    glm::vec2 scroll{0.0f, 0.0f};
    int zIndex{0};
};

struct TileLayer {
    std::string id;
    std::string tilemapId;
    std::string tilesetId;
    bool collision{false};
    int zIndex{0};
    glm::vec2 offset{0.0f, 0.0f};
};

struct LevelInstance {
    std::string prefabId;
    glm::vec2 pos{0.0f, 0.0f};
    glm::vec2 scale{1.0f, 1.0f};
    float rot{0.0f};
    std::unordered_map<std::string, ComponentSpec> overrides;
};

enum class TriggerActivation {
    OnEnter,
    OnExit,
    WhileInside,
    Manual
};

struct TriggerShape {
    std::string type{"AABB"};
    glm::vec2 pos{0.0f, 0.0f};
    glm::vec2 size{0.0f, 0.0f};
    float radius{0.0f};
};

struct LevelTrigger {
    std::string id;
    TriggerShape shape{};
    std::string event;
    std::unordered_map<std::string, float> params{};
    TriggerActivation activation{TriggerActivation::OnEnter};
};

struct LevelLight {
    std::string id;
    std::string type{"Point"};
    glm::vec2 pos{0.0f, 0.0f};
    glm::vec2 dir{0.0f, -1.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float radius{4.0f};
    float intensity{1.0f};
    float falloff{2.0f};
    float emissiveBoost{0.0f};
    float innerCutoff{0.9f}; // cos(theta) preferred; loader accepts degrees if provided
    float outerCutoff{0.7f};
    std::string cookie{};
    float cookieStrength{0.0f};
};

struct LevelData {
    LevelMetadata metadata{};
    CameraSettings camera{};
    std::vector<LevelLayer> layers{};
    std::vector<TileLayer> tileLayers{};
    std::vector<LevelInstance> instances{};
    std::vector<LevelTrigger> triggers{};
    // Optional area metadata (lighting/audio/etc.)
    struct Region {
        std::string id;
        glm::vec4 bounds{0,0,0,0}; // minX,minY,maxX,maxY
        glm::vec3 tint{1.0f,1.0f,1.0f};
        std::string ambientId{};
    };
    std::vector<Region> regions{};
    std::vector<LevelLight> lights{};
};

#endif //GL2D_LEVELSCHEMA_HPP
