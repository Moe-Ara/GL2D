#ifndef GL2D_LEDGESENSORCOMPONENT_HPP
#define GL2D_LEDGESENSORCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "Physics/PhysicsUnits.hpp"
#include <glm/vec2.hpp>
#include <vector>
#include <memory>

class Entity;

class LedgeSensorComponent : public IUpdatableComponent {
public:
    struct Hit {
        bool hit{false};
        glm::vec2 point{0.0f};
        glm::vec2 normal{0.0f};
        Entity* entity{nullptr};
    };

    void update(Entity& /*owner*/, double /*dt*/) override {}

    void setWorldEntities(std::vector<std::unique_ptr<Entity>>* world) { m_worldEntities = world; }
    void setLayerMask(uint32_t mask) { m_layerMask = mask; }
    void setProbeDistance(float distance) { m_probeDistance = distance; }

    Hit detect(const glm::vec2& origin,
               float maxDistance,
               Entity& owner) const;

private:
    float m_probeDistance{PhysicsUnits::toUnits(0.6f)};
    uint32_t m_layerMask{0xFFFFFFFFu};
    std::vector<std::unique_ptr<Entity>>* m_worldEntities{nullptr};
};

#endif // GL2D_LEDGESENSORCOMPONENT_HPP
