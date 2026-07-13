#include "ECS/Systems/KinematicCharacterPhysicsSystem.hpp"

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Collision2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace ECS {
namespace {
constexpr float kSkin = 0.01f;
constexpr float kGroundProbe = 0.5f;
constexpr float kOverlapEpsilon = 1e-5f;

struct Bounds {
    glm::vec2 min{0.0f};
    glm::vec2 max{0.0f};
};

struct StaticEntry {
    Entity entity;
    Bounds bounds;
    glm::vec2 velocity{0.0f};
    std::uint32_t categoryBits{1u};
    std::uint32_t maskBits{0xFFFFFFFFu};
};

bool finite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

void validate(const Transform2D& transform, const AabbCollider2D& collider) {
    if (!finite(transform.position) || !finite(transform.scale) ||
        !finite(collider.halfExtents) || !finite(collider.offset) ||
        collider.halfExtents.x <= 0.0f || collider.halfExtents.y <= 0.0f) {
        throw std::invalid_argument("ECS AABB collider has invalid transform or bounds");
    }
}

Bounds boundsFor(const Transform2D& transform, const AabbCollider2D& collider) {
    const glm::vec2 center = transform.position + collider.offset * transform.scale;
    const glm::vec2 half = glm::abs(collider.halfExtents * transform.scale);
    return {center - half, center + half};
}

bool overlapsStrict(float minA, float maxA, float minB, float maxB) {
    return maxA > minB + kOverlapEpsilon && minA < maxB - kOverlapEpsilon;
}

bool overlaps(const Bounds& a, const Bounds& b) {
    return overlapsStrict(a.min.x, a.max.x, b.min.x, b.max.x) &&
           overlapsStrict(a.min.y, a.max.y, b.min.y, b.max.y);
}

bool layersCollide(const AabbCollider2D& character, const StaticEntry& obstacle) {
    return (character.maskBits & obstacle.categoryBits) != 0u &&
           (obstacle.maskBits & character.categoryBits) != 0u;
}

void translate(Bounds& bounds, const glm::vec2& delta) {
    bounds.min += delta;
    bounds.max += delta;
}

void resolveInitialOverlaps(Transform2D& transform, Bounds& characterBounds,
                            KinematicBody2D& body, GroundContact2D& ground,
                            CharacterCollisionState2D& collisions,
                            Entity character,
                            const AabbCollider2D& collider,
                            const std::vector<StaticEntry>& statics) {
    for (int pass = 0; pass < 4; ++pass) {
        bool resolved = false;
        for (const StaticEntry& obstacle : statics) {
            if (obstacle.entity == character) {
                continue;
            }
            if (obstacle.bounds.max.x < characterBounds.min.x) {
                continue;
            }
            if (obstacle.bounds.min.x > characterBounds.max.x) {
                break;
            }
            if (!layersCollide(collider, obstacle) || !overlaps(characterBounds, obstacle.bounds)) {
                continue;
            }

            const std::array translations{
                glm::vec2{obstacle.bounds.min.x - characterBounds.max.x - kSkin, 0.0f},
                glm::vec2{obstacle.bounds.max.x - characterBounds.min.x + kSkin, 0.0f},
                glm::vec2{0.0f, obstacle.bounds.min.y - characterBounds.max.y - kSkin},
                glm::vec2{0.0f, obstacle.bounds.max.y - characterBounds.min.y + kSkin}
            };
            const auto best = std::ranges::min_element(translations, {}, [](const glm::vec2& value) {
                return std::abs(value.x) + std::abs(value.y);
            });
            transform.position += *best;
            translate(characterBounds, *best);
            if (best->x != 0.0f) {
                body.velocity.x = 0.0f;
                collisions.hitWallLeft |= best->x > 0.0f;
                collisions.hitWallRight |= best->x < 0.0f;
                collisions.wallNormal = {best->x > 0.0f ? 1.0f : -1.0f, 0.0f};
            } else {
                body.velocity.y = 0.0f;
                if (best->y > 0.0f) {
                    ground.grounded = true;
                    ground.normal = {0.0f, 1.0f};
                    ground.platformVelocity = obstacle.velocity;
                    ground.surface = obstacle.entity;
                } else {
                    collisions.hitCeiling = true;
                }
            }
            resolved = true;
        }
        if (!resolved) {
            break;
        }
    }
}
}

void KinematicCharacterPhysicsSystem::update(Registry& registry, float fixedDeltaTime) {
    if (!std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0f) {
        throw std::invalid_argument(
            "KinematicCharacterPhysicsSystem requires a positive finite fixed delta time");
    }

    registry.each<Transform2D, StaticCollider2D, SurfaceVelocity2D>(
        [fixedDeltaTime](Entity, Transform2D& transform, const StaticCollider2D&,
                         const SurfaceVelocity2D& surface) {
            if (!finite(surface.velocity)) {
                throw std::invalid_argument("Surface velocity must be finite");
            }
            transform.position += surface.velocity * fixedDeltaTime;
        });

    std::vector<StaticEntry> statics;
    statics.reserve(registry.size());
    registry.each<Transform2D, AabbCollider2D, StaticCollider2D>(
        [&](Entity entity, const Transform2D& transform,
            const AabbCollider2D& collider, const StaticCollider2D&) {
            validate(transform, collider);
            const auto* surface = registry.tryGet<SurfaceVelocity2D>(entity);
            statics.push_back({entity, boundsFor(transform, collider),
                               surface ? surface->velocity : glm::vec2{0.0f},
                               collider.categoryBits, collider.maskBits});
        });
    std::ranges::sort(statics, {}, [](const StaticEntry& entry) {
        return entry.bounds.min.x;
    });

    registry.each<Transform2D, AabbCollider2D, KinematicBody2D,
                  GroundContact2D, CharacterCollisionState2D>(
        [&](Entity entity, Transform2D& transform, const AabbCollider2D& collider,
            KinematicBody2D& body, GroundContact2D& ground,
            CharacterCollisionState2D& collisions) {
            validate(transform, collider);
            if (!finite(body.velocity)) {
                throw std::invalid_argument("Kinematic body velocity must be finite");
            }

            const glm::vec2 inheritedSurfaceVelocity = ground.grounded
                ? ground.platformVelocity : glm::vec2{0.0f};
            ground = {};
            collisions = {};
            Bounds characterBounds = boundsFor(transform, collider);
            resolveInitialOverlaps(transform, characterBounds, body, ground,
                                   collisions, entity, collider, statics);

            const float deltaX = (body.velocity.x + inheritedSurfaceVelocity.x) * fixedDeltaTime;
            float resolvedX = transform.position.x + deltaX;
            const float sweepMinX = std::min(characterBounds.min.x,
                                             characterBounds.min.x + deltaX) - kSkin;
            const float sweepMaxX = std::max(characterBounds.max.x,
                                             characterBounds.max.x + deltaX) + kSkin;
            for (const StaticEntry& obstacle : statics) {
                if (obstacle.entity == entity) {
                    continue;
                }
                if (obstacle.bounds.max.x < sweepMinX) {
                    continue;
                }
                if (obstacle.bounds.min.x > sweepMaxX) {
                    break;
                }
                if (!layersCollide(collider, obstacle) ||
                    !overlapsStrict(characterBounds.min.y, characterBounds.max.y,
                                    obstacle.bounds.min.y, obstacle.bounds.max.y)) {
                    continue;
                }
                if (deltaX > 0.0f && characterBounds.max.x <= obstacle.bounds.min.x + kSkin &&
                    characterBounds.max.x + deltaX >= obstacle.bounds.min.x) {
                    resolvedX = std::min(resolvedX,
                        transform.position.x + obstacle.bounds.min.x - characterBounds.max.x - kSkin);
                    collisions.hitWallRight = true;
                    collisions.wallNormal = {-1.0f, 0.0f};
                } else if (deltaX < 0.0f && characterBounds.min.x >= obstacle.bounds.max.x - kSkin &&
                           characterBounds.min.x + deltaX <= obstacle.bounds.max.x) {
                    resolvedX = std::max(resolvedX,
                        transform.position.x + obstacle.bounds.max.x - characterBounds.min.x + kSkin);
                    collisions.hitWallLeft = true;
                    collisions.wallNormal = {1.0f, 0.0f};
                }
            }
            const float appliedX = resolvedX - transform.position.x;
            transform.position.x = resolvedX;
            translate(characterBounds, {appliedX, 0.0f});
            if ((deltaX > 0.0f && appliedX + kOverlapEpsilon < deltaX) ||
                (deltaX < 0.0f && appliedX - kOverlapEpsilon > deltaX)) {
                body.velocity.x = 0.0f;
            }

            const float deltaY = (body.velocity.y + inheritedSurfaceVelocity.y) * fixedDeltaTime;
            float resolvedY = transform.position.y + deltaY;
            const StaticEntry* landedSurface = nullptr;
            for (const StaticEntry& obstacle : statics) {
                if (obstacle.entity == entity) {
                    continue;
                }
                if (obstacle.bounds.max.x < characterBounds.min.x) {
                    continue;
                }
                if (obstacle.bounds.min.x > characterBounds.max.x) {
                    break;
                }
                if (!layersCollide(collider, obstacle) ||
                    !overlapsStrict(characterBounds.min.x, characterBounds.max.x,
                                    obstacle.bounds.min.x, obstacle.bounds.max.x)) {
                    continue;
                }
                if (deltaY > 0.0f && characterBounds.max.y <= obstacle.bounds.min.y + kSkin &&
                    characterBounds.max.y + deltaY >= obstacle.bounds.min.y) {
                    resolvedY = std::min(resolvedY,
                        transform.position.y + obstacle.bounds.min.y - characterBounds.max.y - kSkin);
                    collisions.hitCeiling = true;
                } else if (deltaY <= 0.0f && characterBounds.min.y >= obstacle.bounds.max.y - kSkin &&
                           characterBounds.min.y + deltaY <= obstacle.bounds.max.y) {
                    const float candidate = transform.position.y +
                        obstacle.bounds.max.y - characterBounds.min.y + kSkin;
                    if (!landedSurface || candidate > resolvedY) {
                        resolvedY = std::max(resolvedY, candidate);
                        landedSurface = &obstacle;
                    }
                }
            }
            const float appliedY = resolvedY - transform.position.y;
            transform.position.y = resolvedY;
            translate(characterBounds, {0.0f, appliedY});
            if (collisions.hitCeiling && deltaY > 0.0f &&
                appliedY + kOverlapEpsilon < deltaY) {
                body.velocity.y = 0.0f;
            }
            if (landedSurface && deltaY <= 0.0f &&
                appliedY - kOverlapEpsilon > deltaY) {
                body.velocity.y = 0.0f;
                ground.grounded = true;
                ground.normal = {0.0f, 1.0f};
                ground.platformVelocity = landedSurface->velocity;
                ground.surface = landedSurface->entity;
            }

            if (!ground.grounded && body.velocity.y <= 0.0f) {
                const StaticEntry* nearestSurface = nullptr;
                float nearestSeparation = kGroundProbe + kOverlapEpsilon;
                for (const StaticEntry& obstacle : statics) {
                    if (obstacle.entity == entity) {
                        continue;
                    }
                    if (obstacle.bounds.max.x < characterBounds.min.x) {
                        continue;
                    }
                    if (obstacle.bounds.min.x > characterBounds.max.x) {
                        break;
                    }
                    if (!layersCollide(collider, obstacle) ||
                        !overlapsStrict(characterBounds.min.x, characterBounds.max.x,
                                        obstacle.bounds.min.x, obstacle.bounds.max.x)) {
                        continue;
                    }
                    const float separation = characterBounds.min.y - obstacle.bounds.max.y;
                    if (separation >= -kOverlapEpsilon &&
                        separation <= kGroundProbe &&
                        (separation < nearestSeparation ||
                         (std::abs(separation - nearestSeparation) <= kOverlapEpsilon &&
                          (!nearestSurface || obstacle.entity.index() <
                           nearestSurface->entity.index())))) {
                        nearestSurface = &obstacle;
                        nearestSeparation = separation;
                    }
                }
                if (nearestSurface) {
                    ground.grounded = true;
                    ground.normal = {0.0f, 1.0f};
                    ground.platformVelocity = nearestSurface->velocity;
                    ground.surface = nearestSurface->entity;
                }
            }
        });
}

} // namespace ECS
