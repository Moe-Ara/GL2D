//
// Created by Mohamad on 26/11/2025.
//

#include "LightingComponent.hpp"

LightingComponent::LightingComponent(const std::string &id, LightType type, glm::vec2 pos, float radius,
                                     glm::vec3 color, float intensity, float falloff, float emissiveBoost,
                                     glm::vec2 dir, float innerCutoff, float outerCutoff,
                                     const std::string& cookiePath, float cookieStrength,
                                     std::optional<LightEffector> effector) :
        m_id(id), m_type(type),
        m_localPos(pos), m_radius(radius),
        m_color(color), m_intensity(intensity),
        m_falloff(falloff), m_emissiveBoost(emissiveBoost),
        m_dir(dir), m_innerCutoff(innerCutoff), m_outerCutoff(outerCutoff),
        m_cookiePath(cookiePath), m_cookieStrength(cookieStrength),
        m_effector(std::move(effector)) {

}

const std::string &LightingComponent::id() const {
    return m_id;
}

LightType LightingComponent::type() const {
    return m_type;
}

glm::vec2 LightingComponent::localPos() const {
    return m_localPos;
}

float LightingComponent::radius() const {
    return m_radius;
}

glm::vec3 LightingComponent::color() const {
    return m_color;
}

float LightingComponent::intensity() const {
    return m_intensity;
}

float LightingComponent::falloff() const {
    return m_falloff;
}

float LightingComponent::emissiveBoost() const {
    return m_emissiveBoost;
}

glm::vec2 LightingComponent::direction() const {
    return m_dir;
}

float LightingComponent::innerCutoff() const {
    return m_innerCutoff;
}

float LightingComponent::outerCutoff() const {
    return m_outerCutoff;
}

const std::string &LightingComponent::cookiePath() const {
    return m_cookiePath;
}

float LightingComponent::cookieStrength() const {
    return m_cookieStrength;
}

void LightingComponent::setEffector(const std::optional<LightEffector> &eff) {
    m_effector = eff;
}

void LightingComponent::setLocalPos(glm::vec2 position) {
    m_localPos = position;
}

void LightingComponent::setRadius(float radius) {
    m_radius = radius;
}

void LightingComponent::setColor(glm::vec3 color) {
    m_color = color;
}

void LightingComponent::setIntensity(float intensity) {
    m_intensity = intensity;
}

void LightingComponent::setFalloff(float falloff) {
    m_falloff = falloff;
}

void LightingComponent::setEmissiveBoost(float emissiveBoost) {
    m_emissiveBoost = emissiveBoost;
}

void LightingComponent::setDirection(glm::vec2 dir) {
    m_dir = dir;
}

void LightingComponent::setCutoff(float inner, float outer) {
    m_innerCutoff = inner;
    m_outerCutoff = outer;
}

void LightingComponent::setCookie(const std::string &path, float strength) {
    m_cookiePath = path;
    m_cookieStrength = strength;
}
