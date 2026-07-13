#include "ECS/Animation/AnimationGraph2D.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace ECS {
namespace {
constexpr float kMinimumFrameDuration = 1e-5f;

bool finite(const glm::vec4& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z) && std::isfinite(value.w);
}
}

AnimationGraph2D::AnimationGraph2D(
    std::vector<AnimationState2D> states,
    std::vector<AnimationTransition2D> transitions,
    std::string initialState)
    : m_states(std::move(states)) {
    if (m_states.empty()) {
        throw std::invalid_argument("AnimationGraph2D requires at least one state");
    }

    for (std::size_t stateIndex = 0; stateIndex < m_states.size(); ++stateIndex) {
        const AnimationState2D& animationState = m_states[stateIndex];
        if (animationState.name.empty() ||
            !m_stateIndices.emplace(animationState.name, stateIndex).second) {
            throw std::invalid_argument("Animation state names must be non-empty and unique");
        }
        if (animationState.clip.frames.empty()) {
            throw std::invalid_argument("Animation state '" + animationState.name + "' has no frames");
        }
        for (const AnimationFrame2D& frame : animationState.clip.frames) {
            if (!std::isfinite(frame.durationSeconds) ||
                frame.durationSeconds < kMinimumFrameDuration ||
                !finite(frame.uvRect) || !finite(frame.tint)) {
                throw std::invalid_argument(
                    "Animation state '" + animationState.name + "' has an invalid frame");
            }
        }
    }

    const auto initial = findState(initialState);
    if (!initial) {
        throw std::invalid_argument("Animation initial state does not exist: " + initialState);
    }
    m_initialStateIndex = *initial;

    m_transitions.reserve(transitions.size());
    for (AnimationTransition2D& transition : transitions) {
        if ((transition.fromState != "*" && !findState(transition.fromState)) ||
            !findState(transition.toState)) {
            throw std::invalid_argument("Animation transition references an unknown state");
        }
        if (transition.condition != AnimationCondition2D::Always &&
            transition.parameter.empty()) {
            throw std::invalid_argument("Conditional animation transition requires a parameter");
        }
        if (!std::isfinite(transition.threshold) ||
            !std::isfinite(transition.minimumStateSeconds) ||
            transition.minimumStateSeconds < 0.0f) {
            throw std::invalid_argument("Animation transition contains invalid timing or threshold data");
        }
        m_transitions.push_back({
            transition.fromState == "*" ? 0u : *findState(transition.fromState),
            *findState(transition.toState), transition.fromState == "*",
            transition.condition, std::move(transition.parameter), transition.threshold,
            transition.expectedBool, transition.minimumStateSeconds
        });
    }
}

const AnimationState2D& AnimationGraph2D::state(std::size_t index) const {
    if (index >= m_states.size()) {
        throw std::out_of_range("Animation state index is out of range");
    }
    return m_states[index];
}

std::optional<std::size_t> AnimationGraph2D::findState(std::string_view name) const {
    const auto found = m_stateIndices.find(std::string{name});
    return found == m_stateIndices.end()
        ? std::nullopt : std::optional<std::size_t>{found->second};
}

} // namespace ECS
