#ifndef ACOLLIDER_HPP
#define ACOLLIDER_HPP

#include "AABB.hpp"
#include "ICollider.hpp"
#include "Utils/Transform.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <memory>

class ACollider : public ICollider {
public:
    virtual ~ACollider() = default;

    virtual ColliderType getType() const = 0;
    virtual AABB getAABB() const = 0;

    std::unique_ptr<Hit> hit(const ICollider &other) const override;
    void setTransform(Transform *transform) override;
    Transform &getTransform() const override;

    void setTrigger(bool isTrigger, bool fireOnce = false);
    bool isTrigger() const { return m_isTrigger; }
    bool triggersOnce() const { return m_fireOnce; }
    bool hasTriggered() const { return m_triggered; }
    void clearTriggerState() { m_triggered = false; }
    void markTriggerFired() { m_triggered = true; }
    // Layer filtering: layer is 0-31; collisionMask uses bitmask (1<<layer).
    void setLayer(uint32_t layer);
    uint32_t getLayer() const { return m_layer; }
    void setCollisionMask(uint32_t mask) { m_collisionMask = mask; }
    uint32_t getCollisionMask() const { return m_collisionMask; }
    bool allowsCollisionWith(const ACollider& other) const;

protected:
    Transform *tryGetTransform() const { return m_transform; }

private:
    Transform *m_transform{nullptr};
    bool m_isTrigger{false};
    bool m_fireOnce{false};
    bool m_triggered{false};
    uint32_t m_layer{0};
    uint32_t m_collisionMask{0xFFFFFFFFu};
};

#endif
