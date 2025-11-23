#include "CollisionDispatcher.hpp"
#include "AABBCollider.hpp"
#include "ACollider.hpp"
#include "CircleCollider.hpp"
#include "CapsuleCollider.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <glm/glm.hpp>

namespace {
glm::vec2 closestPointOnSegment(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &p) {
    const glm::vec2 ab = b - a;
    const float abLen2 = glm::dot(ab, ab);
    if (abLen2 <= 0.000001f) {
        return a;
    }
    const float t = glm::clamp(glm::dot(p - a, ab) / abLen2, 0.0f, 1.0f);
    return a + ab * t;
}

float lengthSafe(const glm::vec2 &v) {
    const float len2 = glm::dot(v, v);
    if (len2 <= 0.000001f) {
        return 0.0f;
    }
    return std::sqrt(len2);
}
} // namespace

namespace {
void handleTriggers(std::unique_ptr<Hit> &hit, const ICollider &a, const ICollider &b) {
    if (!hit) return;

    const auto *ca = dynamic_cast<const ACollider *>(&a);
    const auto *cb = dynamic_cast<const ACollider *>(&b);

    auto triggerConsumed = [](const ACollider *c) {
        return c && c->isTrigger() && c->triggersOnce() && c->hasTriggered();
    };

    if (triggerConsumed(ca) || triggerConsumed(cb)) {
        hit.reset();
        return;
    }

    bool anyTrigger = (ca && ca->isTrigger()) || (cb && cb->isTrigger());
    if (anyTrigger) {
        hit->penetration = 0.0f;
    }

    auto mark = [](const ACollider *c) {
        if (c && c->isTrigger() && c->triggersOnce()) {
            const_cast<ACollider *>(c)->markTriggerFired();
        }
    };
    mark(ca);
    mark(cb);
}
} // namespace

std::unique_ptr<Hit> CollisionDispatcher::dispatch(const ICollider &a, const ICollider &b) {
    const ColliderType typeA = a.getType();
    const ColliderType typeB = b.getType();

    // Symmetric handling: ensure (A,B) ordering is covered.
    if (typeA == ColliderType::AABB && typeB == ColliderType::AABB) {
        const auto *aa = dynamic_cast<const AABBCollider *>(&a);
        const auto *bb = dynamic_cast<const AABBCollider *>(&b);
        if (aa && bb) {
            auto hit = collideAABB_AABB(*aa, *bb);
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CIRCLE && typeB == ColliderType::CIRCLE) {
        const auto *ca = dynamic_cast<const CircleCollider *>(&a);
        const auto *cb = dynamic_cast<const CircleCollider *>(&b);
        if (ca && cb) {
            auto hit = collideCircle_Circle(*ca, *cb);
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CIRCLE && typeB == ColliderType::AABB) {
        const auto *c = dynamic_cast<const CircleCollider *>(&a);
        const auto *bb = dynamic_cast<const AABBCollider *>(&b);
        if (c && bb) {
            auto hit = collideCircle_AABB(*c, *bb);
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }
    if (typeA == ColliderType::AABB && typeB == ColliderType::CIRCLE) {
        const auto *c = dynamic_cast<const CircleCollider *>(&b);
        const auto *bb = dynamic_cast<const AABBCollider *>(&a);
        if (c && bb) {
            auto hit = collideCircle_AABB(*c, *bb);
            // Flip normal to maintain direction from A to B
            if (hit) {
                hit->normal = -hit->normal;
            }
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CAPSULE && typeB == ColliderType::CIRCLE) {
        const auto *cap = dynamic_cast<const CapsuleCollider *>(&a);
        const auto *c = dynamic_cast<const CircleCollider *>(&b);
        if (cap && c) {
            auto hit = collideCapsule_Circle(*cap, *c);
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }
    if (typeA == ColliderType::CIRCLE && typeB == ColliderType::CAPSULE) {
        const auto *cap = dynamic_cast<const CapsuleCollider *>(&b);
        const auto *c = dynamic_cast<const CircleCollider *>(&a);
        if (cap && c) {
            auto hit = collideCapsule_Circle(*cap, *c);
            if (hit) {
                hit->normal = -hit->normal;
            }
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CAPSULE && typeB == ColliderType::AABB) {
        const auto *cap = dynamic_cast<const CapsuleCollider *>(&a);
        const auto *bb = dynamic_cast<const AABBCollider *>(&b);
        if (cap && bb) {
            auto hit = collideCapsule_AABB(*cap, *bb);
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }
    if (typeA == ColliderType::AABB && typeB == ColliderType::CAPSULE) {
        const auto *cap = dynamic_cast<const CapsuleCollider *>(&b);
        const auto *bb = dynamic_cast<const AABBCollider *>(&a);
        if (cap && bb) {
            auto hit = collideCapsule_AABB(*cap, *bb);
            if (hit) {
                hit->normal = -hit->normal;
            }
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CAPSULE && typeB == ColliderType::CAPSULE) {
        const auto *capA = dynamic_cast<const CapsuleCollider *>(&a);
        const auto *capB = dynamic_cast<const CapsuleCollider *>(&b);
        if (capA && capB) {
            auto hit = collideCapsule_Capsule(*capA, *capB);
            handleTriggers(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    return nullptr;
}

std::unique_ptr<Hit> CollisionDispatcher::collideAABB_AABB(const AABBCollider & a, const AABBCollider & b) {
    const AABB aBox = a.getAABB();
    const AABB bBox = b.getAABB();

    const glm::vec2 aMin = aBox.getMin();
    const glm::vec2 aMax = aBox.getMax();
    const glm::vec2 bMin = bBox.getMin();
    const glm::vec2 bMax = bBox.getMax();

    const float overlapX = std::min(aMax.x, bMax.x) - std::max(aMin.x, bMin.x);
    const float overlapY = std::min(aMax.y, bMax.y) - std::max(aMin.y, bMin.y);

    if (overlapX <= 0.0f || overlapY <= 0.0f) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    if (overlapX < overlapY) {
        hit->penetration = overlapX;
        hit->normal = (aBox.center().x < bBox.center().x) ? glm::vec2{-1.0f, 0.0f} : glm::vec2{1.0f, 0.0f};
        hit->contactPoint = glm::vec2{(aBox.center().x + bBox.center().x) * 0.5f,
                                     std::clamp(aBox.center().y, bMin.y, bMax.y)};
    } else {
        hit->penetration = overlapY;
        hit->normal = (aBox.center().y < bBox.center().y) ? glm::vec2{0.0f, -1.0f} : glm::vec2{0.0f, 1.0f};
        hit->contactPoint = glm::vec2{std::clamp(aBox.center().x, bMin.x, bMax.x),
                                     (aBox.center().y + bBox.center().y) * 0.5f};
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCircle_Circle(const CircleCollider & a, const CircleCollider & b) {
    const AABB aBox = a.getAABB();
    const AABB bBox = b.getAABB();
    const glm::vec2 centerA = aBox.center();
    const glm::vec2 centerB = bBox.center();

    const float radiusA = 0.5f * std::min(aBox.width(), aBox.height());
    const float radiusB = 0.5f * std::min(bBox.width(), bBox.height());

    const glm::vec2 delta = centerB - centerA;
    const float dist = lengthSafe(delta);
    const float sumR = radiusA + radiusB;
    if (dist >= sumR) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = sumR - dist;
    if (dist > 0.000001f) {
        hit->normal = delta / dist;
        hit->contactPoint = centerA + hit->normal * (radiusA - hit->penetration * 0.5f);
    } else {
        hit->normal = {1.0f, 0.0f};
        hit->contactPoint = centerA;
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCircle_AABB(const CircleCollider & c, const AABBCollider & b) {
    const AABB circleBox = c.getAABB();
    const AABB box = b.getAABB();
    const glm::vec2 circleCenter = circleBox.center();
    const float radius = 0.5f * std::min(circleBox.width(), circleBox.height());

    const glm::vec2 closest = glm::clamp(circleCenter, box.getMin(), box.getMax());
    const glm::vec2 diff = circleCenter - closest;
    const float dist = lengthSafe(diff);

    if (dist >= radius) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = radius - dist;
    if (dist > 0.000001f) {
        hit->normal = diff / dist;
        hit->contactPoint = closest;
    } else {
        // Circle center is inside box; choose axis of minimal penetration.
        const float left = circleCenter.x - box.getMin().x;
        const float right = box.getMax().x - circleCenter.x;
        const float down = circleCenter.y - box.getMin().y;
        const float up = box.getMax().y - circleCenter.y;
        const float minPen = std::min({left, right, down, up});
        if (minPen == left) hit->normal = {-1.0f, 0.0f};
        else if (minPen == right) hit->normal = {1.0f, 0.0f};
        else if (minPen == down) hit->normal = {0.0f, -1.0f};
        else hit->normal = {0.0f, 1.0f};
        hit->contactPoint = circleCenter;
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCapsule_Circle(const CapsuleCollider & cap, const CircleCollider & c) {
    const glm::vec2 a = cap.getWorldA();
    const glm::vec2 bPt = cap.getWorldB();

    const AABB circleBox = c.getAABB();
    const glm::vec2 circleCenter = circleBox.center();
    const float radiusC = 0.5f * std::min(circleBox.width(), circleBox.height());

    const glm::vec2 closest = closestPointOnSegment(a, bPt, circleCenter);
    const glm::vec2 delta = circleCenter - closest;
    const float dist = lengthSafe(delta);

    // Estimate scaled capsule radius from its AABB.
    const float capsuleRadius = 0.5f * std::min(cap.getAABB().width(), cap.getAABB().height());
    const float sumR = capsuleRadius + radiusC;

    if (dist >= sumR) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = sumR - dist;
    if (dist > 0.000001f) {
        hit->normal = delta / dist;
        hit->contactPoint = closest + hit->normal * capsuleRadius;
    } else {
        hit->normal = {1.0f, 0.0f};
        hit->contactPoint = closest;
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCapsule_AABB(const CapsuleCollider & cap, const AABBCollider & box) {
    // Treat capsule as a segment with radius; find closest point between segment and AABB.
    const glm::vec2 a = cap.getWorldA();
    const glm::vec2 bPt = cap.getWorldB();
    const AABB bBox = box.getAABB();
    const glm::vec2 minB = bBox.getMin();
    const glm::vec2 maxB = bBox.getMax();

    const glm::vec2 boxCenter = bBox.center();
    const glm::vec2 closestOnSeg = closestPointOnSegment(a, bPt, boxCenter);
    const glm::vec2 closestOnBox = glm::clamp(closestOnSeg, minB, maxB);

    const glm::vec2 delta = closestOnSeg - closestOnBox;
    const float dist = lengthSafe(delta);
    const float capRadius = 0.5f * std::min(cap.getAABB().width(), cap.getAABB().height());

    if (dist >= capRadius) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = capRadius - dist;
    if (dist > 0.000001f) {
        hit->normal = delta / dist;
        hit->contactPoint = closestOnBox;
    } else {
        // Segment inside box; pick axis with minimal expansion.
        const float left = closestOnSeg.x - minB.x;
        const float right = maxB.x - closestOnSeg.x;
        const float down = closestOnSeg.y - minB.y;
        const float up = maxB.y - closestOnSeg.y;
        const float minPen = std::min({left, right, down, up});
        if (minPen == left) hit->normal = {-1.0f, 0.0f};
        else if (minPen == right) hit->normal = {1.0f, 0.0f};
        else if (minPen == down) hit->normal = {0.0f, -1.0f};
        else hit->normal = {0.0f, 1.0f};
        hit->contactPoint = closestOnSeg;
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCapsule_Capsule(const CapsuleCollider & capA, const CapsuleCollider & capB) {
    const glm::vec2 a0 = capA.getWorldA();
    const glm::vec2 a1 = capA.getWorldB();
    const glm::vec2 b0 = capB.getWorldA();
    const glm::vec2 b1 = capB.getWorldB();

    const glm::vec2 da = a1 - a0;
    const glm::vec2 db = b1 - b0;
    const glm::vec2 r = a0 - b0;
    const float daDotDa = glm::dot(da, da);
    const float dbDotDb = glm::dot(db, db);
    const float daDotDb = glm::dot(da, db);
    const float denom = daDotDa * dbDotDb - daDotDb * daDotDb;

    float s = 0.0f;
    float t = 0.0f;
    if (denom != 0.0f) {
        s = (daDotDb * glm::dot(db, r) - dbDotDb * glm::dot(da, r)) / denom;
        s = glm::clamp(s, 0.0f, 1.0f);
    }
    t = (s * daDotDb + glm::dot(db, r)) / dbDotDb;
    t = glm::clamp(t, 0.0f, 1.0f);
    if (denom != 0.0f) {
        s = (daDotDb * t - glm::dot(da, r)) / daDotDa;
        s = glm::clamp(s, 0.0f, 1.0f);
    }

    const glm::vec2 closestA = a0 + da * s;
    const glm::vec2 closestB = b0 + db * t;
    const glm::vec2 delta = closestB - closestA;
    const float dist = lengthSafe(delta);

    const float radiusA = 0.5f * std::min(capA.getAABB().width(), capA.getAABB().height());
    const float radiusB = 0.5f * std::min(capB.getAABB().width(), capB.getAABB().height());
    const float sumR = radiusA + radiusB;

    if (dist >= sumR) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = sumR - dist;
    if (dist > 0.000001f) {
        hit->normal = delta / dist;
        hit->contactPoint = closestA + hit->normal * radiusA;
    } else {
        hit->normal = {1.0f, 0.0f};
        hit->contactPoint = closestA;
    }
    return hit;
}
