//
// Created by Mohamad on 26/11/2025.
//

#ifndef GL2D_LIGHT_HPP
#define GL2D_LIGHT_HPP
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "LightType.hpp"

// CPU-side light representation used by the lighting pass.
struct Light {
    LightType type{LightType::POINT};
    glm::vec2 pos{0.0f};
    glm::vec2 dir{0.0f, -1.0f}; // used for directional/spot
    float radius{0.0f};         // point/spot only
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{0.0f};
    float falloff{1.0f};        // attenuation exponent
    float innerCutoff{0.8f};    // cos(theta); spot only
    float outerCutoff{0.5f};    // cos(theta); spot only
    float emissiveBoost{0.0f};
    int cookieSlot{-1};         // index into bound cookie samplers
    float cookieStrength{0.0f}; // 0 disables; 1 fully applies cookie
    // Effector-derived offsets (applied per-frame on CPU).
    float intensityMul{1.0f};
    float radiusMul{1.0f};
    glm::vec2 posOffset{0.0f};
};
#endif //GL2D_LIGHT_HPP
