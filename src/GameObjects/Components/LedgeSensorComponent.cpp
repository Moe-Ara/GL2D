#include "LedgeSensorComponent.hpp"

#include "GameObjects/Entity.hpp"
#include "GameObjects/Components/LedgeComponent.hpp"
#include "Physics/PhysicsCasts.hpp"

#include <utility>

namespace {
static constexpr int kMaxCastIterations = 8;
}

LedgeSensorComponent::Hit LedgeSensorComponent::detect(const glm::vec2& origin,
                                                       float maxDistance,
                                                       Entity& owner) const {
    Hit result{};
    if (!m_worldEntities) {
        return result;
    }
    PhysicsCasts::CastFilter filter{};
    filter.layerMask = m_layerMask;
    filter.includeTriggers = true;
    filter.ignore = &owner;
    glm::vec2 direction{0.0f, -1.0f};
    const float probeDistance = (maxDistance > 0.0f) ? maxDistance : m_probeDistance;
    for (int i = 0; i < kMaxCastIterations; ++i) {
        auto hit = PhysicsCasts::rayCast(origin, direction, probeDistance, *m_worldEntities, filter);
        if (!hit.hit) {
            break;
        }
        if (hit.entity) {
            if (hit.entity == &owner) {
                filter.ignore = hit.entity;
                continue;
            }
            if (hit.entity->getComponent<LedgeComponent>()) {
                result.hit = true;
                result.point = hit.point;
                result.normal = hit.normal;
                result.entity = hit.entity;
                return result;
            }
        }
        filter.ignore = hit.entity;
    }
    return result;
}
