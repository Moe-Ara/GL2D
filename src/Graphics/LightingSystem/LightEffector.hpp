//
// LightEffector.hpp
//

#ifndef GL2D_LIGHTEFFECTOR_HPP
#define GL2D_LIGHTEFFECTOR_HPP

#include <glm/vec2.hpp>

// Runtime effect applied to a light before sending to GPU.
struct LightEffector {
    enum class Type { Flicker, Pulse, Sweep };

    Type type{Type::Flicker};
    float strength{0.2f};   // 0..1 amplitude modifier
    float speed{1.5f};      // cycles per second
    float phase{0.0f};      // radians offset
    glm::vec2 sweepDir{1.0f, 0.0f}; // used for Sweep
    float sweepSpan{300.0f};        // units traveled for Sweep
};

#endif // GL2D_LIGHTEFFECTOR_HPP
