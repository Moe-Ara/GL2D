#include "BroadphaseBVH.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include <glm/common.hpp>

namespace {
constexpr std::uint32_t kLeafCapacity = 4;

float area(const AABB& bounds) {
    return bounds.width() * bounds.height();
}
}

void BroadphaseBVH::build(const std::vector<Entry>& entries) {
    clear();
    if (entries.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error(
            "BroadphaseBVH entry count exceeds its 32-bit node index range");
    }
    m_entries.reserve(entries.size());
    m_nodes.reserve(entries.empty() ? 0 : entries.size() * 2 - 1);

    m_userScratch.clear();
    m_userScratch.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (!entries[i].user) {
            throw std::invalid_argument("BroadphaseBVH entries require non-null users");
        }
        if (!m_userScratch.insert(entries[i].user).second) {
            throw std::invalid_argument("BroadphaseBVH users must be unique");
        }
        m_entries.push_back({entries[i], i});
    }
    if (!m_entries.empty()) {
        buildNode(0, static_cast<std::uint32_t>(m_entries.size()));
    }
}

void BroadphaseBVH::clear() noexcept {
    m_entries.clear();
    m_nodes.clear();
}

std::uint32_t BroadphaseBVH::buildNode(std::uint32_t begin,
                                      std::uint32_t end) {
    const std::uint32_t nodeIndex = static_cast<std::uint32_t>(m_nodes.size());
    glm::vec2 minimum{std::numeric_limits<float>::max()};
    glm::vec2 maximum{-std::numeric_limits<float>::max()};
    for (std::uint32_t i = begin; i < end; ++i) {
        minimum = glm::min(minimum, m_entries[i].value.bounds.getMin());
        maximum = glm::max(maximum, m_entries[i].value.bounds.getMax());
    }
    const AABB bounds{minimum, maximum};
    m_nodes.push_back(Node{bounds, begin, end});
    if (end - begin <= kLeafCapacity) {
        return nodeIndex;
    }

    glm::vec2 centroidMinimum{std::numeric_limits<float>::max()};
    glm::vec2 centroidMaximum{-std::numeric_limits<float>::max()};
    for (std::uint32_t i = begin; i < end; ++i) {
        const glm::vec2 center = m_entries[i].value.bounds.center();
        centroidMinimum = glm::min(centroidMinimum, center);
        centroidMaximum = glm::max(centroidMaximum, center);
    }
    const glm::vec2 extent = centroidMaximum - centroidMinimum;
    const int axis = extent.y > extent.x ? 1 : 0;
    std::stable_sort(m_entries.begin() + begin, m_entries.begin() + end,
        [axis](const IndexedEntry& first, const IndexedEntry& second) {
            const float firstCenter = first.value.bounds.center()[axis];
            const float secondCenter = second.value.bounds.center()[axis];
            if (firstCenter != secondCenter) {
                return firstCenter < secondCenter;
            }
            return first.insertionOrder < second.insertionOrder;
        });

    const std::uint32_t middle = begin + (end - begin) / 2;
    const std::uint32_t left = buildNode(begin, middle);
    const std::uint32_t right = buildNode(middle, end);
    m_nodes[nodeIndex].left = left;
    m_nodes[nodeIndex].right = right;
    return nodeIndex;
}

void BroadphaseBVH::query(const AABB& bounds,
                          std::vector<void*>& output) const {
    if (!m_nodes.empty()) {
        queryNode(0, bounds, output);
    }
}

void BroadphaseBVH::queryNode(std::uint32_t nodeIndex, const AABB& bounds,
                              std::vector<void*>& output) const {
    const Node& node = m_nodes[nodeIndex];
    if (!node.bounds.overlaps(bounds)) {
        return;
    }
    if (node.leaf()) {
        for (std::uint32_t i = node.begin; i < node.end; ++i) {
            if (m_entries[i].value.bounds.overlaps(bounds)) {
                output.push_back(m_entries[i].value.user);
            }
        }
        return;
    }
    queryNode(node.left, bounds, output);
    queryNode(node.right, bounds, output);
}

std::vector<BroadphaseBVH::Pair> BroadphaseBVH::overlappingPairs() const {
    std::vector<Pair> result;
    overlappingPairs(result);
    return result;
}

void BroadphaseBVH::overlappingPairs(std::vector<Pair>& output) const {
    if (!m_nodes.empty()) {
        collectNodePairs(0, 0, output);
    }
}

void BroadphaseBVH::appendPair(const IndexedEntry& first,
                               const IndexedEntry& second,
                               std::vector<Pair>& output) const {
    if (!first.value.bounds.overlaps(second.value.bounds)) {
        return;
    }
    if (first.insertionOrder < second.insertionOrder) {
        output.push_back({first.value.user, second.value.user});
    } else {
        output.push_back({second.value.user, first.value.user});
    }
}

void BroadphaseBVH::collectNodePairs(std::uint32_t firstIndex,
                                     std::uint32_t secondIndex,
                                     std::vector<Pair>& output) const {
    const Node& first = m_nodes[firstIndex];
    const Node& second = m_nodes[secondIndex];
    if (!first.bounds.overlaps(second.bounds)) {
        return;
    }

    if (firstIndex == secondIndex) {
        if (first.leaf()) {
            for (std::uint32_t i = first.begin; i < first.end; ++i) {
                for (std::uint32_t j = i + 1; j < first.end; ++j) {
                    appendPair(m_entries[i], m_entries[j], output);
                }
            }
            return;
        }
        collectNodePairs(first.left, first.left, output);
        collectNodePairs(first.left, first.right, output);
        collectNodePairs(first.right, first.right, output);
        return;
    }

    if (first.leaf() && second.leaf()) {
        for (std::uint32_t i = first.begin; i < first.end; ++i) {
            for (std::uint32_t j = second.begin; j < second.end; ++j) {
                appendPair(m_entries[i], m_entries[j], output);
            }
        }
        return;
    }

    if (first.leaf() || (!second.leaf() && area(second.bounds) > area(first.bounds))) {
        collectNodePairs(firstIndex, second.left, output);
        collectNodePairs(firstIndex, second.right, output);
    } else {
        collectNodePairs(first.left, secondIndex, output);
        collectNodePairs(first.right, secondIndex, output);
    }
}
