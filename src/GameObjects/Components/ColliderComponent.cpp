//
// ColliderComponent.cpp
//

#include "ColliderComponent.hpp"

#include <GL/glew.h>
#include <algorithm>

#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/CapsuleCollider.hpp"

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
        const glm::vec2 halfHeight = glm::vec2{0.0f, usedSize.y * 0.5f};
        const glm::vec2 center = size * 0.5f;
        const glm::vec2 offset = m_capsuleOffsetOverrideActive ? m_capsuleOffsetOverride : glm::vec2{0.0f};
        capsule->setRadius(std::max(0.5f * usedSize.x, 1.0f));
        capsule->setLocalA(-halfHeight);
        capsule->setLocalB(halfHeight);
        capsule->setLocalOffset(center + offset);
        return true;
    }

    return false;
}

void ColliderComponent::invokeTriggerEnter(Entity &owner, Entity &other) {
    if (m_onTriggerEnter) {
        m_onTriggerEnter(owner, other);
    }
}

void ColliderComponent::invokeTriggerExit(Entity &owner, Entity &other) {
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

void ColliderComponent::debugDraw(const glm::mat4 &viewProj, const glm::vec3 &color) const {
    if (!m_collider) return;

    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&viewProj[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(color.r, color.g, color.b);
    glLineWidth(1.0f);

    switch (m_collider->getType()) {
        case ColliderType::AABB: {
            const AABB aabb = m_collider->getAABB();
            const glm::vec2 min = aabb.getMin();
            const glm::vec2 max = aabb.getMax();
            glBegin(GL_LINE_LOOP);
            glVertex2f(min.x, min.y);
            glVertex2f(max.x, min.y);
            glVertex2f(max.x, max.y);
            glVertex2f(min.x, max.y);
            glEnd();
            break;
        }
        case ColliderType::CIRCLE: {
            const AABB aabb = m_collider->getAABB();
            const glm::vec2 center = aabb.center();
            const float radius = 0.5f * std::min(aabb.width(), aabb.height());
            const int segments = 24;
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < segments; ++i) {
                const float theta = (static_cast<float>(i) / segments) * 2.0f * 3.1415926f;
                glVertex2f(center.x + std::cos(theta) * radius, center.y + std::sin(theta) * radius);
            }
            glEnd();
            break;
        }
        case ColliderType::CAPSULE: {
            auto *cap = dynamic_cast<CapsuleCollider *>(m_collider.get());
            if (!cap) break;
            const glm::vec2 a = cap->getWorldA();
            const glm::vec2 b = cap->getWorldB();
            const float radius = 0.5f * std::min(cap->getAABB().width(), cap->getAABB().height());
            // draw segment
            glBegin(GL_LINES);
            glVertex2f(a.x, a.y);
            glVertex2f(b.x, b.y);
            glEnd();
            const glm::vec2 dir = glm::normalize(b - a);
            const glm::vec2 perp = glm::vec2{-dir.y, dir.x};
            const auto drawCap = [&](const glm::vec2 &center, float verticalSign) {
                const int segments = 24;
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i <= segments; ++i) {
                    const float theta = (static_cast<float>(i) / segments) * glm::pi<float>();
                    const glm::vec2 circle =
                            std::cos(theta) * perp + verticalSign * std::sin(theta) * dir;
                    glVertex2f(center.x + circle.x * radius, center.y + circle.y * radius);
                }
                glEnd();
            };
            drawCap(b, +1.0f);
            drawCap(a, -1.0f);
            glBegin(GL_LINES);
            const glm::vec2 topRight = b + perp * radius;
            const glm::vec2 topLeft = b - perp * radius;
            const glm::vec2 bottomRight = a + perp * radius;
            const glm::vec2 bottomLeft = a - perp * radius;
            glVertex2f(topLeft.x, topLeft.y);
            glVertex2f(bottomLeft.x, bottomLeft.y);
            glVertex2f(topRight.x, topRight.y);
            glVertex2f(bottomRight.x, bottomRight.y);
            glEnd();
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
