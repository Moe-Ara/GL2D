#include "Config/GameConfig.hpp"

#include "Utils/SimpleJson.hpp"
#include "Physics/PhysicsUnits.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace Config {
namespace {
constexpr char kPlayerSettingsPath[] = "assets/config/player_settings.json";

bool readNumber(const Utils::JsonValue &node, const std::string &key,
                float &out, bool convertToUnits = false) {
    if (!node.isObject() || !node.hasKey(key)) {
        return false;
    }
    const auto &value = node.at(key);
    if (!value.isNumber()) {
        return false;
    }
    const float raw = static_cast<float>(value.asNumber());
    out = convertToUnits ? PhysicsUnits::toUnits(raw) : raw;
    return true;
}

const Utils::JsonValue *asObject(const Utils::JsonValue &node,
                                 const std::string &key) {
    if (!node.isObject() || !node.hasKey(key)) {
        return nullptr;
    }
    const auto &sub = node.at(key);
    if (!sub.isObject()) {
        return nullptr;
    }
    return &sub;
}

PlayerConfig parseConfig(const Utils::JsonValue &root) {
    PlayerConfig config{};
    const Utils::JsonValue *movementNode = asObject(root, "movement");
    const Utils::JsonValue &movementSource =
        movementNode && movementNode->isObject() ? *movementNode : root;
    readNumber(movementSource, "moveSpeed", config.movement.moveSpeed, true);
    readNumber(movementSource, "acceleration", config.movement.acceleration,
               true);
    readNumber(movementSource, "deceleration", config.movement.deceleration,
               true);
    readNumber(movementSource, "jumpImpulse", config.movement.jumpImpulse,
               true);
    readNumber(movementSource, "gravity", config.movement.gravity, true);
    readNumber(movementSource, "walkSpeedMultiplier",
               config.movement.walkSpeedMultiplier);
    readNumber(movementSource, "walkAxisThreshold",
               config.movement.walkAxisThreshold);
    readNumber(movementSource, "runAxisThreshold",
               config.movement.runAxisThreshold);
    readNumber(movementSource, "climbSpeed", config.movement.climbSpeed, true);
    readNumber(movementSource, "climbAcceleration",
               config.movement.climbAcceleration, true);
    readNumber(movementSource, "ledgeProbeDistance",
               config.movement.ledgeProbeDistance, true);
    readNumber(movementSource, "ledgeHorizontalOffset",
               config.movement.ledgeHorizontalOffset, true);
    readNumber(movementSource, "ledgeVerticalOffset",
               config.movement.ledgeVerticalOffset, true);
    readNumber(movementSource, "hangVerticalOffset",
               config.movement.hangVerticalOffset, true);
    readNumber(movementSource, "hangClimbOffset",
               config.movement.hangClimbOffset, true);

    const Utils::JsonValue *combatNode = asObject(root, "combat");
    if (combatNode) {
        readNumber(*combatNode, "attackRange", config.combat.attackRange);
        readNumber(*combatNode, "attackCooldown", config.combat.attackCooldown);
        readNumber(*combatNode, "attackDamage", config.combat.attackDamage);
    }

    const Utils::JsonValue *healthNode = asObject(root, "health");
    if (healthNode) {
        readNumber(*healthNode, "maxHp", config.health.maxHp);
        readNumber(*healthNode, "armor", config.health.armor);
        readNumber(*healthNode, "regenRate", config.health.regenRate);
    }

    return config;
}

PlayerConfig loadConfig() {
    std::ifstream file(kPlayerSettingsPath);
    if (!file.is_open()) {
        return {};
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    try {
        const auto root = Utils::JsonValue::parse(buffer.str());
        return parseConfig(root);
    } catch (const std::exception &ex) {
        std::cerr << "GameConfig: failed to parse player settings: " << ex.what()
                  << std::endl;
    }
    return {};
}
} // namespace

const PlayerConfig &GameConfig::player() {
    static const PlayerConfig config = loadConfig();
    return config;
}

} // namespace Config
