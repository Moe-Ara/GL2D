//
// TriggerSystem.hpp
//

#ifndef GL2D_TRIGGERSYSTEM_HPP
#define GL2D_TRIGGERSYSTEM_HPP

#include "GameObjects/Entity.hpp"

#include <memory>
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
    void handleExit(const ColliderEntry& a, const ColliderEntry& b);
    static bool overlapsAABB(const ACollider& a, const ACollider& b);

    std::unordered_set<PairKey, PairKeyHash> m_activeOverlaps{};
};

#endif //GL2D_TRIGGERSYSTEM_HPP
