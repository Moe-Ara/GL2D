#pragma once

#include <glm/vec3.hpp>

namespace Rendering {

struct PostProcessSettings {
    bool enabled{true};
    bool bloomEnabled{true};
    float exposure{1.0f};
    float gamma{2.2f};
    float bloomThreshold{1.0f};
    float bloomSoftKnee{0.5f};
    float bloomStrength{0.35f};
    int bloomIterations{4};
    float saturation{1.0f};
    float contrast{1.0f};
    glm::vec3 colorTint{1.0f, 1.0f, 1.0f};
    float vignetteStrength{0.2f};
    float vignetteSoftness{0.45f};
    // Fraction of the frame height covered by each cinematic letterbox bar
    // (0 = none, up to 0.45). Animate it for slide-in bars.
    float letterboxAmount{0.0f};
};

} // namespace Rendering
