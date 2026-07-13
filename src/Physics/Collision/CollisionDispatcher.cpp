#include "CollisionDispatcher.hpp"
#include "AABBCollider.hpp"
#include "ACollider.hpp"
#include "CircleCollider.hpp"
#include "CapsuleCollider.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <functional>
#include <limits>
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

glm::vec2 toBoxLocal(const OrientedBounds2D& box,
                     const glm::vec2& worldPoint) {
    const glm::vec2 offset = worldPoint - box.center;
    return {glm::dot(offset, box.axisX), glm::dot(offset, box.axisY)};
}

glm::vec2 toBoxWorld(const OrientedBounds2D& box,
                     const glm::vec2& localPoint) {
    return box.center + box.axisX * localPoint.x + box.axisY * localPoint.y;
}

glm::vec2 boxContactPoint(const OrientedBounds2D& box,
                          const glm::vec2& direction) {
    constexpr float epsilon = 1e-6f;
    const auto coordinate = [&](const glm::vec2& axis, float extent) {
        const float projection = glm::dot(direction, axis);
        if (std::abs(projection) <= epsilon) {
            return 0.0f;
        }
        return projection > 0.0f ? extent : -extent;
    };
    const float x = coordinate(box.axisX, box.halfExtents.x);
    const float y = coordinate(box.axisY, box.halfExtents.y);
    return toBoxWorld(box, {x, y});
}

glm::vec2 canonicalAxis(glm::vec2 axis) {
    if (axis.x < 0.0f || (std::abs(axis.x) <= 1e-6f && axis.y < 0.0f)) {
        axis = -axis;
    }
    return axis;
}

struct SegmentPair {
    glm::vec2 a{0.0f};
    glm::vec2 b{0.0f};
};

struct SegmentBoxPair {
    glm::vec2 segment{0.0f};
    glm::vec2 box{0.0f};
    float distanceSquared{std::numeric_limits<float>::max()};
};

SegmentPair closestPointsOnSegments(const glm::vec2& p0, const glm::vec2& p1,
                                    const glm::vec2& q0, const glm::vec2& q1) {
    constexpr float epsilon = 1e-6f;
    const glm::vec2 d1 = p1 - p0;
    const glm::vec2 d2 = q1 - q0;
    const glm::vec2 r = p0 - q0;
    const float a = glm::dot(d1, d1);
    const float e = glm::dot(d2, d2);
    const float f = glm::dot(d2, r);
    float s = 0.0f;
    float t = 0.0f;

    if (a <= epsilon && e <= epsilon) {
        return {p0, q0};
    }
    if (a <= epsilon) {
        t = glm::clamp(f / e, 0.0f, 1.0f);
    } else {
        const float c = glm::dot(d1, r);
        if (e <= epsilon) {
            s = glm::clamp(-c / a, 0.0f, 1.0f);
        } else {
            const float b = glm::dot(d1, d2);
            const float denominator = a * e - b * b;
            if (std::abs(denominator) > epsilon) {
                s = glm::clamp((b * f - c * e) / denominator, 0.0f, 1.0f);
            }
            t = glm::clamp((b * s + f) / e, 0.0f, 1.0f);
            if (t <= 0.0f) {
                s = glm::clamp(-c / a, 0.0f, 1.0f);
            } else if (t >= 1.0f) {
                s = glm::clamp((b - c) / a, 0.0f, 1.0f);
            }
        }
    }
    return {p0 + d1 * s, q0 + d2 * t};
}

SegmentBoxPair closestPointsSegmentBox(const glm::vec2& a,
                                       const glm::vec2& b,
                                       const glm::vec2& boxMin,
                                       const glm::vec2& boxMax) {
    SegmentBoxPair result{};
    const auto consider = [&](const glm::vec2& segmentPoint,
                              const glm::vec2& boxPoint) {
        const glm::vec2 delta = segmentPoint - boxPoint;
        const float distanceSquared = glm::dot(delta, delta);
        if (distanceSquared < result.distanceSquared) {
            result = {segmentPoint, boxPoint, distanceSquared};
        }
    };

    consider(a, glm::clamp(a, boxMin, boxMax));
    consider(b, glm::clamp(b, boxMin, boxMax));

    const glm::vec2 bottomLeft{boxMin.x, boxMin.y};
    const glm::vec2 bottomRight{boxMax.x, boxMin.y};
    const glm::vec2 topRight{boxMax.x, boxMax.y};
    const glm::vec2 topLeft{boxMin.x, boxMax.y};
    const glm::vec2 edgeStarts[]{bottomLeft, bottomRight, topRight, topLeft};
    const glm::vec2 edgeEnds[]{bottomRight, topRight, topLeft, bottomLeft};
    for (std::size_t i = 0; i < 4; ++i) {
        const SegmentPair pair = closestPointsOnSegments(
            a, b, edgeStarts[i], edgeEnds[i]);
        consider(pair.a, pair.b);
    }
    return result;
}
} // namespace

namespace {
void applyTriggerSemantics(std::unique_ptr<Hit> &hit, const ICollider &a,
                           const ICollider &b) {
    if (!hit) return;

    const auto *ca = dynamic_cast<const ACollider *>(&a);
    const auto *cb = dynamic_cast<const ACollider *>(&b);

    const bool anyTrigger = (ca && ca->isTrigger()) || (cb && cb->isTrigger());
    if (anyTrigger) {
        hit->penetration = 0.0f;
    }
}
} // namespace

std::unique_ptr<Hit> CollisionDispatcher::dispatch(const ICollider &a, const ICollider &b) {
    const ColliderType typeA = a.getType();
    const ColliderType typeB = b.getType();

    const auto *caBase = dynamic_cast<const ACollider *>(&a);
    const auto *cbBase = dynamic_cast<const ACollider *>(&b);
    if (caBase && cbBase) {
        if (!caBase->allowsCollisionWith(*cbBase) || !cbBase->allowsCollisionWith(*caBase)) {
            return nullptr;
        }
    }

    // Symmetric handling: ensure (A,B) ordering is covered.
    if (typeA == ColliderType::AABB && typeB == ColliderType::AABB) {
        const auto *aa = dynamic_cast<const AABBCollider *>(&a);
        const auto *bb = dynamic_cast<const AABBCollider *>(&b);
        if (aa && bb) {
            auto hit = collideAABB_AABB(*aa, *bb);
            applyTriggerSemantics(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CIRCLE && typeB == ColliderType::CIRCLE) {
        const auto *ca = dynamic_cast<const CircleCollider *>(&a);
        const auto *cb = dynamic_cast<const CircleCollider *>(&b);
        if (ca && cb) {
            auto hit = collideCircle_Circle(*ca, *cb);
            applyTriggerSemantics(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CIRCLE && typeB == ColliderType::AABB) {
        const auto *c = dynamic_cast<const CircleCollider *>(&a);
        const auto *bb = dynamic_cast<const AABBCollider *>(&b);
        if (c && bb) {
            auto hit = collideCircle_AABB(*c, *bb);
            applyTriggerSemantics(hit, a, b);
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
            applyTriggerSemantics(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CAPSULE && typeB == ColliderType::CIRCLE) {
        const auto *cap = dynamic_cast<const CapsuleCollider *>(&a);
        const auto *c = dynamic_cast<const CircleCollider *>(&b);
        if (cap && c) {
            auto hit = collideCapsule_Circle(*cap, *c);
            applyTriggerSemantics(hit, a, b);
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
            applyTriggerSemantics(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CAPSULE && typeB == ColliderType::AABB) {
        const auto *cap = dynamic_cast<const CapsuleCollider *>(&a);
        const auto *bb = dynamic_cast<const AABBCollider *>(&b);
        if (cap && bb) {
            auto hit = collideCapsule_AABB(*cap, *bb);
            applyTriggerSemantics(hit, a, b);
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
            applyTriggerSemantics(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    if (typeA == ColliderType::CAPSULE && typeB == ColliderType::CAPSULE) {
        const auto *capA = dynamic_cast<const CapsuleCollider *>(&a);
        const auto *capB = dynamic_cast<const CapsuleCollider *>(&b);
        if (capA && capB) {
            auto hit = collideCapsule_Capsule(*capA, *capB);
            applyTriggerSemantics(hit, a, b);
            return hit;
        }
        return nullptr;
    }

    return nullptr;
}

std::unique_ptr<Hit> CollisionDispatcher::collideAABB_AABB(const AABBCollider & a, const AABBCollider & b) {
    const OrientedBounds2D aBox = a.getOrientedBounds();
    const OrientedBounds2D bBox = b.getOrientedBounds();
    const glm::vec2 centerDelta = aBox.center - bBox.center;
    const glm::vec2 axes[]{aBox.axisX, aBox.axisY, bBox.axisX, bBox.axisY};
    float minimumPenetration = std::numeric_limits<float>::max();
    glm::vec2 minimumAxis{1.0f, 0.0f};

    for (const glm::vec2& axis : axes) {
        const glm::vec2 candidateAxis = canonicalAxis(axis);
        const float radiusA =
            aBox.halfExtents.x * std::abs(glm::dot(candidateAxis, aBox.axisX)) +
            aBox.halfExtents.y * std::abs(glm::dot(candidateAxis, aBox.axisY));
        const float radiusB =
            bBox.halfExtents.x * std::abs(glm::dot(candidateAxis, bBox.axisX)) +
            bBox.halfExtents.y * std::abs(glm::dot(candidateAxis, bBox.axisY));
        const float penetration = radiusA + radiusB -
                                  std::abs(glm::dot(centerDelta, candidateAxis));
        if (penetration <= 0.0f) {
            return nullptr;
        }
        const bool shallower = penetration < minimumPenetration - 1e-6f;
        const bool equalButCanonical =
            std::abs(penetration - minimumPenetration) <= 1e-6f &&
            (candidateAxis.x < minimumAxis.x - 1e-6f ||
             (std::abs(candidateAxis.x - minimumAxis.x) <= 1e-6f &&
              candidateAxis.y < minimumAxis.y));
        if (shallower || equalButCanonical) {
            minimumPenetration = penetration;
            minimumAxis = candidateAxis;
        }
    }

    const float centerProjection = glm::dot(centerDelta, minimumAxis);
    if (centerProjection < -1e-6f ||
        (std::abs(centerProjection) <= 1e-6f &&
         std::less<const AABBCollider*>{}(&a, &b))) {
        minimumAxis = -minimumAxis;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = minimumPenetration;
    hit->normal = minimumAxis;
    const glm::vec2 pointA = boxContactPoint(aBox, -minimumAxis);
    const glm::vec2 pointB = boxContactPoint(bBox, minimumAxis);
    hit->contactPoint = (pointA + pointB) * 0.5f;
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCircle_Circle(const CircleCollider & a, const CircleCollider & b) {
    const AABB aBox = a.getAABB();
    const AABB bBox = b.getAABB();
    const glm::vec2 centerA = aBox.center();
    const glm::vec2 centerB = bBox.center();

    const float radiusA = a.getWorldRadius();
    const float radiusB = b.getWorldRadius();

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
        hit->normal = -delta / dist;
        hit->contactPoint = centerA - hit->normal *
            (radiusA - hit->penetration * 0.5f);
    } else {
        hit->normal = {1.0f, 0.0f};
        hit->contactPoint = centerA;
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCircle_AABB(const CircleCollider & c, const AABBCollider & b) {
    const glm::vec2 circleCenter = c.getWorldCenter();
    const float radius = c.getWorldRadius();
    const OrientedBounds2D box = b.getOrientedBounds();
    const glm::vec2 localCenter = toBoxLocal(box, circleCenter);
    const glm::vec2 localMin = -box.halfExtents;
    const glm::vec2 localMax = box.halfExtents;

    const glm::vec2 closest = glm::clamp(localCenter, localMin, localMax);
    const glm::vec2 diff = localCenter - closest;
    const float dist = lengthSafe(diff);

    if (dist >= radius) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = radius - dist;
    if (dist > 0.000001f) {
        const glm::vec2 localNormal = diff / dist;
        hit->normal = box.axisX * localNormal.x + box.axisY * localNormal.y;
        hit->contactPoint = toBoxWorld(box, closest);
    } else {
        // Circle center is inside box; choose axis of minimal penetration.
        const float left = localCenter.x - localMin.x;
        const float right = localMax.x - localCenter.x;
        const float down = localCenter.y - localMin.y;
        const float up = localMax.y - localCenter.y;
        const float minPen = std::min({left, right, down, up});
        glm::vec2 localNormal{0.0f, 1.0f};
        if (minPen == left) localNormal = {-1.0f, 0.0f};
        else if (minPen == right) localNormal = {1.0f, 0.0f};
        else if (minPen == down) localNormal = {0.0f, -1.0f};
        hit->normal = box.axisX * localNormal.x + box.axisY * localNormal.y;
        hit->penetration = radius + minPen;
        glm::vec2 localContact = localCenter;
        if (minPen == left) localContact.x = localMin.x;
        else if (minPen == right) localContact.x = localMax.x;
        else if (minPen == down) localContact.y = localMin.y;
        else localContact.y = localMax.y;
        hit->contactPoint = toBoxWorld(box, localContact);
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCapsule_Circle(const CapsuleCollider & cap, const CircleCollider & c) {
    const glm::vec2 a = cap.getWorldA();
    const glm::vec2 bPt = cap.getWorldB();

    const AABB circleBox = c.getAABB();
    const glm::vec2 circleCenter = circleBox.center();
    const float radiusC = c.getWorldRadius();

    const glm::vec2 closest = closestPointOnSegment(a, bPt, circleCenter);
    const glm::vec2 delta = circleCenter - closest;
    const float dist = lengthSafe(delta);

    const float capsuleRadius = cap.getWorldRadius();
    const float sumR = capsuleRadius + radiusC;

    if (dist >= sumR) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = sumR - dist;
    if (dist > 0.000001f) {
        hit->normal = -delta / dist;
        hit->contactPoint = closest - hit->normal * capsuleRadius;
    } else {
        hit->normal = {1.0f, 0.0f};
        hit->contactPoint = closest;
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCapsule_AABB(const CapsuleCollider & cap, const AABBCollider & box) {
    const OrientedBounds2D orientedBox = box.getOrientedBounds();
    const glm::vec2 a = toBoxLocal(orientedBox, cap.getWorldA());
    const glm::vec2 bPt = toBoxLocal(orientedBox, cap.getWorldB());
    const glm::vec2 minB = -orientedBox.halfExtents;
    const glm::vec2 maxB = orientedBox.halfExtents;
    const SegmentBoxPair closest = closestPointsSegmentBox(a, bPt, minB, maxB);
    const glm::vec2 delta = closest.segment - closest.box;
    const float dist = std::sqrt(std::max(closest.distanceSquared, 0.0f));
    const float capRadius = cap.getWorldRadius();

    if (dist >= capRadius) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = capRadius - dist;
    if (dist > 0.000001f) {
        const glm::vec2 localNormal = delta / dist;
        hit->normal = orientedBox.axisX * localNormal.x +
                      orientedBox.axisY * localNormal.y;
        hit->contactPoint = toBoxWorld(orientedBox, closest.box);
    } else {
        // The center segment intersects the box. Compare the complete capsule
        // projection on each box axis so containment cannot under-report the
        // translation required to separate the shapes.
        const glm::vec2 capsuleMin{
            std::min(a.x, bPt.x) - capRadius,
            std::min(a.y, bPt.y) - capRadius};
        const glm::vec2 capsuleMax{
            std::max(a.x, bPt.x) + capRadius,
            std::max(a.y, bPt.y) + capRadius};
        const float left = capsuleMax.x - minB.x;
        const float right = maxB.x - capsuleMin.x;
        const float down = capsuleMax.y - minB.y;
        const float up = maxB.y - capsuleMin.y;
        const float minPen = std::min({left, right, down, up});
        glm::vec2 localNormal{0.0f, 1.0f};
        if (minPen == left) localNormal = {-1.0f, 0.0f};
        else if (minPen == right) localNormal = {1.0f, 0.0f};
        else if (minPen == down) localNormal = {0.0f, -1.0f};
        hit->normal = orientedBox.axisX * localNormal.x +
                      orientedBox.axisY * localNormal.y;
        hit->penetration = minPen;
        const glm::vec2 contactBase = glm::clamp(closest.segment, minB, maxB);
        glm::vec2 localContact{};
        if (minPen == left) localContact = {minB.x, contactBase.y};
        else if (minPen == right) localContact = {maxB.x, contactBase.y};
        else if (minPen == down) localContact = {contactBase.x, minB.y};
        else localContact = {contactBase.x, maxB.y};
        hit->contactPoint = toBoxWorld(orientedBox, localContact);
    }
    return hit;
}

std::unique_ptr<Hit> CollisionDispatcher::collideCapsule_Capsule(const CapsuleCollider & capA, const CapsuleCollider & capB) {
    const glm::vec2 a0 = capA.getWorldA();
    const glm::vec2 a1 = capA.getWorldB();
    const glm::vec2 b0 = capB.getWorldA();
    const glm::vec2 b1 = capB.getWorldB();

    const SegmentPair closest = closestPointsOnSegments(a0, a1, b0, b1);
    const glm::vec2 closestA = closest.a;
    const glm::vec2 closestB = closest.b;
    const glm::vec2 delta = closestB - closestA;
    const float dist = lengthSafe(delta);

    const float radiusA = capA.getWorldRadius();
    const float radiusB = capB.getWorldRadius();
    const float sumR = radiusA + radiusB;

    if (dist >= sumR) {
        return nullptr;
    }

    auto hit = std::make_unique<Hit>();
    hit->collided = true;
    hit->penetration = sumR - dist;
    if (dist > 0.000001f) {
        hit->normal = -delta / dist;
        hit->contactPoint = closestA - hit->normal * radiusA;
    } else {
        hit->normal = {1.0f, 0.0f};
        hit->contactPoint = closestA;
    }
    return hit;
}
