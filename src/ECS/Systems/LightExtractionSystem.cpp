#include "ECS/Systems/LightExtractionSystem.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <ranges>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace ECS {
namespace {
glm::vec2 rotated(glm::vec2 value, float degrees) noexcept {
    const float angle = glm::radians(degrees);
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    return {value.x * cosine - value.y * sine,
            value.x * sine + value.y * cosine};
}

bool valid(const Transform2D& transform, const Light2D& light) noexcept {
    const std::array values{
        transform.position.x, transform.position.y, transform.scale.x,
        transform.scale.y, transform.rotationDegrees, light.localOffset.x,
        light.localOffset.y, light.direction.x, light.direction.y,
        light.color.x, light.color.y, light.color.z, light.radius,
        light.intensity, light.falloff, light.emissiveBoost,
        light.innerCutoff, light.outerCutoff, light.cookieStrength
    };
    const bool knownType = light.type == LightType::POINT ||
                           light.type == LightType::DIRECTIONAL ||
                           light.type == LightType::SPOT;
    return light.enabled && knownType &&
           std::ranges::all_of(values, [](float value) {
               return std::isfinite(value);
           }) &&
           light.color.x >= 0.0f && light.color.y >= 0.0f &&
           light.color.z >= 0.0f && light.intensity >= 0.0f &&
           light.falloff > 0.0f && light.emissiveBoost >= 0.0f &&
           light.cookieStrength >= 0.0f && light.cookieStrength <= 1.0f &&
           light.innerCutoff >= -1.0f && light.innerCutoff <= 1.0f &&
           light.outerCutoff >= -1.0f && light.outerCutoff <= 1.0f &&
           light.innerCutoff >= light.outerCutoff &&
           (light.type == LightType::DIRECTIONAL || light.radius > 0.0f);
}

bool valid(const LightEffector& effector) noexcept {
    const std::array values{
        effector.strength, effector.speed, effector.phase,
        effector.sweepDir.x, effector.sweepDir.y, effector.sweepSpan
    };
    return std::ranges::all_of(values, [](float value) {
               return std::isfinite(value);
           }) &&
           effector.strength >= 0.0f && effector.strength <= 1.0f &&
           effector.sweepSpan >= 0.0f;
}
} // namespace

void applyLightEffector(Light& light, const LightEffector& effector,
                        double timeSeconds) noexcept {
    const float time = static_cast<float>(timeSeconds);
    switch (effector.type) {
        case LightEffector::Type::Flicker:
        case LightEffector::Type::Pulse: {
            const float wave = 0.5f *
                (std::sin(time * effector.speed + effector.phase) + 1.0f);
            const float factor = 1.0f - effector.strength * 0.5f +
                                 effector.strength * wave;
            light.intensity *= std::max(0.1f, factor);
            if (effector.type == LightEffector::Type::Pulse) {
                light.radius *= std::max(0.5f, factor);
            }
            break;
        }
        case LightEffector::Type::Sweep: {
            glm::vec2 direction = effector.sweepDir;
            const float length = glm::length(direction);
            if (length > 0.0001f) {
                direction /= length;
            }
            const float wave = std::sin(
                time * effector.speed + effector.phase);
            light.pos += direction * effector.sweepSpan * wave;
            break;
        }
    }
}

std::optional<Light> extractLight(const Transform2D& transform,
                                  const Light2D& light,
                                  const LightAnimation2D* animation,
                                  double timeSeconds) noexcept {
    if (!valid(transform, light) ||
        (animation && !valid(animation->effector)) ||
        !std::isfinite(timeSeconds)) {
        return std::nullopt;
    }

    Light result{};
    result.type = light.type;
    result.pos = transform.position +
        rotated(light.localOffset * transform.scale, transform.rotationDegrees);
    result.dir = rotated(light.direction, transform.rotationDegrees);
    const float directionLength = glm::length(result.dir);
    result.dir = directionLength > 0.0001f
        ? result.dir / directionLength : glm::vec2{0.0f, -1.0f};
    result.radius = light.radius;
    result.color = light.color;
    result.intensity = light.intensity;
    result.falloff = light.falloff;
    result.emissiveBoost = light.emissiveBoost;
    result.innerCutoff = light.innerCutoff;
    result.outerCutoff = light.outerCutoff;
    result.cookieStrength = light.cookieStrength;
    if (animation) {
        applyLightEffector(result, animation->effector, timeSeconds);
    }
    return result;
}

} // namespace ECS
