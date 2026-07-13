//
// TriggerSystem.hpp
//

#ifndef GL2D_TRIGGERSYSTEM_HPP
#define GL2D_TRIGGERSYSTEM_HPP

#include "GameObjects/Entity.hpp"
#include "Physics/BroadphaseBVH.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ColliderComponent;
class ACollider;

class TriggerSystem {
public:
    TriggerSystem() = default;
    ~TriggerSystem() = default;

    TriggerSystem(const TriggerSystem&) = delete;
    TriggerSystem& operator=(const TriggerSystem&) = delete;
    TriggerSystem(TriggerSystem&&) = delete;
    TriggerSystem& operator=(TriggerSystem&&) = delete;

    void update(const std::vector<std::unique_ptr<Entity>>& entities);
    void clear();
    void unregisterEntity(uint64_t entityId);

private:
    struct ColliderEntry {
        Entity* entity{nullptr};
        ColliderComponent* component{nullptr};
        ACollider* collider{nullptr};
    };

    struct PairKey {
        uint64_t a;
        uint64_t b;
        bool operator==(const PairKey&) const = default;
    };

    struct PairKeyHash {
        size_t operator()(const PairKey& key) const noexcept {
            return std::hash<uint64_t>{}(key.a) ^ (std::hash<uint64_t>{}(key.b) << 1);
        }
    };

    static PairKey makeKey(uint64_t a, uint64_t b);
    void handleEnter(const ColliderEntry& a, const ColliderEntry& b, bool aWasTriggered, bool bWasTriggered);
    void handleStay(const ColliderEntry& a, const ColliderEntry& b);
    void handleExit(const ColliderEntry& a, const ColliderEntry& b);

    std::unordered_set<PairKey, PairKeyHash> m_activeOverlaps{};
    // Per-update scratch, kept as members so steady-state updates do not
    // allocate.
    std::vector<ColliderEntry> m_entryScratch;
    std::vector<BroadphaseBVH::Entry> m_broadphaseScratch;
    std::vector<BroadphaseBVH::Pair> m_pairScratch;
    std::unordered_map<uint64_t, ColliderEntry*> m_entriesById;
    std::unordered_set<PairKey, PairKeyHash> m_overlapScratch;
    BroadphaseBVH m_broadphase;
};

#endif //GL2D_TRIGGERSYSTEM_HPP
