//
// Loads particle effect presets from JSON.
//

#include "ParticleEffectLoader.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <limits>
#include <glm/glm.hpp>
#include "Utils/SimpleJson.hpp"
#include "Exceptions/SubsystemExceptions.hpp"

namespace {
float numberOrDefault(const Utils::JsonValue& obj, const std::string& key, float fallback) {
    if (!obj.hasKey(key)) {
        return fallback;
    }
    const auto& value = obj.at(key);
    if (!value.isNumber() || !std::isfinite(value.asNumber()) ||
        std::abs(value.asNumber()) > std::numeric_limits<float>::max()) {
        throw Engine::ParticleException(
            "Particle effect field '" + key + "' must be a finite number");
    }
    return static_cast<float>(value.asNumber());
}

template<typename Integer>
Integer integerOrDefault(const Utils::JsonValue& obj, const std::string& key,
                         Integer fallback, std::uint64_t maximum) {
    if (!obj.hasKey(key)) {
        return fallback;
    }
    const auto& value = obj.at(key);
    if (!value.isNumber()) {
        throw Engine::ParticleException(
            "Particle effect field '" + key + "' must be an integer");
    }
    const double number = value.asNumber();
    if (!std::isfinite(number) || number < 0.0 || std::floor(number) != number ||
        number > static_cast<double>(maximum)) {
        throw Engine::ParticleException(
            "Particle effect field '" + key + "' is outside its valid integer range");
    }
    return static_cast<Integer>(number);
}

glm::vec2 vec2OrDefault(const Utils::JsonValue& obj, const std::string& key, const glm::vec2& fallback) {
    if (!obj.hasKey(key)) return fallback;
    const auto& v = obj.at(key);
    if (!v.isArray() || v.asArray().size() != 2 ||
        !v.asArray()[0].isNumber() || !v.asArray()[1].isNumber()) {
        throw Engine::ParticleException(
            "Particle effect field '" + key + "' must contain two numbers");
    }
    const double x = v.asArray()[0].asNumber();
    const double y = v.asArray()[1].asNumber();
    if (!std::isfinite(x) || !std::isfinite(y) ||
        std::abs(x) > std::numeric_limits<float>::max() ||
        std::abs(y) > std::numeric_limits<float>::max()) {
        throw Engine::ParticleException(
            "Particle effect field '" + key + "' must contain finite numbers");
    }
    return {static_cast<float>(x), static_cast<float>(y)};
}

glm::vec4 vec4OrDefault(const Utils::JsonValue& obj, const std::string& key, const glm::vec4& fallback) {
    if (!obj.hasKey(key)) return fallback;
    const auto& v = obj.at(key);
    if (!v.isArray() || v.asArray().size() != 4) {
        throw Engine::ParticleException(
            "Particle effect field '" + key + "' must contain four numbers");
    }
    glm::vec4 result{};
    for (std::size_t index = 0; index < 4; ++index) {
        if (!v.asArray()[index].isNumber() ||
            !std::isfinite(v.asArray()[index].asNumber()) ||
            std::abs(v.asArray()[index].asNumber()) >
                std::numeric_limits<float>::max()) {
            throw Engine::ParticleException(
                "Particle effect field '" + key + "' must contain finite numbers");
        }
        result[index] = static_cast<float>(v.asArray()[index].asNumber());
    }
    return result;
}

ParticleEffectDefinition parseEffect(const Utils::JsonValue& node) {
    if (!node.isObject()) {
        throw Engine::ParticleException("Particle effect must be an object");
    }
    const auto& obj = node.asObject();
    ParticleEffectDefinition def{};

    auto nameIt = obj.find("name");
    if (nameIt == obj.end() || !nameIt->second.isString()) {
        throw Engine::ParticleException("Particle effect missing string 'name'");
    }
    def.name = nameIt->second.asString();

    def.maxParticles = integerOrDefault<std::size_t>(
        node, "maxParticles", 128, 1'000'000);
    def.config.spawnRate = numberOrDefault(node, "spawnRate", def.config.spawnRate);
    def.config.burstCount = integerOrDefault<unsigned int>(
        node, "burstCount", def.config.burstCount,
        std::numeric_limits<unsigned int>::max());
    def.config.minLifeTime = numberOrDefault(node, "minLifeTime", def.config.minLifeTime);
    def.config.maxLifeTime = numberOrDefault(node, "maxLifeTime", def.config.maxLifeTime);
    def.config.minSpeed = numberOrDefault(node, "minSpeed", def.config.minSpeed);
    def.config.maxSpeed = numberOrDefault(node, "maxSpeed", def.config.maxSpeed);
    def.config.direction = numberOrDefault(node, "direction", def.config.direction);
    def.config.spread = numberOrDefault(node, "spread", def.config.spread);
    def.config.minSize = numberOrDefault(node, "minSize", def.config.minSize);
    def.config.maxSize = numberOrDefault(node, "maxSize", def.config.maxSize);
    def.config.endSizeMultiplier = numberOrDefault(
        node, "endSizeMultiplier", def.config.endSizeMultiplier);
    def.config.minAngularVelocity = numberOrDefault(
        node, "minAngularVelocity", def.config.minAngularVelocity);
    def.config.maxAngularVelocity = numberOrDefault(
        node, "maxAngularVelocity", def.config.maxAngularVelocity);
    def.config.startColor = vec4OrDefault(node, "startColor", def.config.startColor);
    def.config.endColor = vec4OrDefault(node, "endColor", def.config.endColor);
    def.config.gravity = vec2OrDefault(node, "gravity", def.config.gravity);
    def.config.drag = numberOrDefault(node, "drag", def.config.drag);
    def.config.homingStrength = numberOrDefault(node, "homingStrength", def.config.homingStrength);
    def.config.orbitStrength = numberOrDefault(node, "orbitStrength", def.config.orbitStrength);
    def.config.spiralStrength = numberOrDefault(node, "spiralStrength", def.config.spiralStrength);
    def.config.randomSeed = integerOrDefault<std::uint32_t>(
        node, "randomSeed", def.config.randomSeed,
        std::numeric_limits<std::uint32_t>::max());

    validateParticleEmitterConfig(def.maxParticles, def.config);

    return def;
}
} // namespace

std::vector<ParticleEffectDefinition> ParticleEffectLoader::loadFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw Engine::ParticleException("Failed to open particle effects file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
        throw Engine::ParticleException(
            "Failed while reading particle effects file: " + path);
    }
    const auto root = Utils::JsonValue::parse(buffer.str());

    std::vector<ParticleEffectDefinition> effects;
    if (root.isArray()) {
        for (const auto& entry : root.asArray()) {
            effects.push_back(parseEffect(entry));
        }
    } else if (root.isObject() && root.hasKey("effects") && root.at("effects").isArray()) {
        for (const auto& entry : root.at("effects").asArray()) {
            effects.push_back(parseEffect(entry));
        }
    } else {
        throw Engine::ParticleException("Particle effects file must be an array or object with 'effects' array");
    }
    return effects;
}
