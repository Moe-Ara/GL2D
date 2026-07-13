//
// Created by Mohamad on 23/11/2025.
//

#ifndef GL2D_RIGIDBODY_HPP
#define GL2D_RIGIDBODY_HPP
#include "Physics/Collision/ICollider.hpp"
#include "Utils/Transform.hpp"
#include <functional>
#include <glm/vec2.hpp>
#include <memory>
#include <vector>

enum class RigidBodyType { STATIC, KINEMATIC, DYNAMIC };
// SUBSTEPPED adaptively divides fast movement to reduce tunnelling. CONTINUOUS
// remains as a source-compatible alias; this engine does not claim exact TOI CCD.
enum class CollisionDetection {
  DISCRETE,
  SUBSTEPPED,
  CONTINUOUS = SUBSTEPPED
};
class PhysicsEngine;
class RigidBody {
public:
  explicit RigidBody(float m = 1.0f, RigidBodyType type = RigidBodyType::DYNAMIC);

  ~RigidBody() = default;

  RigidBody(const RigidBody &other) = delete;

  RigidBody &operator=(const RigidBody &other) = delete;

  RigidBody(RigidBody &&other) = delete;

  RigidBody &operator=(RigidBody &&other) = delete;

  void setTransform(Transform *transform);
  Transform *getTransform() const { return m_transform; }

  void setCollider(ICollider *collider);
  ICollider *getCollider() const { return m_collider; }

  void setPosition(const glm::vec2 &pos);
  const glm::vec2 &getPosition() const { return m_position; }

  void setMass(float mass);
  float getMass() const { return m_mass; }
  float getInvMass() const { return m_invMass; }

  void setBodyType(RigidBodyType type);
  RigidBodyType getBodyType() const { return m_bodyType; }

  void setLinearDamping(float damping);
  float getLinearDamping() const { return m_linearDamping; }
  void setGravityScale(float scale);
  float getGravityScale() const { return m_gravityScale; }

  // Contact material. Friction is a Coulomb coefficient (>= 0); restitution is
  // the bounciness in [0, 1]. Combined at each contact by the solver: friction
  // uses the geometric mean and restitution the maximum of the two bodies, so a
  // frictionless/inelastic body (e.g. a controller-driven character) keeps its
  // contacts clean regardless of what it touches.
  void setFriction(float friction);
  float getFriction() const { return m_friction; }
  void setRestitution(float restitution);
  float getRestitution() const { return m_restitution; }

  void applyForce(const glm::vec2 &force);
  void applyImpulse(const glm::vec2 &impulse);

  void setVelocity(const glm::vec2 &v);
  const glm::vec2 &getVelocity() const { return m_velocity; }

  void setAngularVelocity(float v);
  float getAngularVelocity() const { return m_angularVelocity; }

  void applyTorque(float torque);

  void setRotation(float radians);
  float getRotation() const { return m_rotation; }

  void setInertia(float inertia);
  float getInvInertia() const { return m_invInertia; }
  void setAngularDamping(float damping);
  float getAngularDamping() const { return m_angularDamping; }
  void setCollisionDetection(CollisionDetection mode) noexcept {
    m_detectionType = mode;
  }
  [[nodiscard]] CollisionDetection getCollisionDetection() const noexcept {
    return m_detectionType;
  }

  // Force generators: return true to stay registered, false to self-remove.
  void addForceGenerator(std::function<bool(RigidBody &, float)> generator);
  void clearForceGenerators(); // clears all (e.g., when resetting body)

  void integrate(float dt);

private:
  friend class PhysicsEngine;
  struct StepLoads {
    glm::vec2 force{0.0f};
    float torque{0.0f};
  };

  StepLoads prepareStep(float dt);
  void integratePrepared(float dt, const StepLoads& loads);
  void updateInverseMassAndInertia() noexcept;
  glm::vec2 m_velocity{};
  glm::vec2 m_position{};
  glm::vec2 m_forces{};

  float m_mass{1.0f};
  float m_invMass{1.0f};
  float m_linearDamping{0.f};
  float m_gravityScale{1.0f};
  float m_friction{0.4f};
  float m_restitution{0.0f};

  float m_rotation{0.0f};
  float m_angularVelocity{0.0f};
  float m_torque{0.0f};
  float m_inertia{1.0f};
  float m_invInertia{1.0f};
  float m_angularDamping{0.0f};

  ICollider *m_collider{nullptr}; // non-owning
  Transform *m_transform{nullptr}; // non-owning
  RigidBodyType m_bodyType{RigidBodyType::DYNAMIC};
  CollisionDetection m_detectionType{CollisionDetection::SUBSTEPPED};

  std::vector<std::function<bool(RigidBody &, float)>> m_forceGenerators{};
};

#endif // GL2D_RIGIDBODY_HPP
