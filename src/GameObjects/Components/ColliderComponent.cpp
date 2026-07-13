//
// ColliderComponent.cpp
//

#include "ColliderComponent.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/TriggerComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/CapsuleCollider.hpp"
#include "RenderingSystem/DebugDraw2D.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>

ColliderComponent::ColliderComponent(std::unique_ptr<ACollider> collider,
                                     ColliderType requestedType,
                                     float padding)
        : m_collider(std::move(collider)),
          m_requestedType(requestedType),
          m_padding(padding) {}

void ColliderComponent::setCollider(std::unique_ptr<ACollider> collider) {
    m_collider = std::move(collider);
    if (m_collider) {
        m_collider->setTrigger(m_isTrigger, m_triggerOnce);
        m_collider->setLayer(m_layer);
        m_collider->setCollisionMask(m_collisionMask);
    }
    m_transformBound = false;
}

void ColliderComponent::ensureCollider(Entity &owner) {
    if (!m_collider) {
        m_collider = createCollider(m_requestedType);
        m_transformBound = false;
        if (m_collider) {
            m_collider->setTrigger(m_isTrigger, m_triggerOnce);
            m_collider->setLayer(m_layer);
            m_collider->setCollisionMask(m_collisionMask);
            fitToSprite(owner, m_padding);
        }
    }
    if (m_collider && !m_transformBound) {
        tryBindTransform(owner);
    }
}

void ColliderComponent::update(Entity &owner, double /*dt*/) {
    ensureCollider(owner);
}

void ColliderComponent::tryBindTransform(Entity &owner) {
    auto *transformComponent = owner.getComponent<TransformComponent>();
    if (transformComponent) {
        m_collider->setTransform(&transformComponent->getTransform());
        m_transformBound = true;
    }
}

bool ColliderComponent::fitToSprite(Entity &owner, float padding) {
    if (!m_collider) {
        return false;
    }

    auto *spriteComponent = owner.getComponent<SpriteComponent>();
    if (!spriteComponent) {
        return false;
    }
    auto *sprite = spriteComponent->sprite();
    if (!sprite) {
        return false;
    }

    const glm::vec2 size = sprite->getSize();
    const float pad = padding;

    if (auto *aabbCollider = dynamic_cast<AABBCollider *>(m_collider.get())) {
        if (pad >= 0.0f) {
            glm::vec2 p{pad, pad};
            aabbCollider->setLocalBounds(-p, size + p);
        } else {
            glm::vec2 inset{std::abs(pad), std::abs(pad)};
            glm::vec2 min{inset.x, 0.0f}; // keep feet at y=0
            glm::vec2 max{size.x - inset.x, size.y - inset.y};
            max.x = std::max(max.x, min.x + 1.0f);
            max.y = std::max(max.y, min.y + 1.0f);
            aabbCollider->setLocalBounds(min, max);
        }
        return true;
    }

    if (auto *circleCollider = dynamic_cast<CircleCollider *>(m_collider.get())) {
        const float radius = 0.5f * std::max(size.x, size.y) + pad;
        circleCollider->setRadius(std::max(radius, 1.0f));
        // Place the circle at the sprite's center in local space.
        circleCollider->setLocalOffset(size * 0.5f);
        return true;
    }

    if (auto *capsule = dynamic_cast<CapsuleCollider *>(m_collider.get())) {
        const glm::vec2 usedSize = m_capsuleOverrideActive ? m_capsuleSizeOverride : size;
        const float width = std::max(usedSize.x + padding * 2.0f, 0.0f);
        const float height = std::max(usedSize.y + padding * 2.0f, 0.0f);
        const float radius = std::max(0.5f * std::min(width, height), 1.0f);
        const float halfSegment = std::max(height * 0.5f - radius, 0.0f);
        const glm::vec2 center = size * 0.5f;
        const glm::vec2 offset = m_capsuleOffsetOverrideActive ? m_capsuleOffsetOverride : glm::vec2{0.0f};
        capsule->setRadius(radius);
        capsule->setLocalA({0.0f, -halfSegment});
        capsule->setLocalB({0.0f, halfSegment});
        capsule->setLocalOffset(center + offset);
        return true;
    }

    return false;
}

void ColliderComponent::invokeTriggerEnter(Entity &owner, Entity &other) {
    if (m_isTrigger) {
        if (auto* trigger = owner.getComponent<TriggerComponent>()) {
            trigger->handleEnter(owner, other);
        }
    }
    if (m_onTriggerEnter) {
        m_onTriggerEnter(owner, other);
    }
}

void ColliderComponent::invokeTriggerStay(Entity& owner, Entity& other) {
    if (m_isTrigger) {
        if (auto* trigger = owner.getComponent<TriggerComponent>()) {
            trigger->handleStay(owner, other);
        }
    }
    if (m_onTriggerStay) {
        m_onTriggerStay(owner, other);
    }
}

void ColliderComponent::invokeTriggerExit(Entity &owner, Entity &other) {
    if (m_isTrigger) {
        if (auto* trigger = owner.getComponent<TriggerComponent>()) {
            trigger->handleExit(owner, other);
        }
    }
    if (m_onTriggerExit) {
        m_onTriggerExit(owner, other);
    }
}

void ColliderComponent::setLayer(uint32_t layer) {
    m_layer = std::min<uint32_t>(layer, 31u);
    if (m_collider) {
        m_collider->setLayer(m_layer);
    }
}

void ColliderComponent::setCollisionMask(uint32_t mask) {
    m_collisionMask = mask;
    if (m_collider) {
        m_collider->setCollisionMask(mask);
    }
}

std::unique_ptr<ACollider> ColliderComponent::createCollider(ColliderType type) const {
    switch (type) {
        case ColliderType::AABB:
            return std::make_unique<AABBCollider>(glm::vec2{0.0f}, glm::vec2{0.0f});
        case ColliderType::CIRCLE:
            return std::make_unique<CircleCollider>(0.0f);
        case ColliderType::CAPSULE:
            return std::make_unique<CapsuleCollider>(glm::vec2{}, glm::vec2{}, 0.0f);
        case ColliderType::COMPOSITE:
        default:
            return nullptr;
    }
}

void ColliderComponent::debugDraw(Rendering::Renderer& renderer,
                                  const glm::vec4& color) const {
    if (!m_collider) return;

    constexpr float thickness = 1.0f;

    switch (m_collider->getType()) {
        case ColliderType::AABB: {
            const AABB aabb = m_collider->getAABB();
            Rendering::DebugDraw2D::rectangle(renderer, aabb.getMin(), aabb.getMax(),
                                              thickness, color);
            break;
        }
        case ColliderType::CIRCLE: {
            const auto* circle = dynamic_cast<const CircleCollider*>(m_collider.get());
            if (!circle) break;
            Rendering::DebugDraw2D::circle(renderer, circle->getWorldCenter(),
                                           circle->getWorldRadius(), thickness, color, 24);
            break;
        }
        case ColliderType::CAPSULE: {
            const auto *cap = dynamic_cast<const CapsuleCollider *>(m_collider.get());
            if (!cap) break;
            const glm::vec2 a = cap->getWorldA();
            const glm::vec2 b = cap->getWorldB();
            const float radius = cap->getWorldRadius();
            const glm::vec2 axis = b - a;
            const float axisLength = glm::length(axis);
            if (axisLength <= 1e-6f) {
                Rendering::DebugDraw2D::circle(renderer, a, radius, thickness, color, 24);
                break;
            }
            const glm::vec2 dir = axis / axisLength;
            const glm::vec2 perp = glm::vec2{-dir.y, dir.x};
            const auto drawCap = [&](const glm::vec2& center, float directionSign) {
                constexpr int segments = 16;
                std::array<glm::vec2, segments + 1> points{};
                for (int i = 0; i <= segments; ++i) {
                    const float theta = static_cast<float>(i) /
                                        static_cast<float>(segments) * glm::pi<float>();
                    const glm::vec2 offset = std::cos(theta) * perp +
                                             directionSign * std::sin(theta) * dir;
                    points[static_cast<std::size_t>(i)] = center + offset * radius;
                }
                Rendering::DebugDraw2D::polyline(renderer, points, false,
                                                 thickness, color);
            };
            drawCap(b, +1.0f);
            drawCap(a, -1.0f);
            Rendering::DebugDraw2D::line(renderer, b - perp * radius,
                                         a - perp * radius, thickness, color);
            Rendering::DebugDraw2D::line(renderer, b + perp * radius,
                                         a + perp * radius, thickness, color);
            break;
        }
        default:
            break;
    }
}
void ColliderComponent::setCapsuleSizeOverride(const glm::vec2 &size) {
    m_capsuleSizeOverride = size;
    m_capsuleOverrideActive = size.x > 0.0f && size.y > 0.0f;
}

void ColliderComponent::clearCapsuleSizeOverride() {
    m_capsuleOverrideActive = false;
}

void ColliderComponent::setCapsuleOffsetOverride(const glm::vec2 &offset) {
    m_capsuleOffsetOverride = offset;
    m_capsuleOffsetOverrideActive = true;
}

void ColliderComponent::clearCapsuleOffsetOverride() {
    m_capsuleOffsetOverrideActive = false;
}
