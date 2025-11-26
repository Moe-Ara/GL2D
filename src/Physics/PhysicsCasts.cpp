#include "PhysicsCasts.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/AABB.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/ACollider.hpp"
#include "Physics/Collision/CapsuleCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"
#include "Utils/Transform.hpp"
#include <glm/glm.hpp>

namespace {
constexpr float kEpsilon = 1e-5f;
constexpr float kDefaultCastDistance = 1e6f;

struct ColliderEntry {
    Entity* entity{nullptr};
    ACollider* collider{nullptr};
};

struct RayHitData {
    float t{0.0f};
    glm::vec2 point{0.0f};
    glm::vec2 normal{0.0f};
};

bool shouldSkip(const ColliderEntry& entry, const PhysicsCasts::CastFilter& filter) {
    if (!entry.collider) return true;
    if (filter.ignore && entry.entity == filter.ignore) return true;
    if (!filter.includeTriggers && entry.collider->isTrigger()) return true;
    const uint32_t bit = 1u << entry.collider->getLayer();
    if ((filter.layerMask & bit) == 0u) return true;
    return false;
}

std::vector<ColliderEntry> gatherColliders(const std::vector<std::unique_ptr<Entity>>& entities) {
    std::vector<ColliderEntry> out;
    out.reserve(entities.size());
    for (const auto& ePtr : entities) {
        if (!ePtr) continue;
        auto* colliderComp = ePtr->getComponent<ColliderComponent>();
        if (!colliderComp) continue;
        colliderComp->ensureCollider(*ePtr);
        auto* collider = colliderComp->collider();
        if (!collider) continue;
        out.push_back(ColliderEntry{ePtr.get(), collider});
    }
    return out;
}

glm::vec2 safeNormal(const glm::vec2& n, const glm::vec2& fallback) {
    const float len2 = glm::dot(n, n);
    if (len2 <= kEpsilon * kEpsilon) {
        return fallback;
    }
    return n / std::sqrt(len2);
}

bool rayVsAabb(const glm::vec2& origin,
               const glm::vec2& dir,
               const AABB& box,
               float maxDistance,
               RayHitData& out) {
    const glm::vec2 bMin = box.getMin();
    const glm::vec2 bMax = box.getMax();

    float tMin = 0.0f;
    float tMax = maxDistance;
    glm::vec2 normal{0.0f};

    for (int axis = 0; axis < 2; ++axis) {
        const float d = axis == 0 ? dir.x : dir.y;
        const float o = axis == 0 ? origin.x : origin.y;
        const float minVal = axis == 0 ? bMin.x : bMin.y;
        const float maxVal = axis == 0 ? bMax.x : bMax.y;

        if (std::abs(d) < kEpsilon) {
            if (o < minVal || o > maxVal) {
                return false;
            }
            continue;
        }

        const float invD = 1.0f / d;
        float t1 = (minVal - o) * invD;
        float t2 = (maxVal - o) * invD;
        if (t1 > t2) std::swap(t1, t2);

        if (t1 > tMin) {
            tMin = t1;
            normal = glm::vec2{0.0f};
            normal[axis] = (d > 0.0f) ? -1.0f : 1.0f;
        }
        tMax = std::min(tMax, t2);
        if (tMax < tMin) {
            return false;
        }
    }

    if (tMin < 0.0f || tMin > maxDistance) {
        return false;
    }

    out.t = tMin;
    out.point = origin + dir * tMin;
    out.normal = normal;
    return true;
}

bool rayVsCircle(const glm::vec2& origin,
                 const glm::vec2& dir,
                 const glm::vec2& center,
                 float radius,
                 float maxDistance,
                 RayHitData& out) {
    const glm::vec2 m = origin - center;
    const float b = glm::dot(m, dir);
    const float c = glm::dot(m, m) - radius * radius;

    if (c <= 0.0f) {
        out.t = 0.0f;
        out.point = origin;
        out.normal = safeNormal(origin - center, -dir);
        return true; // origin starts inside the circle.
    }

    if (b > 0.0f) {
        return false; // pointing away
    }

    const float disc = b * b - c;
    if (disc < 0.0f) {
        return false;
    }
    float t = -b - std::sqrt(disc);
    if (t < 0.0f || t > maxDistance) {
        return false;
    }

    out.t = t;
    out.point = origin + dir * t;
    out.normal = safeNormal(out.point - center, -dir);
    return true;
}

struct SegmentClosest {
    float s{0.0f};
    float t{0.0f};
    glm::vec2 p{0.0f};
    glm::vec2 q{0.0f};
};

SegmentClosest closestPointsOnSegments(const glm::vec2& p0,
                                       const glm::vec2& p1,
                                       const glm::vec2& q0,
                                       const glm::vec2& q1) {
    const glm::vec2 d1 = p1 - p0;
    const glm::vec2 d2 = q1 - q0;
    const glm::vec2 r = p0 - q0;

    const float a = glm::dot(d1, d1);
    const float e = glm::dot(d2, d2);
    const float f = glm::dot(d2, r);

    float s = 0.0f;
    float t = 0.0f;

    if (a <= kEpsilon && e <= kEpsilon) {
        return SegmentClosest{s, t, p0, q0};
    }

    if (a <= kEpsilon) {
        s = 0.0f;
        t = glm::clamp(f / e, 0.0f, 1.0f);
    } else {
        const float c = glm::dot(d1, r);
        if (e <= kEpsilon) {
            s = glm::clamp(-c / a, 0.0f, 1.0f);
            t = 0.0f;
        } else {
            const float b = glm::dot(d1, d2);
            const float denom = a * e - b * b;
            if (denom != 0.0f) {
                s = glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
            }
            t = glm::clamp((b * s + f) / e, 0.0f, 1.0f);
            if (denom != 0.0f) {
                s = glm::clamp((b * t - c) / a, 0.0f, 1.0f);
            }
        }
    }

    return SegmentClosest{s, t, p0 + d1 * s, q0 + d2 * t};
}

bool rayVsCapsule(const glm::vec2& origin,
                  const glm::vec2& dir,
                  float maxDistance,
                  const glm::vec2& a,
                  const glm::vec2& b,
                  float radius,
                  RayHitData& out) {
    const glm::vec2 rayEnd = origin + dir * maxDistance;
    const SegmentClosest closest = closestPointsOnSegments(a, b, origin, rayEnd);
    const glm::vec2 delta = closest.p - closest.q;
    const float dist2 = glm::dot(delta, delta);

    if (dist2 > radius * radius) {
        return false;
    }

    const float distanceAlongRay = closest.t * maxDistance;
    if (distanceAlongRay < 0.0f || distanceAlongRay > maxDistance) {
        return false;
    }

    out.t = distanceAlongRay;
    out.point = origin + dir * distanceAlongRay;
    out.normal = safeNormal(out.point - closest.p, -dir);
    return true;
}

float refineHitDistance(float low,
                        float high,
                        int iterations,
                        const std::function<bool(float)>& overlaps) {
    float lo = std::max(0.0f, low);
    float hi = std::max(lo, high);
    for (int i = 0; i < iterations; ++i) {
        const float mid = 0.5f * (lo + hi);
        if (overlaps(mid)) {
            hi = mid;
        } else {
            lo = mid;
        }
    }
    return hi;
}

float extractRadiusFromCapsuleAABB(const CapsuleCollider& capsule) {
    const AABB bounds = capsule.getAABB();
    return 0.5f * std::min(bounds.width(), bounds.height());
}

float extractRadiusFromCircleAABB(const CircleCollider& circle) {
    const AABB bounds = circle.getAABB();
    return 0.5f * std::min(bounds.width(), bounds.height());
}

glm::vec2 closestPointOnSegment(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p) {
    const glm::vec2 ab = b - a;
    const float abLen2 = glm::dot(ab, ab);
    if (abLen2 <= kEpsilon) return a;
    const float t = glm::clamp(glm::dot(p - a, ab) / abLen2, 0.0f, 1.0f);
    return a + ab * t;
}

std::unique_ptr<Hit> dispatchWithoutTriggerSideEffects(ACollider& moving, ACollider& target) {
    const bool wasTriggered = target.hasTriggered();
    auto hit = CollisionDispatcher::dispatch(moving, target);
    if (!wasTriggered && target.hasTriggered()) {
        target.clearTriggerState();
    }
    return hit;
}
} // namespace

namespace PhysicsCasts {

CastHit rayCast(const glm::vec2& origin,
                const glm::vec2& direction,
                float maxDistance,
                const std::vector<std::unique_ptr<Entity>>& entities,
                CastFilter filter) {
    CastHit best{};
    const float dirLen = glm::length(direction);
    if (dirLen < kEpsilon) {
        return best;
    }

    const glm::vec2 dir = direction / dirLen;
    const float maxDist = (maxDistance > 0.0f) ? maxDistance : kDefaultCastDistance;
    float closest = maxDist;

    const auto colliders = gatherColliders(entities);
    for (const auto& entry : colliders) {
        if (shouldSkip(entry, filter)) continue;

        RayHitData data{};
        switch (entry.collider->getType()) {
            case ColliderType::AABB: {
                auto* box = dynamic_cast<AABBCollider*>(entry.collider);
                if (box && rayVsAabb(origin, dir, box->getAABB(), closest, data)) {
                    closest = data.t;
                    best = CastHit{true, data.point, data.normal, data.t, entry.entity, entry.collider};
                }
                break;
            }
            case ColliderType::CIRCLE: {
                auto* circle = dynamic_cast<CircleCollider*>(entry.collider);
                if (circle) {
                    const AABB bounds = circle->getAABB();
                    const glm::vec2 center = bounds.center();
                    const float radius = extractRadiusFromCircleAABB(*circle);
                    if (rayVsCircle(origin, dir, center, radius, closest, data)) {
                        closest = data.t;
                        best = CastHit{true, data.point, data.normal, data.t, entry.entity, entry.collider};
                    }
                }
                break;
            }
            case ColliderType::CAPSULE: {
                auto* cap = dynamic_cast<CapsuleCollider*>(entry.collider);
                if (cap) {
                    const float radius = extractRadiusFromCapsuleAABB(*cap);
                    if (rayVsCapsule(origin, dir, closest, cap->getWorldA(), cap->getWorldB(), radius, data)) {
                        closest = data.t;
                        best = CastHit{true, data.point, data.normal, data.t, entry.entity, entry.collider};
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    return best;
}

CastHit boxCast(const AABB& box,
                const glm::vec2& direction,
                float maxDistance,
                const std::vector<std::unique_ptr<Entity>>& entities,
                CastFilter filter) {
    CastHit best{};
    const float dirLen = glm::length(direction);
    if (dirLen < kEpsilon) {
        return best;
    }

    const glm::vec2 dir = direction / dirLen;
    const float maxDist = (maxDistance > 0.0f) ? maxDistance : kDefaultCastDistance;
    float closest = maxDist;

    const glm::vec2 halfSize = (box.getMax() - box.getMin()) * 0.5f;
    const glm::vec2 startCenter = box.center();

    Transform movingTransform{};
    AABBCollider movingCollider(box.getMin(), box.getMax());
    movingCollider.setTransform(&movingTransform);

    const auto colliders = gatherColliders(entities);
    for (const auto& entry : colliders) {
        if (shouldSkip(entry, filter)) continue;

        const AABB target = entry.collider->getAABB();
        const glm::vec2 expandedMin = target.getMin() - halfSize;
        const glm::vec2 expandedMax = target.getMax() + halfSize;
        const AABB expanded(expandedMin, expandedMax);

        RayHitData data{};
        if (!rayVsAabb(startCenter, dir, expanded, closest, data)) {
            continue;
        }

        const auto overlapsAt = [&](float dist) {
            movingTransform.setPos(dir * dist);
            auto hit = dispatchWithoutTriggerSideEffects(movingCollider, *entry.collider);
            return hit && hit->collided;
        };

        const float refined = refineHitDistance(0.0f, data.t, 4, overlapsAt);
        if (refined > closest || refined > maxDist) {
            continue;
        }

        movingTransform.setPos(dir * refined);
        auto hit = dispatchWithoutTriggerSideEffects(movingCollider, *entry.collider);
        if (!hit || !hit->collided) {
            continue;
        }

        closest = refined;
        const glm::vec2 normal = safeNormal(hit->normal, data.normal);
        const glm::vec2 point = (glm::dot(hit->contactPoint, hit->contactPoint) > 0.0f)
                                ? hit->contactPoint
                                : data.point;
        best = CastHit{true, point, normal, refined, entry.entity, entry.collider};
    }

    return best;
}

CastHit capsuleCast(const glm::vec2& a,
                    const glm::vec2& b,
                    float radius,
                    const glm::vec2& direction,
                    float maxDistance,
                    const std::vector<std::unique_ptr<Entity>>& entities,
                    CastFilter filter) {
    CastHit best{};
    const float dirLen = glm::length(direction);
    if (dirLen < kEpsilon) {
        return best;
    }

    const glm::vec2 dir = direction / dirLen;
    const float maxDist = (maxDistance > 0.0f) ? maxDistance : kDefaultCastDistance;
    float closest = maxDist;

    const glm::vec2 capMin{std::min(a.x, b.x) - radius, std::min(a.y, b.y) - radius};
    const glm::vec2 capMax{std::max(a.x, b.x) + radius, std::max(a.y, b.y) + radius};
    const glm::vec2 halfSize = (capMax - capMin) * 0.5f;
    const glm::vec2 startCenter = 0.5f * (capMin + capMax);

    Transform movingTransform{};
    CapsuleCollider movingCapsule(a, b, radius);
    movingCapsule.setTransform(&movingTransform);

    const auto colliders = gatherColliders(entities);
    for (const auto& entry : colliders) {
        if (shouldSkip(entry, filter)) continue;

        const AABB target = entry.collider->getAABB();
        const glm::vec2 expandedMin = target.getMin() - halfSize;
        const glm::vec2 expandedMax = target.getMax() + halfSize;
        const AABB expanded(expandedMin, expandedMax);

        RayHitData data{};
        if (!rayVsAabb(startCenter, dir, expanded, closest, data)) {
            continue;
        }

        const auto overlapsAt = [&](float dist) {
            movingTransform.setPos(dir * dist);
            auto hit = dispatchWithoutTriggerSideEffects(movingCapsule, *entry.collider);
            return hit && hit->collided;
        };

        const float refined = refineHitDistance(0.0f, data.t, 4, overlapsAt);
        if (refined > closest || refined > maxDist) {
            continue;
        }

        movingTransform.setPos(dir * refined);
        auto hit = dispatchWithoutTriggerSideEffects(movingCapsule, *entry.collider);
        if (!hit || !hit->collided) {
            continue;
        }

        closest = refined;
        const glm::vec2 normal = safeNormal(hit->normal, data.normal);
        const glm::vec2 point = (glm::dot(hit->contactPoint, hit->contactPoint) > 0.0f)
                                ? hit->contactPoint
                                : data.point;
        best = CastHit{true, point, normal, refined, entry.entity, entry.collider};
    }

    return best;
}

std::vector<CastHit> overlapCircle(const glm::vec2& center,
                                   float radius,
                                   const std::vector<std::unique_ptr<Entity>>& entities,
                                   CastFilter filter) {
    std::vector<CastHit> hits;
    if (radius <= 0.0f) return hits;

    const auto colliders = gatherColliders(entities);
    for (const auto& entry : colliders) {
        if (shouldSkip(entry, filter)) continue;

        switch (entry.collider->getType()) {
            case ColliderType::AABB: {
                const AABB box = entry.collider->getAABB();
                const glm::vec2 closest = glm::clamp(center, box.getMin(), box.getMax());
                const glm::vec2 diff = center - closest;
                const float dist2 = glm::dot(diff, diff);
                if (dist2 <= radius * radius) {
                    const float dist = std::sqrt(std::max(dist2, 0.0f));
                    const glm::vec2 n = safeNormal(diff, glm::vec2{1.0f, 0.0f});
                    hits.push_back(CastHit{true, closest, n, radius - dist, entry.entity, entry.collider});
                }
                break;
            }
            case ColliderType::CIRCLE: {
                const auto* circle = dynamic_cast<CircleCollider*>(entry.collider);
                if (!circle) break;
                const AABB bounds = circle->getAABB();
                const glm::vec2 otherCenter = bounds.center();
                const float otherRadius = extractRadiusFromCircleAABB(*circle);
                const glm::vec2 diff = center - otherCenter;
                const float dist = glm::length(diff);
                const float sumR = radius + otherRadius;
                if (dist <= sumR) {
                    const glm::vec2 n = safeNormal(diff, glm::vec2{1.0f, 0.0f});
                    const glm::vec2 contact = otherCenter + n * otherRadius;
                    hits.push_back(CastHit{true, contact, n, sumR - dist, entry.entity, entry.collider});
                }
                break;
            }
            case ColliderType::CAPSULE: {
                const auto* cap = dynamic_cast<CapsuleCollider*>(entry.collider);
                if (!cap) break;
                const glm::vec2 a = cap->getWorldA();
                const glm::vec2 bPt = cap->getWorldB();
                const float capRadius = extractRadiusFromCapsuleAABB(*cap);
                const glm::vec2 closest = closestPointOnSegment(a, bPt, center);
                const glm::vec2 diff = center - closest;
                const float dist = glm::length(diff);
                const float sumR = radius + capRadius;
                if (dist <= sumR) {
                    const glm::vec2 n = safeNormal(diff, glm::vec2{1.0f, 0.0f});
                    const glm::vec2 contact = closest + n * capRadius;
                    hits.push_back(CastHit{true, contact, n, sumR - dist, entry.entity, entry.collider});
                }
                break;
            }
            default:
                break;
        }
    }
    return hits;
}

} // namespace PhysicsCasts
