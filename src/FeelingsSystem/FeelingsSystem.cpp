//
// Created by Mohamad on 26/11/2025.
//

#include "FeelingsManager.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <glm/glm.hpp>
namespace FeelingsSystem{
    
FeelingsManager::FeelingsManager() = default;
FeelingsManager::~FeelingsManager() = default;

void FeelingsManager::setFeeling(const FeelingSnapshot& feeling, std::optional<float> blendMs) {
    if (const auto error = validationError(feeling)) {
        throw std::invalid_argument("Invalid feeling '" + feeling.id + "': " +
                                    *error);
    }
    if (blendMs && (!std::isfinite(*blendMs) || *blendMs < 0.0f)) {
        throw std::invalid_argument(
            "Feeling blend override must be finite and non-negative");
    }
    // If no blend specified, honor the snapshot's blend time.
    const float duration = blendMs.has_value() ? *blendMs : feeling.blendInMs;
    if (duration <= 0.0f) {
        m_snapshot = feeling;
        m_isBlending = false;
        m_blendDurationMs = 0.0f;
        m_elapsedMs = 0.0f;
        return;
    }

    m_start = m_snapshot;
    m_target = feeling;
    m_blendDurationMs = duration;
    m_elapsedMs = 0.0f;
    m_isBlending = true;
}

void FeelingsManager::update(float deltaMs) {
    if (!std::isfinite(deltaMs) || deltaMs < 0.0f) {
        throw std::invalid_argument(
            "FeelingsManager delta must be finite and non-negative");
    }
    if (!m_isBlending) {
        return;
    }
    m_elapsedMs = std::min(m_elapsedMs + deltaMs, m_blendDurationMs);
    const float t = (m_blendDurationMs <= 0.0f)
                        ? 1.0f
                        : std::clamp(m_elapsedMs / m_blendDurationMs, 0.0f, 1.0f);

    // Discrete fields: switch to target if set, otherwise keep start until finished.
    auto stepDiscrete = [t](auto& out, const auto& startOpt, const auto& targetOpt) {
        if (targetOpt.has_value()) {
            out = targetOpt;
        } else {
            out = (t < 1.0f) ? startOpt : std::nullopt;
        }
    };

    // Optional overrides blend through their field-specific neutral value. This
    // prevents effects from snapping when entering from or returning to "unset".
    auto blendOverride = [t](auto& out, const auto& startOpt,
                             const auto& targetOpt, const auto& neutral) {
        if (!startOpt && !targetOpt) {
            out.reset();
            return;
        }
        const auto from = startOpt.value_or(neutral);
        const auto to = targetOpt.value_or(neutral);
        out = from + (to - from) * t;
        if (!targetOpt && t >= 1.0f) out.reset();
    };

    // Identity / meta
    m_snapshot.id = m_target.id;
    m_snapshot.blendInMs = m_target.blendInMs;
    m_snapshot.blendOutMs = m_target.blendOutMs;

    // Rendering
    blendOverride(m_snapshot.colorGrade, m_start.colorGrade, m_target.colorGrade,
                  glm::vec4(1.0f));
    blendOverride(m_snapshot.vignette, m_start.vignette, m_target.vignette, 0.0f);
    blendOverride(m_snapshot.bloomStrength, m_start.bloomStrength,
                  m_target.bloomStrength, 0.0f);
    stepDiscrete(m_snapshot.paletteID, m_start.paletteID, m_target.paletteID);

    // Camera
    blendOverride(m_snapshot.zoomMul, m_start.zoomMul, m_target.zoomMul, 1.0f);
    blendOverride(m_snapshot.offset, m_start.offset, m_target.offset, glm::vec2(0.0f));
    blendOverride(m_snapshot.shakeMagnitude, m_start.shakeMagnitude,
                  m_target.shakeMagnitude, 0.0f);
    blendOverride(m_snapshot.shakeRoughness, m_start.shakeRoughness,
                  m_target.shakeRoughness, 0.0f);
    blendOverride(m_snapshot.parallaxBias, m_start.parallaxBias,
                  m_target.parallaxBias, glm::vec4(0.0f));
    blendOverride(m_snapshot.followSpeedMul, m_start.followSpeedMul,
                  m_target.followSpeedMul, 1.0f);

    // Gameplay
    blendOverride(m_snapshot.timeScale, m_start.timeScale, m_target.timeScale, 1.0f);
    blendOverride(m_snapshot.entitySpeedMul, m_start.entitySpeedMul,
                  m_target.entitySpeedMul, 1.0f);
    blendOverride(m_snapshot.animationSpeedMul, m_start.animationSpeedMul,
                  m_target.animationSpeedMul, 1.0f);
    blendOverride(m_snapshot.accelerationSpeedMul, m_start.accelerationSpeedMul,
                  m_target.accelerationSpeedMul, 1.0f);
    blendOverride(m_snapshot.damageMul, m_start.damageMul, m_target.damageMul, 1.0f);
    blendOverride(m_snapshot.armorMul, m_start.armorMul, m_target.armorMul, 1.0f);

    // Particles / VFX
    stepDiscrete(m_snapshot.particlePresetId, m_start.particlePresetId, m_target.particlePresetId);
    blendOverride(m_snapshot.ambientLight, m_start.ambientLight,
                  m_target.ambientLight, glm::vec4(1.0f));
    blendOverride(m_snapshot.fogColor, m_start.fogColor, m_target.fogColor,
                  glm::vec4(0.0f));
    blendOverride(m_snapshot.fogDensity, m_start.fogDensity,
                  m_target.fogDensity, 0.0f);
    blendOverride(m_snapshot.lightIntensityMul, m_start.lightIntensityMul,
                  m_target.lightIntensityMul, 1.0f);
    blendOverride(m_snapshot.lightRadiusMul, m_start.lightRadiusMul,
                  m_target.lightRadiusMul, 1.0f);
    blendOverride(m_snapshot.lightColorMul, m_start.lightColorMul,
                  m_target.lightColorMul, glm::vec3(1.0f));
    blendOverride(m_snapshot.ambientLightMul, m_start.ambientLightMul,
                  m_target.ambientLightMul, glm::vec3(1.0f));
    blendOverride(m_snapshot.ambientLightAdd, m_start.ambientLightAdd,
                  m_target.ambientLightAdd, glm::vec3(0.0f));

    // Audio (music / routing / FX)
    stepDiscrete(m_snapshot.musicTrackId, m_start.musicTrackId, m_target.musicTrackId);
    blendOverride(m_snapshot.musicVolume, m_start.musicVolume, m_target.musicVolume, 1.0f);
    stepDiscrete(m_snapshot.sfxTag, m_start.sfxTag, m_target.sfxTag);
    blendOverride(m_snapshot.sfxVolumeMul, m_start.sfxVolumeMul,
                  m_target.sfxVolumeMul, 1.0f);
    stepDiscrete(m_snapshot.audioFxPreset, m_start.audioFxPreset, m_target.audioFxPreset);
    blendOverride(m_snapshot.reverbSend, m_start.reverbSend, m_target.reverbSend, 0.0f);
    blendOverride(m_snapshot.delaySend, m_start.delaySend, m_target.delaySend, 0.0f);
    blendOverride(m_snapshot.delayTimeMs, m_start.delayTimeMs,
                  m_target.delayTimeMs, 0.0f);
    blendOverride(m_snapshot.delayFeedback, m_start.delayFeedback,
                  m_target.delayFeedback, 0.0f);
    // Filter overrides are discrete in practice: zero means bypass in the
    // current audio backend, so a neutral fade remains well-defined.
    blendOverride(m_snapshot.lowpassHz, m_start.lowpassHz, m_target.lowpassHz, 0.0f);
    blendOverride(m_snapshot.highpassHz, m_start.highpassHz, m_target.highpassHz, 0.0f);

    // UI
    blendOverride(m_snapshot.uiTint, m_start.uiTint, m_target.uiTint,
                  glm::vec4(1.0f));
    blendOverride(m_snapshot.uiLerpSpeed, m_start.uiLerpSpeed,
                  m_target.uiLerpSpeed, 1.0f);

    if (t >= 1.0f) {
        m_snapshot = m_target;
        m_isBlending = false;
        m_blendDurationMs = 0.0f;
        m_elapsedMs = 0.0f;
    }
}
}
