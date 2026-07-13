//
// LevelLoader.cpp
//

#include "LevelLoader.hpp"
#include "LevelBuilder.hpp"
#include "Level.hpp"
#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/TriggerComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/TilemapComponent.hpp"
#include "GameObjects/Components/LightingComponent.hpp"
#include "GameObjects/Prefabs/PrefabCatalouge.hpp"
#include "Managers/TilemapManager.hpp"
#include "Managers/TilesetManager.hpp"
#include "Utils/SimpleJson.hpp"
#include "Exceptions/SubsystemExceptions.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace {
bool finite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}

TriggerActivationMode activationMode(TriggerActivation activation) {
    switch (activation) {
        case TriggerActivation::OnEnter:
            return TriggerActivationMode::OnEnter;
        case TriggerActivation::OnExit:
            return TriggerActivationMode::OnExit;
        case TriggerActivation::WhileInside:
            return TriggerActivationMode::WhileInside;
        case TriggerActivation::Manual:
            return TriggerActivationMode::Manual;
    }
    throw Engine::LevelException("Level trigger has an invalid activation mode");
}

std::unique_ptr<ACollider> makeTriggerCollider(const LevelTrigger& trigger) {
    const std::string shape = lower(trigger.shape.type);
    if (shape == "aabb" || shape == "box") {
        if (!finite(trigger.shape.size) || trigger.shape.size.x <= 0.0f ||
            trigger.shape.size.y <= 0.0f) {
            throw Engine::LevelException(
                "Level trigger '" + trigger.id +
                "' requires a positive finite AABB size");
        }
        const glm::vec2 halfExtents = trigger.shape.size * 0.5f;
        return std::make_unique<AABBCollider>(-halfExtents, halfExtents);
    }
    if (shape == "circle") {
        if (!std::isfinite(trigger.shape.radius) ||
            trigger.shape.radius <= 0.0f) {
            throw Engine::LevelException(
                "Level trigger '" + trigger.id +
                "' requires a positive finite circle radius");
        }
        return std::make_unique<CircleCollider>(trigger.shape.radius);
    }
    throw Engine::LevelException(
        "Level trigger '" + trigger.id + "' uses unsupported shape type: " +
        trigger.shape.type);
}
} // namespace

Level LevelLoader::loadFromData(const LevelData &data) {
    if (data.metadata.schemaVersion != 1) {
        throw Engine::LevelException(
            "Level '" + data.metadata.name +
            "' uses unsupported schema version " +
            std::to_string(data.metadata.schemaVersion) + " (expected 1)");
    }
    Level level{};
    level.id = data.metadata.name;
    level.camera = data.camera;
    level.regions = data.regions;
    level.lights = data.lights;

    std::vector<PlacedEntitySpec> placements;
    placements.reserve(data.instances.size());
    for (const auto &instance : data.instances) {
        if (!PrefabCatalouge::contains(instance.prefabId)) {
            throw Engine::LevelException(
                "Level '" + data.metadata.name +
                "' references unknown prefab: " + instance.prefabId);
        }
        PlacedEntitySpec spec{};
        spec.prefabId = instance.prefabId;
        spec.position = instance.pos;
        spec.scale = instance.scale;
        spec.rotation = instance.rot;
        spec.componentOverrides = instance.overrides;
        placements.push_back(spec);
    }

    auto entities = LevelBuilder::build(placements);
    for (auto &entity : entities) {
        level.entities.push_back(std::move(entity));
    }

    // Tile layers as standalone entities (for rendering/collision).
    for (const auto &layer : data.tileLayers) {
        auto tilemapData = TilemapManager::get(layer.tilemapId);
        if (!tilemapData) {
            throw Engine::LevelException(
                "Level '" + data.metadata.name +
                "' references unknown tilemap: " + layer.tilemapId);
        }
        if (tilemapData->tilesetId.empty() && !layer.tilesetId.empty()) {
            tilemapData->tilesetId = layer.tilesetId;
        }
        auto tileEntity = std::make_unique<Entity>();
        auto &transform = tileEntity->addComponent<TransformComponent>();
        transform.setPosition(layer.offset);
        auto tileComp = std::make_unique<TilemapComponent>(tilemapData, layer.zIndex, layer.collision);
        tileEntity->addComponent(std::move(tileComp));
        level.entities.push_back(std::move(tileEntity));
    }

    // Trigger geometry participates in the same exact collision path as all
    // other trigger colliders. TriggerComponent owns only activation metadata.
    std::unordered_set<std::string> triggerIds;
    triggerIds.reserve(data.triggers.size());
    for (const auto &trigger : data.triggers) {
        if (trigger.id.empty()) {
            throw Engine::LevelException("Level trigger id cannot be empty");
        }
        if (!triggerIds.insert(trigger.id).second) {
            throw Engine::LevelException(
                "Level contains duplicate trigger id: " + trigger.id);
        }
        if (trigger.event.empty()) {
            throw Engine::LevelException(
                "Level trigger '" + trigger.id + "' has an empty event id");
        }
        if (!finite(trigger.shape.pos)) {
            throw Engine::LevelException(
                "Level trigger '" + trigger.id +
                "' has a non-finite position");
        }
        for (const auto& [name, value] : trigger.params) {
            if (name.empty() || !std::isfinite(value)) {
                throw Engine::LevelException(
                    "Level trigger '" + trigger.id +
                    "' has an invalid parameter");
            }
        }

        auto trigEntity = std::make_unique<Entity>();
        auto &transform = trigEntity->addComponent<TransformComponent>();
        transform.setPosition(trigger.shape.pos);

        trigEntity->addComponent<TriggerComponent>(
            trigger.event, activationMode(trigger.activation), trigger.params);
        auto& collider = trigEntity->addComponent<ColliderComponent>(
            makeTriggerCollider(trigger));
        collider.setTrigger(true);

        level.entities.push_back(std::move(trigEntity));
    }

    auto toLightType = [](const std::string& typeStr) {
        if (typeStr == "Directional" || typeStr == "DIRECTIONAL" || typeStr == "directional") return LightType::DIRECTIONAL;
        if (typeStr == "Spot" || typeStr == "SPOT" || typeStr == "spot") return LightType::SPOT;
        return LightType::POINT;
    };

    // Lights as standalone entities to be picked up by the lighting pass.
    for (const auto &light : data.lights) {
        auto lightEntity = std::make_unique<Entity>();
        auto &transform = lightEntity->addComponent<TransformComponent>();
        transform.setPosition(light.pos);

        auto lightComp = std::make_unique<LightingComponent>(
                light.id.empty() ? "light" : light.id,
                toLightType(light.type),
                glm::vec2(0.0f),
                light.radius,
                light.color,
                light.intensity,
                light.falloff,
                light.emissiveBoost,
                light.dir,
                light.innerCutoff,
                light.outerCutoff,
                light.cookie,
                light.cookieStrength);
        lightEntity->addComponent(std::move(lightComp));
        level.entities.push_back(std::move(lightEntity));
    }

    return level;
}

Level LevelLoader::loadFromFile(const std::string &path) {
    std::ifstream in(path);
    if (!in) {
        throw Engine::LevelException("LevelLoader: cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const auto jsonText = buffer.str();
    auto root = Utils::JsonValue::parse(jsonText);

    auto getVec2 = [](const Utils::JsonValue &node, glm::vec2 fallback = {0.0f, 0.0f}) {
        if (!node.isArray()) return fallback;
        const auto &arr = node.asArray();
        if (arr.size() < 2) return fallback;
        return glm::vec2(static_cast<float>(arr[0].asNumber()), static_cast<float>(arr[1].asNumber()));
    };
    auto getVec3 = [](const Utils::JsonValue &node, glm::vec3 fallback = {1.0f, 1.0f, 1.0f}) {
        if (!node.isArray()) return fallback;
        const auto &arr = node.asArray();
        if (arr.size() < 3) return fallback;
        return glm::vec3(static_cast<float>(arr[0].asNumber()), static_cast<float>(arr[1].asNumber()),
                         static_cast<float>(arr[2].asNumber()));
    };
    auto getVec4 = [](const Utils::JsonValue &node, glm::vec4 fallback = {0,0,0,0}) {
        if (!node.isArray()) return fallback;
        const auto &arr = node.asArray();
        if (arr.size() < 4) return fallback;
        return glm::vec4(static_cast<float>(arr[0].asNumber()), static_cast<float>(arr[1].asNumber()),
                         static_cast<float>(arr[2].asNumber()), static_cast<float>(arr[3].asNumber()));
    };
    auto getNumber = [](const Utils::JsonValue &node, float fallback = 0.0f) {
        return node.isNumber() ? static_cast<float>(node.asNumber()) : fallback;
    };

    LevelData data{};

    if (root.hasKey("metadata")) {
        const auto &meta = root.at("metadata");
        if (meta.hasKey("name")) data.metadata.name = meta.at("name").asString();
        if (meta.hasKey("author")) data.metadata.author = meta.at("author").asString();
        if (meta.hasKey("build")) data.metadata.build = meta.at("build").asString();
        if (meta.hasKey("schemaVersion")) data.metadata.schemaVersion = static_cast<int>(meta.at("schemaVersion").asNumber());
    }

    if (root.hasKey("camera")) {
        const auto &cam = root.at("camera");
        if (cam.hasKey("bounds")) data.camera.bounds = getVec4(cam.at("bounds"));
        if (cam.hasKey("followMode")) data.camera.followMode = cam.at("followMode").asString();
        if (cam.hasKey("deadZone")) data.camera.deadZone = getVec2(cam.at("deadZone"));
        if (cam.hasKey("lookAhead")) data.camera.lookAhead = getVec2(cam.at("lookAhead"));
    }

    if (root.hasKey("layers")) {
        for (const auto &entry : root.at("layers").asArray()) {
            LevelLayer layer{};
            if (entry.hasKey("id")) layer.id = entry.at("id").asString();
            if (entry.hasKey("type")) layer.type = entry.at("type").asString();
            if (entry.hasKey("texture")) layer.texture = entry.at("texture").asString();
            if (entry.hasKey("scroll")) layer.scroll = getVec2(entry.at("scroll"));
            if (entry.hasKey("zIndex")) layer.zIndex = static_cast<int>(entry.at("zIndex").asNumber());
            data.layers.push_back(layer);
        }
    }

    if (root.hasKey("tileLayers")) {
        for (const auto &entry : root.at("tileLayers").asArray()) {
            TileLayer layer{};
            if (entry.hasKey("id")) layer.id = entry.at("id").asString();
            if (entry.hasKey("tilemapId")) layer.tilemapId = entry.at("tilemapId").asString();
            if (entry.hasKey("tilesetId")) layer.tilesetId = entry.at("tilesetId").asString();
            if (entry.hasKey("collision")) layer.collision = entry.at("collision").asBoolean();
            if (entry.hasKey("zIndex")) layer.zIndex = static_cast<int>(entry.at("zIndex").asNumber());
            if (entry.hasKey("offset")) layer.offset = getVec2(entry.at("offset"));
            data.tileLayers.push_back(layer);
        }
    }

    if (root.hasKey("regions")) {
        for (const auto &entry : root.at("regions").asArray()) {
            LevelData::Region region{};
            if (entry.hasKey("id")) region.id = entry.at("id").asString();
            if (entry.hasKey("bounds")) region.bounds = getVec4(entry.at("bounds"));
            if (entry.hasKey("tint")) region.tint = getVec3(entry.at("tint"), {1.0f,1.0f,1.0f});
            if (entry.hasKey("ambientId")) region.ambientId = entry.at("ambientId").asString();
            data.regions.push_back(region);
        }
    }

    if (root.hasKey("lights")) {
        for (const auto &entry : root.at("lights").asArray()) {
            LevelLight light{};
            if (entry.hasKey("id")) light.id = entry.at("id").asString();
            if (entry.hasKey("type")) light.type = entry.at("type").asString();
            if (entry.hasKey("pos")) light.pos = getVec2(entry.at("pos"));
            if (entry.hasKey("dir")) light.dir = getVec2(entry.at("dir"), {0.0f, -1.0f});
            if (entry.hasKey("color")) light.color = getVec3(entry.at("color"), {1.0f, 1.0f, 1.0f});
            if (entry.hasKey("radius")) light.radius = static_cast<float>(entry.at("radius").asNumber());
            if (entry.hasKey("intensity")) light.intensity = static_cast<float>(entry.at("intensity").asNumber());
            if (entry.hasKey("falloff")) light.falloff = static_cast<float>(entry.at("falloff").asNumber());
            if (entry.hasKey("emissiveBoost")) light.emissiveBoost = static_cast<float>(entry.at("emissiveBoost").asNumber());
            if (entry.hasKey("innerCutoff")) light.innerCutoff = getNumber(entry.at("innerCutoff"), light.innerCutoff);
            if (entry.hasKey("outerCutoff")) light.outerCutoff = getNumber(entry.at("outerCutoff"), light.outerCutoff);
            if (entry.hasKey("cookie")) light.cookie = entry.at("cookie").asString();
            if (entry.hasKey("cookieStrength")) light.cookieStrength = getNumber(entry.at("cookieStrength"), light.cookieStrength);
            data.lights.push_back(light);
        }
    }

    if (root.hasKey("instances")) {
        for (const auto &entry : root.at("instances").asArray()) {
            LevelInstance inst{};
            if (entry.hasKey("prefabId")) inst.prefabId = entry.at("prefabId").asString();
            if (entry.hasKey("pos")) inst.pos = getVec2(entry.at("pos"));
            if (entry.hasKey("scale")) inst.scale = getVec2(entry.at("scale"), {1.0f,1.0f});
            if (entry.hasKey("rot")) inst.rot = static_cast<float>(entry.at("rot").asNumber());

            if (entry.hasKey("overrides")) {
                const auto &ovObj = entry.at("overrides").asObject();
                for (const auto &kv : ovObj) {
                    ComponentSpec spec{};
                    spec.type = kv.second.at("type").asString();
                    if (kv.second.hasKey("numbers")) {
                        for (const auto &numEntry : kv.second.at("numbers").asObject()) {
                            spec.numbers[numEntry.first] = static_cast<float>(numEntry.second.asNumber());
                        }
                    }
                    if (kv.second.hasKey("strings")) {
                        for (const auto &strEntry : kv.second.at("strings").asObject()) {
                            spec.strings[strEntry.first] = strEntry.second.asString();
                        }
                    }
                    inst.overrides[kv.first] = spec;
                }
            }

            data.instances.push_back(inst);
        }
    }

    if (root.hasKey("triggers")) {
        for (const auto &entry : root.at("triggers").asArray()) {
            LevelTrigger trig{};
            if (entry.hasKey("id")) trig.id = entry.at("id").asString();
            if (entry.hasKey("event")) trig.event = entry.at("event").asString();
            if (entry.hasKey("params")) {
                for (const auto &p : entry.at("params").asObject()) {
                    trig.params[p.first] = static_cast<float>(p.second.asNumber());
                }
            }
            if (entry.hasKey("activation")) {
                const auto &act = entry.at("activation").asString();
                if (act == "OnEnter") trig.activation = TriggerActivation::OnEnter;
                else if (act == "OnExit") trig.activation = TriggerActivation::OnExit;
                else if (act == "WhileInside") trig.activation = TriggerActivation::WhileInside;
                else if (act == "Manual") trig.activation = TriggerActivation::Manual;
                else {
                    throw Engine::LevelException(
                        "Level trigger '" + trig.id +
                        "' has unknown activation: " + act);
                }
            }
            if (entry.hasKey("shape")) {
                const auto &shape = entry.at("shape");
                if (shape.hasKey("type")) trig.shape.type = shape.at("type").asString();
                if (shape.hasKey("pos")) trig.shape.pos = getVec2(shape.at("pos"));
                if (shape.hasKey("size")) trig.shape.size = getVec2(shape.at("size"));
                if (shape.hasKey("radius")) trig.shape.radius = static_cast<float>(shape.at("radius").asNumber());
            }
            data.triggers.push_back(trig);
        }
    }

    return loadFromData(data);
}
