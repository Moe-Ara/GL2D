#pragma once

#include "GameObjects/Texture.hpp"
#include "Graphics/LightingSystem/LightEffector.hpp"
#include "Graphics/LightingSystem/LightType.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <memory>

namespace ECS {

// Authored light data. Transform2D supplies the world anchor and rotation.
// Animation is deliberately separate so static lights remain compact.
struct Light2D {
    LightType type{LightType::POINT};
    glm::vec2 localOffset{0.0f};
    glm::vec2 direction{0.0f, -1.0f};
    glm::vec3 color{1.0f};
    float radius{300.0f};
    float intensity{1.0f};
    float falloff{2.0f};
    float emissiveBoost{0.0f};
    float innerCutoff{0.9f};
    float outerCutoff{0.7f};
    std::shared_ptr<GameObjects::Texture> cookie{};
    float cookieStrength{0.0f};
    bool enabled{true};

    [[nodiscard]] static Light2D point(float radius, glm::vec3 color,
                                       float intensity = 1.0f) {
        Light2D light{};
        light.radius = radius;
        light.color = color;
        light.intensity = intensity;
        return light;
    }

    [[nodiscard]] static Light2D directional(glm::vec2 direction,
                                             glm::vec3 color,
                                             float intensity = 1.0f) {
        Light2D light{};
        light.type = LightType::DIRECTIONAL;
        light.direction = direction;
        light.color = color;
        light.intensity = intensity;
        return light;
    }

    [[nodiscard]] static Light2D spot(float radius, glm::vec2 direction,
                                      glm::vec3 color,
                                      float intensity = 1.0f) {
        Light2D light = point(radius, color, intensity);
        light.type = LightType::SPOT;
        light.direction = direction;
        return light;
    }
};

struct LightAnimation2D {
    LightEffector effector{};
};

} // namespace ECS
