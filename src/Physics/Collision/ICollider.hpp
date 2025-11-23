//
// Created by Mohamad on 23/11/2025.
//

#ifndef GL2D_ICOLLIDER_HPP
#define GL2D_ICOLLIDER_HPP

#include "AABB.hpp"
#include "Utils/Transform.hpp"
#include <glm/glm.hpp>
#include <memory>
struct Hit {
    bool collided = false;
    glm::vec2 normal{0, 0};      // Expected to be normalized when collided is true
    float penetration = 0.f;     // Non-negative depth along the collision normal
    glm::vec2 contactPoint{0, 0};
};
enum class ColliderType { AABB, CIRCLE, CAPSULE, POLYGON, COMPOSITE };
class ICollider {
public:
    virtual ~ICollider() = default;

    virtual ColliderType getType() const = 0;
    virtual AABB getAABB() const = 0;

    virtual std::unique_ptr<Hit> hit(const ICollider& other) const = 0;
    // Optional hook to bind an external transform; default no-op.
    virtual void setTransform(Transform* /*transform*/) {}
    virtual Transform& getTransform() const = 0;

};

#endif //GL2D_ICOLLIDER_HPP
