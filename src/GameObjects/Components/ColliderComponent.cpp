//
// ColliderComponent.cpp
//

#include "ColliderComponent.hpp"

#include <GL/glew.h>

#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/CapsuleCollider.hpp"

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
    }
    m_transformBound = false;
}

void ColliderComponent::ensureCollider(Entity &owner) {
    if (!m_collider) {
        m_collider = createCollider(m_requestedType);
        m_transformBound = false;
        if (m_collider) {
            m_collider->setTrigger(m_isTrigger, m_triggerOnce);
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
            // draw caps
            const int segments = 24;
            const glm::vec2 dirNorm = glm::normalize(b - a + glm::vec2(0.0001f));
            const glm::vec2 normal{-dirNorm.y, dirNorm.x};
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= segments; ++i) {
                const float theta = (static_cast<float>(i) / segments) * 3.1415926f;
                const glm::vec2 p = a + normal * std::cos(theta) * radius + dirNorm * std::sin(theta) * radius;
                glVertex2f(p.x, p.y);
            }
            glEnd();
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= segments; ++i) {
                const float theta = (static_cast<float>(i) / segments) * 3.1415926f;
                const glm::vec2 p = b - normal * std::cos(theta) * radius + dirNorm * std::sin(theta) * radius;
                glVertex2f(p.x, p.y);
            }
            glEnd();
            break;
        }
        default:
            break;
    }
}
