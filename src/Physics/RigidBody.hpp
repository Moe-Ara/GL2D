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
enum class CollisionDetection { CONTINUOUS, DISCRETE };
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

  void setLinearDamping(float d) { m_linearDamping = d; }
  float getLinearDamping() const { return m_linearDamping; }

  void applyForce(const glm::vec2 &force) { m_forces += force; }
  void applyImpulse(const glm::vec2 &impulse) { m_velocity += impulse * m_invMass; }

  void setVelocity(const glm::vec2 &v) { m_velocity = v; }
  const glm::vec2 &getVelocity() const { return m_velocity; }

  void setAngularVelocity(float v) { m_angularVelocity = v; }
  float getAngularVelocity() const { return m_angularVelocity; }

  void applyTorque(float torque) { m_torque += torque; }

  void setRotation(float radians);
  float getRotation() const { return m_rotation; }

  void setInertia(float inertia);
  float getInvInertia() const { return m_invInertia; }
  void setAngularDamping(float damping) { m_angularDamping = damping; }
  float getAngularDamping() const { return m_angularDamping; }

  // Force generators: return true to stay registered, false to self-remove.
  void addForceGenerator(std::function<bool(RigidBody &, float)> generator);
  void clearForceGenerators(); // clears all (e.g., when resetting body)

  void integrate(float dt);

private:
  glm::vec2 m_velocity{};
  glm::vec2 m_position{};
  glm::vec2 m_forces{};

  float m_mass{1.0f};
  float m_invMass{1.0f};
  float m_linearDamping{0.f};

  float m_rotation{0.0f};
  float m_angularVelocity{0.0f};
  float m_torque{0.0f};
  float m_inertia{1.0f};
  float m_invInertia{1.0f};
  float m_angularDamping{0.0f};

  ICollider *m_collider{nullptr}; // non-owning
  Transform *m_transform{nullptr}; // non-owning
  RigidBodyType m_bodyType{RigidBodyType::DYNAMIC};
  CollisionDetection m_detectionType{CollisionDetection::CONTINUOUS};

  std::vector<std::function<bool(RigidBody &, float)>> m_forceGenerators{};
};

#endif // GL2D_RIGIDBODY_HPP
