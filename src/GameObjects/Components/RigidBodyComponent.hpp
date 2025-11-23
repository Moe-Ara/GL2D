//
// Created by Mohamad on 23/11/2025.
//

#ifndef GL2D_RIGIDBODYCOMPONENT_HPP
#define GL2D_RIGIDBODYCOMPONENT_HPP


#include <memory>
#include "GameObjects/IComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "Physics/RigidBody.hpp"

class RigidBodyComponent : public IUpdatableComponent {
public:
    explicit RigidBodyComponent(std::unique_ptr<RigidBody> rigidBody = nullptr);

    virtual ~RigidBodyComponent() = default;

    RigidBodyComponent(const RigidBodyComponent &other) = delete;

    RigidBodyComponent &operator=(const RigidBodyComponent &other) = delete;

    RigidBodyComponent(RigidBodyComponent &&other) = delete;

    RigidBodyComponent &operator=(RigidBodyComponent &&other) = delete;

    void update(Entity &owner, double dt) override;

    RigidBody* body() const { return m_body.get(); }
    void setBody(std::unique_ptr<RigidBody> body);

private:
    void bindOwner(Entity &owner);

    std::unique_ptr<RigidBody> m_body{};
    bool m_bound{false};
};


#endif //GL2D_RIGIDBODYCOMPONENT_HPP
