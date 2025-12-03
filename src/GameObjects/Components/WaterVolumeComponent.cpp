#include "WaterVolumeComponent.hpp"

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Entity.hpp"

void WaterVolumeComponent::update(Entity& owner, double /*dt*/) {
    auto* colliderComp = owner.getComponent<ColliderComponent>();
    if (!colliderComp) {
        return;
    }

    colliderComp->setTrigger(true);
    colliderComp->ensureCollider(owner);
}
