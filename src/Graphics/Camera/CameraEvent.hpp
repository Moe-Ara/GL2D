//
// Created by Mohamad on 19/11/2025.
//

#ifndef GL2D_CAMERAEVENT_HPP
#define GL2D_CAMERAEVENT_HPP

#include <glm/vec2.hpp>

enum class CameraEventType{
    Shake,
    ZoomTo,
    Pulse,
    HitStop,
    SnapTo,
    Fade
};
struct CameraEvent{
    CameraEventType type;
    glm::vec2 worldPos{0.0f};
    float magnitude=0.0f;
    float duration=0.0f;
    float targetZoom=1.0f;
};

#endif //GL2D_CAMERAEVENT_HPP
