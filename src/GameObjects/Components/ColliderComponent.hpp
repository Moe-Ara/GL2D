//
// ColliderComponent.hpp
//

#ifndef GL2D_COLLIDERCOMPONENT_HPP
#define GL2D_COLLIDERCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/Collision/ACollider.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <glm/vec2.hpp>

class Entity;

class ColliderComponent : public IUpdatableComponent {
public:
    explicit ColliderComponent(std::unique_ptr<ACollider> collider = nullptr,
                               ColliderType requestedType = ColliderType::AABB,
                               float padding = 0.0f);
    ~ColliderComponent() override = default;

    ColliderComponent(const ColliderComponent&) = delete;
    ColliderComponent& operator=(const ColliderComponent&) = delete;
    ColliderComponent(ColliderComponent&&) = delete;
    ColliderComponent& operator=(ColliderComponent&&) = delete;

    void update(Entity& owner, double dt) override;

    ACollider* collider() const { return m_collider.get(); }
    void setCollider(std::unique_ptr<ACollider> collider);
    void setRequestedType(ColliderType type) { m_requestedType = type; }
    void setPadding(float padding) { m_padding = padding; }
    void setTrigger(bool isTrigger, bool fireOnce = false) {
        m_isTrigger = isTrigger;
        m_triggerOnce = fireOnce;
        if (m_collider) {
            m_collider->setTrigger(isTrigger, fireOnce);
        }
    }
    void setLayer(uint32_t layer);
    uint32_t getLayer() const { return m_layer; }
    void setCollisionMask(uint32_t mask);
    uint32_t getCollisionMask() const { return m_collisionMask; }
    bool isTrigger() const { return m_isTrigger; }
    bool triggersOnce() const { return m_triggerOnce; }
    void setOnTriggerEnter(std::function<void(Entity&, Entity&)> callback) { m_onTriggerEnter = std::move(callback); }
    void setOnTriggerExit(std::function<void(Entity&, Entity&)> callback) { m_onTriggerExit = std::move(callback); }
    void invokeTriggerEnter(Entity& owner, Entity& other);
    void invokeTriggerExit(Entity& owner, Entity& other);
    void setCapsuleSizeOverride(const glm::vec2& size);
    void clearCapsuleSizeOverride();
    void setCapsuleOffsetOverride(const glm::vec2& offset);
    void clearCapsuleOffsetOverride();
    // Try to fit collider bounds to the owner's sprite (currently supports AABB colliders).
    bool fitToSprite(Entity& owner, float padding = 0.0f);
    // Ensure a collider exists; if missing, create based on requested type.
    void ensureCollider(Entity& owner);
    // Debug draw the collider (wireframe). Expects an orthographic viewProj already set.
    void debugDraw(const glm::mat4& viewProj, const glm::vec3& color = {1.0f, 0.0f, 0.0f}) const;

private:
    std::unique_ptr<ACollider> createCollider(ColliderType type) const;
    void tryBindTransform(Entity& owner);

    std::unique_ptr<ACollider> m_collider{};
    ColliderType m_requestedType{ColliderType::AABB};
    float m_padding{0.0f};
    bool m_isTrigger{false};
    bool m_triggerOnce{false};
    uint32_t m_layer{0};
    uint32_t m_collisionMask{0xFFFFFFFFu};
    bool m_transformBound{false};
    glm::vec2 m_capsuleSizeOverride{0.0f, 0.0f};
    bool m_capsuleOverrideActive{false};
    glm::vec2 m_capsuleOffsetOverride{0.0f, 0.0f};
    bool m_capsuleOffsetOverrideActive{false};
    std::function<void(Entity&, Entity&)> m_onTriggerEnter{};
    std::function<void(Entity&, Entity&)> m_onTriggerExit{};
};

#endif //GL2D_COLLIDERCOMPONENT_HPP
