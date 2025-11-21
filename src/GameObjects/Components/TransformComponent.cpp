//
// Created by Mohamad on 21/11/2025.
//

#include "TransformComponent.hpp"

Transform &TransformComponent::getTransform() {
    return m_transform;
}

const Transform &TransformComponent::getTransform() const {
    return m_transform;
}

void TransformComponent::setPosition(const glm::vec2 &pos) {
    m_transform.setPos(pos);
}

void TransformComponent::setScale(const glm::vec2 &scale) {
    m_transform.setScale(scale);
}

void TransformComponent::setRotation(float r) {
    m_transform.setRotation(r);
}

const glm::mat4 &TransformComponent::modelMatrix() const {
    return m_transform.getModelMatrix();
}
