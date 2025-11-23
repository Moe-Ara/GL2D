#ifndef COLLISION_DISPATCHER_HPP
#define COLLISION_DISPATCHER_HPP

#include "ICollider.hpp"
#include <memory>

class AABBCollider;
class CircleCollider;
class CapsuleCollider;

class CollisionDispatcher {
public:
    CollisionDispatcher() = delete;

    virtual ~CollisionDispatcher() = default;

    static std::unique_ptr<Hit> dispatch(const ICollider &a, const ICollider &b);

private:
    static std::unique_ptr<Hit> collideAABB_AABB(const AABBCollider &a, const AABBCollider &b);

    static std::unique_ptr<Hit> collideCircle_Circle(const CircleCollider &a, const CircleCollider &b);

    static std::unique_ptr<Hit> collideCircle_AABB(const CircleCollider &c, const AABBCollider &b);

    static std::unique_ptr<Hit> collideCapsule_Circle(const CapsuleCollider &cap, const CircleCollider &c);

    static std::unique_ptr<Hit> collideCapsule_AABB(const CapsuleCollider &cap, const AABBCollider &box);

    static std::unique_ptr<Hit> collideCapsule_Capsule(const CapsuleCollider &cap, const CapsuleCollider &c);

};

#endif
