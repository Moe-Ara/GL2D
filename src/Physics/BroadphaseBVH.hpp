#pragma once

#include "Physics/Collision/AABB.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_set>
#include <vector>

// Rebuilt broadphase for dynamic worlds. A flat median-split BVH is cheap to
// construct, has no authored world bounds, and emits each overlapping pair once.
class BroadphaseBVH final {
public:
    struct Entry {
        AABB bounds;
        void* user{nullptr};
    };

    struct Pair {
        void* first{nullptr};
        void* second{nullptr};
    };

    // Rebuilds from the given entries, reusing the tree's internal storage
    // capacity across rebuilds — this matters for broadphases rebuilt every
    // substep.
    void build(const std::vector<Entry>& entries);
    void clear() noexcept;

    [[nodiscard]] std::size_t size() const noexcept { return m_entries.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_entries.empty(); }

    void query(const AABB& bounds, std::vector<void*>& output) const;
    [[nodiscard]] std::vector<Pair> overlappingPairs() const;
    // Appends into a caller-owned buffer so per-frame callers can reuse it.
    void overlappingPairs(std::vector<Pair>& output) const;

private:
    struct IndexedEntry {
        Entry value;
        std::size_t insertionOrder{0};
    };

    struct Node {
        static constexpr std::uint32_t invalidNode =
            std::numeric_limits<std::uint32_t>::max();
        AABB bounds;
        std::uint32_t begin{0};
        std::uint32_t end{0};
        std::uint32_t left{invalidNode};
        std::uint32_t right{invalidNode};

        [[nodiscard]] bool leaf() const noexcept { return left == invalidNode; }
    };

    std::uint32_t buildNode(std::uint32_t begin, std::uint32_t end);
    void queryNode(std::uint32_t nodeIndex, const AABB& bounds,
                   std::vector<void*>& output) const;
    void collectNodePairs(std::uint32_t first, std::uint32_t second,
                          std::vector<Pair>& output) const;
    void appendPair(const IndexedEntry& first, const IndexedEntry& second,
                    std::vector<Pair>& output) const;

    std::vector<IndexedEntry> m_entries;
    std::vector<Node> m_nodes;
    // Duplicate-user validation scratch; kept as a member so its bucket
    // storage survives per-frame rebuilds.
    std::unordered_set<void*> m_userScratch;
};
