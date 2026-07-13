#pragma once

#include <glm/vec4.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace GameObjects {
class Texture;
}

namespace ECS {

enum class AnimationPlayback2D {
    Loop,
    Once,
    LoopReverse,
    OnceReverse,
    PingPong
};

struct AnimationFrame2D {
    glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};
    float durationSeconds{1.0f / 12.0f};
    std::string event;
    glm::vec4 tint{1.0f};
    std::shared_ptr<GameObjects::Texture> texture;
    std::shared_ptr<GameObjects::Texture> normalTexture;
};

struct AnimationClip2D {
    std::vector<AnimationFrame2D> frames;
    AnimationPlayback2D playback{AnimationPlayback2D::Loop};
};

struct AnimationState2D {
    std::string name;
    AnimationClip2D clip;
};

enum class AnimationCondition2D {
    Always,
    BoolEquals,
    FloatGreater,
    FloatLess,
    FloatGreaterEqual,
    FloatLessEqual
};

struct AnimationTransition2D {
    // "*" matches every source state.
    std::string fromState;
    std::string toState;
    AnimationCondition2D condition{AnimationCondition2D::Always};
    std::string parameter;
    float threshold{0.0f};
    bool expectedBool{true};
    float minimumStateSeconds{0.0f};
};

struct CompiledAnimationTransition2D {
    std::size_t fromState{0};
    std::size_t toState{0};
    bool anySource{false};
    AnimationCondition2D condition{AnimationCondition2D::Always};
    std::string parameter;
    float threshold{0.0f};
    bool expectedBool{true};
    float minimumStateSeconds{0.0f};
};

// Immutable, validated animation resource shared by any number of entities.
class AnimationGraph2D {
public:
    AnimationGraph2D(std::vector<AnimationState2D> states,
                     std::vector<AnimationTransition2D> transitions,
                     std::string initialState);

    [[nodiscard]] const AnimationState2D& state(std::size_t index) const;
    [[nodiscard]] std::optional<std::size_t> findState(std::string_view name) const;
    [[nodiscard]] std::size_t initialStateIndex() const noexcept { return m_initialStateIndex; }
    [[nodiscard]] const std::vector<CompiledAnimationTransition2D>& transitions() const noexcept {
        return m_transitions;
    }

private:
    std::vector<AnimationState2D> m_states;
    std::vector<CompiledAnimationTransition2D> m_transitions;
    std::unordered_map<std::string, std::size_t> m_stateIndices;
    std::size_t m_initialStateIndex{0};
};

} // namespace ECS
