//
// Created by Mohamad on 26/11/2025.
//

#include "FeelingsManager.hpp"

#include <algorithm>
#include <glm/glm.hpp>
namespace FeelingsSystem{
    
FeelingsManager::FeelingsManager() = default;
FeelingsManager::~FeelingsManager() = default;

void FeelingsManager::setFeeling(const FeelingSnapshot& feeling, std::optional<float> blendMs) {
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
    if (!m_isBlending) {
        return;
    }
    m_elapsedMs += deltaMs;
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

    // Blendable fields: lerp if target exists, otherwise keep start until finished then clear.
    auto blendField = [t](auto& out, const auto& startOpt, const auto& targetOpt) {
        out = FeelingsManager::lerpOptional(startOpt, targetOpt, t);
        if (!targetOpt.has_value() && t >= 1.0f) {
            out.reset();
        }
    };

    // Identity / meta
    m_snapshot.id = m_target.id;
    m_snapshot.blendInMs = m_target.blendInMs;
    m_snapshot.blendOutMs = m_target.blendOutMs;

    // Rendering
    blendField(m_snapshot.colorGrade, m_start.colorGrade, m_target.colorGrade);
    blendField(m_snapshot.vignette, m_start.vignette, m_target.vignette);
    blendField(m_snapshot.bloomStrength, m_start.bloomStrength, m_target.bloomStrength);
    stepDiscrete(m_snapshot.paletteID, m_start.paletteID, m_target.paletteID);

    // Camera
    blendField(m_snapshot.zoomMul, m_start.zoomMul, m_target.zoomMul);
    blendField(m_snapshot.offset, m_start.offset, m_target.offset);
    blendField(m_snapshot.shakeMagnitude, m_start.shakeMagnitude, m_target.shakeMagnitude);
    blendField(m_snapshot.shakeRoughness, m_start.shakeRoughness, m_target.shakeRoughness);
    blendField(m_snapshot.parallaxBias, m_start.parallaxBias, m_target.parallaxBias);
    blendField(m_snapshot.followSpeedMul, m_start.followSpeedMul, m_target.followSpeedMul);

    // Gameplay
    blendField(m_snapshot.timeScale, m_start.timeScale, m_target.timeScale);
    blendField(m_snapshot.entitySpeedMul, m_start.entitySpeedMul, m_target.entitySpeedMul);
    blendField(m_snapshot.animationSpeedMul, m_start.animationSpeedMul, m_target.animationSpeedMul);

    // Particles / VFX
    stepDiscrete(m_snapshot.particlePresetId, m_start.particlePresetId, m_target.particlePresetId);
    blendField(m_snapshot.ambientLight, m_start.ambientLight, m_target.ambientLight);
    blendField(m_snapshot.fogColor, m_start.fogColor, m_target.fogColor);
    blendField(m_snapshot.fogDensity, m_start.fogDensity, m_target.fogDensity);

    // Audio (music / routing / FX)
    stepDiscrete(m_snapshot.musicTrackId, m_start.musicTrackId, m_target.musicTrackId);
    blendField(m_snapshot.musicVolume, m_start.musicVolume, m_target.musicVolume);
    stepDiscrete(m_snapshot.sfxTag, m_start.sfxTag, m_target.sfxTag);
    blendField(m_snapshot.sfxVolumeMul, m_start.sfxVolumeMul, m_target.sfxVolumeMul);
    stepDiscrete(m_snapshot.audioFxPreset, m_start.audioFxPreset, m_target.audioFxPreset);
    blendField(m_snapshot.reverbSend, m_start.reverbSend, m_target.reverbSend);
    blendField(m_snapshot.delaySend, m_start.delaySend, m_target.delaySend);
    blendField(m_snapshot.delayTimeMs, m_start.delayTimeMs, m_target.delayTimeMs);
    blendField(m_snapshot.delayFeedback, m_start.delayFeedback, m_target.delayFeedback);
    blendField(m_snapshot.lowpassHz, m_start.lowpassHz, m_target.lowpassHz);
    blendField(m_snapshot.highpassHz, m_start.highpassHz, m_target.highpassHz);

    // UI
    blendField(m_snapshot.uiTint, m_start.uiTint, m_target.uiTint);
    blendField(m_snapshot.uiLerpSpeed, m_start.uiLerpSpeed, m_target.uiLerpSpeed);

    if (t >= 1.0f) {
        m_snapshot = m_target;
        m_isBlending = false;
        m_blendDurationMs = 0.0f;
        m_elapsedMs = 0.0f;
    }
}
}
