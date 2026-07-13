//
// Data-driven loader for feelings definitions.
//

#include "FeelingsLoader.hpp"

#include <cmath>
#include <fstream>
#include <optional>
#include <sstream>

#include <glm/glm.hpp>

#include "Utils/SimpleJson.hpp"
#include "Exceptions/SubsystemExceptions.hpp"

namespace FeelingsSystem {
namespace {
using Utils::JsonValue;

std::optional<float> numberOpt(const JsonValue& obj, const std::string& key) {
    if (!obj.isObject() || !obj.hasKey(key)) return std::nullopt;
    if (!obj.at(key).isNumber()) {
        throw Engine::FeelingsException("Feeling field '" + key + "' must be a number");
    }
    const float value = static_cast<float>(obj.at(key).asNumber());
    if (!std::isfinite(value)) {
        throw Engine::FeelingsException("Feeling field '" + key + "' must be finite");
    }
    return value;
}

std::optional<std::string> stringOpt(const JsonValue& obj, const std::string& key) {
    if (!obj.isObject() || !obj.hasKey(key)) return std::nullopt;
    if (!obj.at(key).isString()) {
        throw Engine::FeelingsException("Feeling field '" + key + "' must be a string");
    }
    return obj.at(key).asString();
}

template <size_t N>
std::optional<glm::vec<N, float, glm::defaultp>> vecOpt(const JsonValue& obj, const std::string& key) {
    if (!obj.isObject() || !obj.hasKey(key)) return std::nullopt;
    const auto& v = obj.at(key);
    if (!v.isArray() || v.asArray().size() != N) {
        throw Engine::FeelingsException(
            "Feeling field '" + key + "' must contain exactly " +
            std::to_string(N) + " numbers");
    }
    glm::vec<N, float, glm::defaultp> out{};
    for (size_t i = 0; i < N; ++i) {
        if (!v.asArray()[i].isNumber()) {
            throw Engine::FeelingsException(
                "Feeling field '" + key + "' must contain only numbers");
        }
        out[i] = static_cast<float>(v.asArray()[i].asNumber());
        if (!std::isfinite(out[i])) {
            throw Engine::FeelingsException(
                "Feeling field '" + key + "' must contain only finite values");
        }
    }
    return out;
}

FeelingSnapshot parseFeeling(const JsonValue& node) {
    if (!node.isObject()) {
        throw Engine::FeelingsException("Feeling entry must be an object");
    }
    const auto& obj = node.asObject();
    auto idIt = obj.find("id");
    if (idIt == obj.end() || !idIt->second.isString()) {
        throw Engine::FeelingsException("Feeling entry missing string 'id'");
    }

    FeelingSnapshot f{};
    f.id = idIt->second.asString();
    if (f.id.empty()) {
        throw Engine::FeelingsException("Feeling id cannot be empty");
    }
    if (auto v = numberOpt(node, "blendInMs")) f.blendInMs = *v;
    if (auto v = numberOpt(node, "blendOutMs")) f.blendOutMs = *v;

    // Rendering
    f.colorGrade = vecOpt<4>(node, "colorGrade");
    f.vignette = numberOpt(node, "vignette");
    f.bloomStrength = numberOpt(node, "bloomStrength");
    f.paletteID = stringOpt(node, "paletteId");

    // Camera
    f.zoomMul = numberOpt(node, "zoomMul");
    f.offset = vecOpt<2>(node, "offset");
    f.shakeMagnitude = numberOpt(node, "shakeMagnitude");
    f.shakeRoughness = numberOpt(node, "shakeRoughness");
    f.parallaxBias = vecOpt<4>(node, "parallaxBias");
    f.followSpeedMul = numberOpt(node, "followSpeedMul");

    // Gameplay
    f.timeScale = numberOpt(node, "timeScale");
    f.entitySpeedMul = numberOpt(node, "entitySpeedMul");
    f.animationSpeedMul = numberOpt(node, "animationSpeedMul");
    f.accelerationSpeedMul = numberOpt(node, "accelerationSpeedMul");
    f.damageMul = numberOpt(node, "damageMul");
    f.armorMul = numberOpt(node, "armorMul");

    // Particles / VFX
    f.particlePresetId = stringOpt(node, "particlePresetId");
    f.ambientLight = vecOpt<4>(node, "ambientLight");
    f.fogColor = vecOpt<4>(node, "fogColor");
    f.fogDensity = numberOpt(node, "fogDensity");
    f.lightIntensityMul = numberOpt(node, "lightIntensityMul");
    f.lightRadiusMul = numberOpt(node, "lightRadiusMul");
    f.lightColorMul = vecOpt<3>(node, "lightColorMul");
    f.ambientLightMul = vecOpt<3>(node, "ambientLightMul");
    f.ambientLightAdd = vecOpt<3>(node, "ambientLightAdd");

    // Audio
    f.musicTrackId = stringOpt(node, "musicTrackId");
    f.musicVolume = numberOpt(node, "musicVolume");
    f.sfxTag = stringOpt(node, "sfxTag");
    f.sfxVolumeMul = numberOpt(node, "sfxVolumeMul");
    f.audioFxPreset = stringOpt(node, "audioFxPreset");
    f.reverbSend = numberOpt(node, "reverbSend");
    f.delaySend = numberOpt(node, "delaySend");
    f.delayTimeMs = numberOpt(node, "delayTimeMs");
    f.delayFeedback = numberOpt(node, "delayFeedback");
    f.lowpassHz = numberOpt(node, "lowpassHz");
    f.highpassHz = numberOpt(node, "highpassHz");

    // UI
    f.uiTint = vecOpt<4>(node, "uiTint");
    f.uiLerpSpeed = numberOpt(node, "uiLerpSpeed");

    if (const auto error = validationError(f)) {
        throw Engine::FeelingsException("Feeling '" + f.id + "': " + *error);
    }

    return f;
}

std::vector<FeelingSnapshot> parseRoot(const JsonValue& root) {
    std::vector<FeelingSnapshot> feelings;
    if (root.isArray()) {
        for (const auto& entry : root.asArray()) {
            feelings.push_back(parseFeeling(entry));
        }
    } else if (root.isObject() && root.hasKey("feelings") && root.at("feelings").isArray()) {
        for (const auto& entry : root.at("feelings").asArray()) {
            feelings.push_back(parseFeeling(entry));
        }
    } else {
        throw Engine::FeelingsException("Feelings file must be an array or object with 'feelings' array");
    }
    return feelings;
}
} // namespace

std::vector<FeelingSnapshot> FeelingsLoader::loadList(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw Engine::FeelingsException("Failed to open feelings file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
        throw Engine::FeelingsException("Failed while reading feelings file: " + path);
    }
    const auto root = JsonValue::parse(buffer.str());
    return parseRoot(root);
}

std::unordered_map<std::string, FeelingSnapshot> FeelingsLoader::loadMap(const std::string& path) {
    std::unordered_map<std::string, FeelingSnapshot> out;
    for (auto& f : loadList(path)) {
        const std::string id = f.id;
        if (!out.emplace(id, std::move(f)).second) {
            throw Engine::FeelingsException("Duplicate feeling id: " + id);
        }
    }
    return out;
}

} // namespace FeelingsSystem
