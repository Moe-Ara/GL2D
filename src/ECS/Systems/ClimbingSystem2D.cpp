#include "ClimbingSystem2D.hpp"

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Climbable2D.hpp"
#include "ECS/Components/Collision2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace ECS {
namespace {
constexpr float kOverlapEpsilon = 1e-5f;

struct Bounds {
    glm::vec2 min{0.0f};
    glm::vec2 max{0.0f};
    [[nodiscard]] glm::vec2 center() const noexcept {
        return (min + max) * 0.5f;
    }
};

struct Candidate {
    Entity entity{};
    Bounds bounds{};
    glm::vec2 axis{0.0f, 1.0f};
    bool snapToAxis{true};
    std::uint32_t categoryBits{1u};
    std::uint32_t maskBits{0xFFFFFFFFu};
};

bool finite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

Bounds boundsFor(const Transform2D& transform, const AabbCollider2D& collider) {
    if (!finite(transform.position) || !finite(transform.scale) ||
        !finite(collider.halfExtents) || !finite(collider.offset) ||
        collider.halfExtents.x <= 0.0f || collider.halfExtents.y <= 0.0f) {
        throw std::invalid_argument("Climbable ECS AABB contains invalid bounds");
    }
    const glm::vec2 center = transform.position + collider.offset * transform.scale;
    const glm::vec2 half = glm::abs(collider.halfExtents * transform.scale);
    return {center - half, center + half};
}

bool overlaps(const Bounds& first, const Bounds& second) {
    return first.max.x > second.min.x + kOverlapEpsilon &&
           first.min.x < second.max.x - kOverlapEpsilon &&
           first.max.y > second.min.y + kOverlapEpsilon &&
           first.min.y < second.max.y - kOverlapEpsilon;
}
}

void ClimbingSystem2D::update(Registry& registry) {
    std::vector<Candidate> candidates;
    candidates.reserve(registry.size());
    registry.each<Transform2D, AabbCollider2D, Climbable2D>(
        [&](Entity entity, const Transform2D& transform,
            const AabbCollider2D& collider, const Climbable2D& climbable) {
            if (!finite(climbable.axis)) {
                throw std::invalid_argument("Climbable axis must be finite and non-zero");
            }
            const float axisLength = glm::length(climbable.axis);
            if (axisLength <= kOverlapEpsilon) {
                throw std::invalid_argument("Climbable axis must be finite and non-zero");
            }
            candidates.push_back({entity, boundsFor(transform, collider),
                                  climbable.axis / axisLength,
                                  climbable.snapToAxis, collider.categoryBits,
                                  collider.maskBits});
        });

    registry.each<Transform2D, AabbCollider2D, CharacterIntent, ClimbingState2D>(
        [&](Entity entity, const Transform2D& transform,
            const AabbCollider2D& collider, const CharacterIntent& intent,
            ClimbingState2D& state) {
            state.startedThisStep = false;
            state.endedThisStep = false;
            state.jumpedOffThisStep = false;
            const Bounds character = boundsFor(transform, collider);

            const Candidate* selected = nullptr;
            float selectedDistance = std::numeric_limits<float>::max();
            for (const Candidate& candidate : candidates) {
                if (candidate.entity == entity ||
                    (collider.maskBits & candidate.categoryBits) == 0u ||
                    (candidate.maskBits & collider.categoryBits) == 0u ||
                    !overlaps(character, candidate.bounds)) {
                    continue;
                }
                if (state.active && candidate.entity == state.climbable) {
                    selected = &candidate;
                    break;
                }
                const glm::vec2 delta =
                    character.center() - candidate.bounds.center();
                const float distance = glm::dot(delta, delta);
                if (distance < selectedDistance ||
                    (distance == selectedDistance && selected &&
                     candidate.entity.index() < selected->entity.index())) {
                    selected = &candidate;
                    selectedDistance = distance;
                }
            }

            if (state.active && intent.jumpPressed) {
                state.active = false;
                state.endedThisStep = true;
                state.jumpedOffThisStep = true;
                state.requireInputRelease = true;
                state.climbable = {};
                state.lateralError = 0.0f;
                return;
            }

            if (!selected) {
                state.endedThisStep = state.active;
                state.requireInputRelease |= state.active;
                state.active = false;
                state.climbable = {};
                state.lateralError = 0.0f;
                return;
            }

            if (state.requireInputRelease) {
                if (std::abs(intent.climbAxis) <= kOverlapEpsilon) {
                    state.requireInputRelease = false;
                } else {
                    state.active = false;
                    state.climbable = {};
                    state.lateralError = 0.0f;
                    return;
                }
            }

            if (!state.active && std::abs(intent.climbAxis) <= kOverlapEpsilon) {
                state.climbable = {};
                state.lateralError = 0.0f;
                return;
            }

            state.startedThisStep = !state.active;
            state.active = true;
            state.climbable = selected->entity;
            state.axis = selected->axis;
            if (selected->snapToAxis) {
                const glm::vec2 perpendicular{-state.axis.y, state.axis.x};
                state.lateralError = glm::dot(
                    selected->bounds.center() - character.center(), perpendicular);
            } else {
                state.lateralError = 0.0f;
            }
        });
}

} // namespace ECS
