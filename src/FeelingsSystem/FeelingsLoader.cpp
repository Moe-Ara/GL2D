//
// Data-driven loader for feelings definitions.
//

#include "FeelingsLoader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <optional>

#include <glm/glm.hpp>

#include "Utils/SimpleJson.hpp"

namespace FeelingsSystem {
namespace {
using Utils::JsonValue;

std::optional<float> numberOpt(const JsonValue& obj, const std::string& key) {
    if (!obj.isObject() || !obj.hasKey(key) || !obj.at(key).isNumber()) return std::nullopt;
    return static_cast<float>(obj.at(key).asNumber());
}

std::optional<std::string> stringOpt(const JsonValue& obj, const std::string& key) {
    if (!obj.isObject() || !obj.hasKey(key) || !obj.at(key).isString()) return std::nullopt;
    return obj.at(key).asString();
}

template <size_t N>
std::optional<glm::vec<N, float, glm::defaultp>> vecOpt(const JsonValue& obj, const std::string& key) {
    if (!obj.isObject() || !obj.hasKey(key)) return std::nullopt;
    const auto& v = obj.at(key);
    if (!v.isArray() || v.asArray().size() != N) return std::nullopt;
    glm::vec<N, float, glm::defaultp> out{};
    for (size_t i = 0; i < N; ++i) {
        out[i] = static_cast<float>(v.asArray()[i].asNumber());
    }
    return out;
}

FeelingSnapshot parseFeeling(const JsonValue& node) {
    if (!node.isObject()) {
        throw std::runtime_error("Feeling entry must be an object");
    }
    const auto& obj = node.asObject();
    auto idIt = obj.find("id");
    if (idIt == obj.end() || !idIt->second.isString()) {
        throw std::runtime_error("Feeling entry missing string 'id'");
    }

    FeelingSnapshot f{};
    f.id = idIt->second.asString();
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

    // Particles / VFX
    f.particlePresetId = stringOpt(node, "particlePresetId");
    f.ambientLight = vecOpt<4>(node, "ambientLight");
    f.fogColor = vecOpt<4>(node, "fogColor");
    f.fogDensity = numberOpt(node, "fogDensity");

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
        throw std::runtime_error("Feelings file must be an array or object with 'feelings' array");
    }
    return feelings;
}
} // namespace

std::vector<FeelingSnapshot> FeelingsLoader::loadList(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open feelings file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    const auto root = JsonValue::parse(buffer.str());
    return parseRoot(root);
}

std::unordered_map<std::string, FeelingSnapshot> FeelingsLoader::loadMap(const std::string& path) {
    std::unordered_map<std::string, FeelingSnapshot> out;
    for (auto& f : loadList(path)) {
        out[f.id] = std::move(f);
    }
    return out;
}

} // namespace FeelingsSystem
