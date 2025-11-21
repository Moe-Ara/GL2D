//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_TRANSFORMCOMPONENT_HPP
#define GL2D_TRANSFORMCOMPONENT_HPP


#include "GameObjects/IComponent.hpp"
#include "Utils/Transform.hpp"

class TransformComponent : public IComponent{
public:
    TransformComponent() = default;
    ~TransformComponent() override = default;

    TransformComponent(const TransformComponent &other) = delete;
    TransformComponent &operator=(const TransformComponent &other) = delete;
    TransformComponent(TransformComponent &&other) = delete;
    TransformComponent &operator=(TransformComponent &&other) = delete;
    Transform& getTransform();
    const Transform& getTransform() const;
    // Convenience passthroughs to keep model matrix in sync.
    void setPosition(const glm::vec2 &pos);
    void setScale(const glm::vec2 &scale);
    void setRotation(float r);
    const glm::mat4 &modelMatrix() const;
private:
    Transform m_transform{};

};


#endif //GL2D_TRANSFORMCOMPONENT_HPP
