#include "FeelingSnapshot.hpp"

#include <cmath>
#include <string>

namespace FeelingsSystem {
namespace {

template <typename T>
bool finite(const T& value) {
    for (glm::length_t index = 0; index < value.length(); ++index) {
        if (!std::isfinite(value[index])) return false;
    }
    return true;
}

bool finite(float value) {
    return std::isfinite(value);
}

template <typename T>
std::optional<std::string> requireFinite(
    const std::optional<T>& value, const char* name) {
    if (value && !finite(*value)) {
        return std::string{name} + " must be finite";
    }
    return std::nullopt;
}

std::optional<std::string> requireNonNegative(
    const std::optional<float>& value, const char* name) {
    if (auto error = requireFinite(value, name)) return error;
    if (value && *value < 0.0f) {
        return std::string{name} + " cannot be negative";
    }
    return std::nullopt;
}

std::optional<std::string> requireUnitRange(
    const std::optional<float>& value, const char* name) {
    if (auto error = requireFinite(value, name)) return error;
    if (value && (*value < 0.0f || *value > 1.0f)) {
        return std::string{name} + " must be in [0, 1]";
    }
    return std::nullopt;
}

template <glm::length_t N>
std::optional<std::string> requireNonNegativeColor(
    const std::optional<glm::vec<N, float, glm::defaultp>>& value,
    const char* name, bool validateAlpha = false) {
    if (auto error = requireFinite(value, name)) return error;
    if (!value) return std::nullopt;
    for (glm::length_t index = 0; index < N; ++index) {
        if ((*value)[index] < 0.0f) {
            return std::string{name} + " cannot contain negative channels";
        }
    }
    if (validateAlpha && N == 4 && (*value)[3] > 1.0f) {
        return std::string{name} + " alpha must be in [0, 1]";
    }
    return std::nullopt;
}

} // namespace

std::optional<std::string> validationError(const FeelingSnapshot& snapshot) {
    if (!std::isfinite(snapshot.blendInMs) || snapshot.blendInMs < 0.0f) {
        return "blendInMs must be finite and non-negative";
    }
    if (!std::isfinite(snapshot.blendOutMs) || snapshot.blendOutMs < 0.0f) {
        return "blendOutMs must be finite and non-negative";
    }

#define GL2D_VALIDATE(expression) \
    do { if (auto error = (expression)) return error; } while (false)

    GL2D_VALIDATE(requireNonNegativeColor(snapshot.colorGrade, "colorGrade", true));
    GL2D_VALIDATE(requireUnitRange(snapshot.vignette, "vignette"));
    GL2D_VALIDATE(requireNonNegative(snapshot.bloomStrength, "bloomStrength"));

    GL2D_VALIDATE(requireFinite(snapshot.zoomMul, "zoomMul"));
    if (snapshot.zoomMul && *snapshot.zoomMul <= 0.0f) {
        return "zoomMul must be greater than zero";
    }
    GL2D_VALIDATE(requireFinite(snapshot.offset, "offset"));
    GL2D_VALIDATE(requireNonNegative(snapshot.shakeMagnitude, "shakeMagnitude"));
    GL2D_VALIDATE(requireNonNegative(snapshot.shakeRoughness, "shakeRoughness"));
    GL2D_VALIDATE(requireFinite(snapshot.parallaxBias, "parallaxBias"));
    GL2D_VALIDATE(requireNonNegative(snapshot.followSpeedMul, "followSpeedMul"));

    GL2D_VALIDATE(requireNonNegative(snapshot.timeScale, "timeScale"));
    GL2D_VALIDATE(requireNonNegative(snapshot.entitySpeedMul, "entitySpeedMul"));
    GL2D_VALIDATE(requireNonNegative(snapshot.animationSpeedMul, "animationSpeedMul"));
    GL2D_VALIDATE(requireNonNegative(snapshot.accelerationSpeedMul, "accelerationSpeedMul"));
    GL2D_VALIDATE(requireNonNegative(snapshot.damageMul, "damageMul"));
    GL2D_VALIDATE(requireNonNegative(snapshot.armorMul, "armorMul"));

    GL2D_VALIDATE(requireNonNegativeColor(snapshot.ambientLight, "ambientLight", true));
    GL2D_VALIDATE(requireNonNegativeColor(snapshot.fogColor, "fogColor", true));
    GL2D_VALIDATE(requireNonNegative(snapshot.fogDensity, "fogDensity"));
    GL2D_VALIDATE(requireNonNegative(snapshot.lightIntensityMul, "lightIntensityMul"));
    GL2D_VALIDATE(requireNonNegative(snapshot.lightRadiusMul, "lightRadiusMul"));
    GL2D_VALIDATE(requireNonNegativeColor(snapshot.lightColorMul, "lightColorMul"));
    GL2D_VALIDATE(requireNonNegativeColor(snapshot.ambientLightMul, "ambientLightMul"));
    GL2D_VALIDATE(requireFinite(snapshot.ambientLightAdd, "ambientLightAdd"));

    GL2D_VALIDATE(requireNonNegative(snapshot.musicVolume, "musicVolume"));
    GL2D_VALIDATE(requireNonNegative(snapshot.sfxVolumeMul, "sfxVolumeMul"));
    GL2D_VALIDATE(requireUnitRange(snapshot.reverbSend, "reverbSend"));
    GL2D_VALIDATE(requireUnitRange(snapshot.delaySend, "delaySend"));
    GL2D_VALIDATE(requireNonNegative(snapshot.delayTimeMs, "delayTimeMs"));
    GL2D_VALIDATE(requireUnitRange(snapshot.delayFeedback, "delayFeedback"));
    GL2D_VALIDATE(requireNonNegative(snapshot.lowpassHz, "lowpassHz"));
    GL2D_VALIDATE(requireNonNegative(snapshot.highpassHz, "highpassHz"));

    GL2D_VALIDATE(requireNonNegativeColor(snapshot.uiTint, "uiTint", true));
    GL2D_VALIDATE(requireNonNegative(snapshot.uiLerpSpeed, "uiLerpSpeed"));

#undef GL2D_VALIDATE
    return std::nullopt;
}

} // namespace FeelingsSystem
