#pragma once

#include "ECS/Animation/AnimationGraph2D.hpp"

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ECS {

struct Animator2D {
    std::shared_ptr<const AnimationGraph2D> graph;
    std::size_t stateIndex{std::numeric_limits<std::size_t>::max()};
    std::size_t frameIndex{0};
    float frameElapsedSeconds{0.0f};
    float stateElapsedSeconds{0.0f};
    float playbackSpeed{1.0f};
    int frameDirection{1};
    bool playing{true};
    bool completed{false};
    std::string requestedState;
};

struct AnimationParameters2D {
    std::unordered_map<std::string, bool> boolValues;
    std::unordered_map<std::string, float> floatValues;

    void setBool(std::string name, bool value) { boolValues[std::move(name)] = value; }
    void setFloat(std::string name, float value) { floatValues[std::move(name)] = value; }
    [[nodiscard]] bool getBool(std::string_view name, bool fallback = false) const;
    [[nodiscard]] float getFloat(std::string_view name, float fallback = 0.0f) const;
};

enum class AnimationEventKind2D {
    StateEntered,
    StateExited,
    Frame,
    ClipLooped,
    Completed
};

struct AnimationEvent2D {
    AnimationEventKind2D kind{AnimationEventKind2D::Frame};
    std::string name;
    std::string state;
    std::size_t frameIndex{0};
};

struct AnimationEventQueue2D {
    std::vector<AnimationEvent2D> events;
};

} // namespace ECS
