//
// TriggerSystem.cpp
//

#include "TriggerSystem.hpp"

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/BroadphaseBVH.hpp"
#include "Physics/Collision/ACollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"

#include <unordered_map>

namespace {
bool isTrigger(const ACollider* c) {
    return c && c->isTrigger();
}

bool hasTriggerOnceFired(const ACollider* c) {
    return c && c->isTrigger() && c->triggersOnce() && c->hasTriggered();
}
} // namespace

TriggerSystem::PairKey TriggerSystem::makeKey(uint64_t a, uint64_t b) {
    if (a < b) return PairKey{a, b};
    return PairKey{b, a};
}

void TriggerSystem::clear() {
    m_activeOverlaps.clear();
}

void TriggerSystem::unregisterEntity(uint64_t entityId) {
    for (auto it = m_activeOverlaps.begin(); it != m_activeOverlaps.end();) {
        if (it->a == entityId || it->b == entityId) {
            it = m_activeOverlaps.erase(it);
        } else {
            ++it;
        }
    }
}

void TriggerSystem::handleEnter(const ColliderEntry& a, const ColliderEntry& b, bool aWasTriggered, bool bWasTriggered) {
    if (a.component && (!a.collider || !a.collider->triggersOnce() || !aWasTriggered)) {
        a.component->invokeTriggerEnter(*a.entity, *b.entity);
    }
    if (b.component && (!b.collider || !b.collider->triggersOnce() || !bWasTriggered)) {
        b.component->invokeTriggerEnter(*b.entity, *a.entity);
    }
}

void TriggerSystem::handleExit(const ColliderEntry& a, const ColliderEntry& b) {
    if (a.component) {
        a.component->invokeTriggerExit(*a.entity, *b.entity);
    }
    if (b.component) {
        b.component->invokeTriggerExit(*b.entity, *a.entity);
    }
}

void TriggerSystem::handleStay(const ColliderEntry& a, const ColliderEntry& b) {
    if (a.component) {
        a.component->invokeTriggerStay(*a.entity, *b.entity);
    }
    if (b.component) {
        b.component->invokeTriggerStay(*b.entity, *a.entity);
    }
}

void TriggerSystem::update(const std::vector<std::unique_ptr<Entity>>& entities) {
    m_entryScratch.clear();
    m_entryScratch.reserve(entities.size());
    m_broadphaseScratch.clear();
    m_broadphaseScratch.reserve(entities.size());
    m_entriesById.clear();
    m_entriesById.reserve(entities.size());

    for (const auto& ePtr : entities) {
        if (!ePtr) continue;
        auto* colliderComp = ePtr->getComponent<ColliderComponent>();
        if (!colliderComp) continue;
        colliderComp->ensureCollider(*ePtr);
        auto* collider = colliderComp->collider();
        if (!collider) continue;
        m_entryScratch.push_back(ColliderEntry{ePtr.get(), colliderComp, collider});
    }
    for (ColliderEntry& entry : m_entryScratch) {
        m_broadphaseScratch.push_back({entry.collider->getAABB(), &entry});
        m_entriesById.emplace(entry.entity->getId(), &entry);
    }

    m_overlapScratch.clear();
    m_overlapScratch.reserve(m_activeOverlaps.size());
    auto& overlapsThisStep = m_overlapScratch;
    auto& entriesById = m_entriesById;
    m_broadphase.build(m_broadphaseScratch);
    m_pairScratch.clear();
    m_broadphase.overlappingPairs(m_pairScratch);
    for (const BroadphaseBVH::Pair& pair : m_pairScratch) {
        auto* a = static_cast<ColliderEntry*>(pair.first);
        auto* b = static_cast<ColliderEntry*>(pair.second);
        if (!a || !b || !a->entity || !b->entity ||
            !a->collider || !b->collider ||
            (!isTrigger(a->collider) && !isTrigger(b->collider))) {
            continue;
        }

        const PairKey key = makeKey(a->entity->getId(), b->entity->getId());
        const bool wasActive = m_activeOverlaps.contains(key);
        const bool aTriggeredBefore = hasTriggerOnceFired(a->collider);
        const bool bTriggeredBefore = hasTriggerOnceFired(b->collider);
        const bool consumedOneShot = aTriggeredBefore || bTriggeredBefore;
        const bool overlapping = static_cast<bool>(
            CollisionDispatcher::dispatch(*a->collider, *b->collider));
        if (!overlapping || (consumedOneShot && !wasActive)) {
            continue;
        }

        overlapsThisStep.insert(key);
        if (!wasActive) {
            handleEnter(*a, *b, aTriggeredBefore, bTriggeredBefore);
            if (a->collider->isTrigger() && a->collider->triggersOnce()) {
                a->collider->markTriggerFired();
            }
            if (b->collider->isTrigger() && b->collider->triggersOnce()) {
                b->collider->markTriggerFired();
            }
        }
        handleStay(*a, *b);
    }

    for (const PairKey& active : m_activeOverlaps) {
        if (overlapsThisStep.contains(active)) {
            continue;
        }
        const auto first = entriesById.find(active.a);
        const auto second = entriesById.find(active.b);
        if (first != entriesById.end() && second != entriesById.end()) {
            handleExit(*first->second, *second->second);
        }
    }
    std::swap(m_activeOverlaps, m_overlapScratch);
}
