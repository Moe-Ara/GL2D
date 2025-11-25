//
// Loads particle effect presets from JSON.
//

#include "ParticleEffectLoader.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glm/glm.hpp>
#include "Utils/SimpleJson.hpp"

namespace {
float numberOrDefault(const Utils::JsonValue& obj, const std::string& key, float fallback) {
    if (obj.hasKey(key) && obj.at(key).isNumber()) {
        return static_cast<float>(obj.at(key).asNumber());
    }
    return fallback;
}

glm::vec2 vec2OrDefault(const Utils::JsonValue& obj, const std::string& key, const glm::vec2& fallback) {
    if (!obj.hasKey(key)) return fallback;
    const auto& v = obj.at(key);
    if (!v.isArray() || v.asArray().size() != 2) return fallback;
    return glm::vec2(
        static_cast<float>(v.asArray()[0].asNumber()),
        static_cast<float>(v.asArray()[1].asNumber())
    );
}

glm::vec4 vec4OrDefault(const Utils::JsonValue& obj, const std::string& key, const glm::vec4& fallback) {
    if (!obj.hasKey(key)) return fallback;
    const auto& v = obj.at(key);
    if (!v.isArray() || v.asArray().size() != 4) return fallback;
    return glm::vec4(
        static_cast<float>(v.asArray()[0].asNumber()),
        static_cast<float>(v.asArray()[1].asNumber()),
        static_cast<float>(v.asArray()[2].asNumber()),
        static_cast<float>(v.asArray()[3].asNumber())
    );
}

ParticleEffectDefinition parseEffect(const Utils::JsonValue& node) {
    if (!node.isObject()) {
        throw std::runtime_error("Particle effect must be an object");
    }
    const auto& obj = node.asObject();
    ParticleEffectDefinition def{};

    auto nameIt = obj.find("name");
    if (nameIt == obj.end() || !nameIt->second.isString()) {
        throw std::runtime_error("Particle effect missing string 'name'");
    }
    def.name = nameIt->second.asString();

    def.maxParticles = static_cast<std::size_t>(numberOrDefault(node, "maxParticles", 128.0f));
    def.config.spawnRate = numberOrDefault(node, "spawnRate", def.config.spawnRate);
    def.config.burstCount = static_cast<unsigned int>(numberOrDefault(node, "burstCount", static_cast<float>(def.config.burstCount)));
    def.config.minLifeTime = numberOrDefault(node, "minLifeTime", def.config.minLifeTime);
    def.config.maxLifeTime = numberOrDefault(node, "maxLifeTime", def.config.maxLifeTime);
    def.config.minSpeed = numberOrDefault(node, "minSpeed", def.config.minSpeed);
    def.config.maxSpeed = numberOrDefault(node, "maxSpeed", def.config.maxSpeed);
    def.config.direction = numberOrDefault(node, "direction", def.config.direction);
    def.config.spread = numberOrDefault(node, "spread", def.config.spread);
    def.config.minSize = numberOrDefault(node, "minSize", def.config.minSize);
    def.config.maxSize = numberOrDefault(node, "maxSize", def.config.maxSize);
    def.config.startColor = vec4OrDefault(node, "startColor", def.config.startColor);
    def.config.endColor = vec4OrDefault(node, "endColor", def.config.endColor);
    def.config.gravity = vec2OrDefault(node, "gravity", def.config.gravity);
    def.config.drag = numberOrDefault(node, "drag", def.config.drag);
    def.config.homingStrength = numberOrDefault(node, "homingStrength", def.config.homingStrength);
    def.config.orbitStrength = numberOrDefault(node, "orbitStrength", def.config.orbitStrength);
    def.config.spiralStrength = numberOrDefault(node, "spiralStrength", def.config.spiralStrength);

    return def;
}
} // namespace

std::vector<ParticleEffectDefinition> ParticleEffectLoader::loadFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open particle effects file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
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
        throw std::runtime_error("Particle effects file must be an array or object with 'effects' array");
    }
    return effects;
}
