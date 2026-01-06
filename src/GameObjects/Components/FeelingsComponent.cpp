//
// Created by Mohamad on 08/12/2025.
//

#include "FeelingsComponent.hpp"

#include <optional>

namespace {
std::optional<float> makeBlend(const FeelingsSystem::FeelingSnapshot &snapshot) {
    if (snapshot.blendInMs <= 0.0f) {
        return std::nullopt;
    }
    return snapshot.blendInMs;
}
}

FeelingsComponent::FeelingsComponent(FeelingsSystem::FeelingsSystem &feelings,
                                     FeelingsSystem::FeelingSnapshot snapshot)
    : m_feelings(feelings), m_snapshot(std::move(snapshot)), m_pendingApply(true) {}

void FeelingsComponent::update(Entity &, double) {
    if (!m_pendingApply) {
        return;
    }
    m_feelings.setFeeling(m_snapshot, makeBlend(m_snapshot));
    m_pendingApply = false;
}

void FeelingsComponent::setSnapshot(FeelingsSystem::FeelingSnapshot snapshot) {
    m_snapshot = std::move(snapshot);
    m_pendingApply = true;
}

void FeelingsComponent::triggerFeeling() {
    m_pendingApply = true;
}
