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
};

#endif //GL2D_LEVELSCHEMA_HPP
