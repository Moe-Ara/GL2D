//
// Created by Mohamad on 19/11/2025.
//

#ifndef GL2D_ICAMERAEFFECT_HPP
#define GL2D_ICAMERAEFFECT_HPP

#include <glm/vec2.hpp>

struct CameraEffectState{
    glm::vec2 posOffset{0.0f};
    float zoomMultiplier=1.0f;
    float rotOffsetDeg=0.0f;
};
class ICameraEffect{
public:
    virtual ~ICameraEffect()=default;
    virtual bool update(double deltaTime, CameraEffectState& inOutState)=0;
};
#endif //GL2D_ICAMERAEFFECT_HPP
