#include "Quadtree.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <utility>

namespace {
constexpr float kEpsilon = 1e-4f;
}

Quadtree::Quadtree(const AABB& worldBounds, Config config) : m_config(config) {
    m_root.bounds = worldBounds;
}

void Quadtree::clear() {
    m_root.items.clear();
    for (auto& c : m_root.children) {
        c.reset();
    }
}

void Quadtree::setBounds(const AABB& worldBounds) {
    clear();
    m_root.bounds = worldBounds;
}

void Quadtree::setBounds(const glm::vec2& min, const glm::vec2& max, bool prebuildTree) {
    setBounds(AABB(min, max));
    if (prebuildTree) {
        prebuild();
    }
}

void Quadtree::insert(const AABB& bounds, void* user) {
    if (!user) return;
    insert(m_root, bounds, user, 0);
}

void Quadtree::update(const AABB& oldBounds, const AABB& newBounds, void* user) {
    remove(user);
    insert(newBounds, user);
}

void Quadtree::remove(void* user) {
    if (!user) return;
    remove(m_root, user);
}

void Quadtree::query(const AABB& queryBounds, std::vector<void*>& out) const {
    query(m_root, queryBounds, out);
}

void Quadtree::debugDraw(const std::function<void(const AABB&, const glm::vec3&)>& drawFn,
                         const glm::vec3& color) const {
    if (!drawFn) return;
    std::function<void(const Node&, int)> recurse = [&](const Node& node, int depth) {
        // Slightly vary color per depth for readability.
        const float fade = 1.0f - std::min(depth * 0.08f, 0.5f);
        drawFn(node.bounds, glm::vec3(color.r * fade, color.g * fade, color.b * fade));
        if (node.isLeaf()) return;
        for (const auto& child : node.children) {
            if (child) recurse(*child, depth + 1);
        }
    };
    recurse(m_root, 0);
}

void Quadtree::prebuild() {
    const AABB rootBounds = m_root.bounds;
    clear();
    m_root.bounds = rootBounds;
    prebuild(m_root, 0);
}

bool Quadtree::canSubdivide(const Node& node, int depth) const {
    if (depth >= m_config.maxDepth) return false;
    const glm::vec2 size = node.bounds.getMax() - node.bounds.getMin();
    return size.x > m_config.minSize + kEpsilon && size.y > m_config.minSize + kEpsilon;
}

void Quadtree::spawnChildren(Node& node) {
    const glm::vec2 min = node.bounds.getMin();
    const glm::vec2 max = node.bounds.getMax();
    const glm::vec2 center = node.bounds.center();

    const AABB quadrants[4] = {
        AABB({min.x, min.y}, {center.x, center.y}),         // SW
        AABB({center.x, min.y}, {max.x, center.y}),          // SE
        AABB({min.x, center.y}, {center.x, max.y}),          // NW
        AABB({center.x, center.y}, {max.x, max.y})           // NE
    };

    for (int i = 0; i < 4; ++i) {
        if (!node.children[i]) {
            node.children[i] = std::make_unique<Node>();
        }
        node.children[i]->bounds = quadrants[i];
        node.children[i]->items.clear();
    }
}

void Quadtree::prebuild(Node& node, int depth) {
    if (!canSubdivide(node, depth)) {
        return;
    }
    spawnChildren(node);
    for (auto& child : node.children) {
        if (child) {
            prebuild(*child, depth + 1);
        }
    }
}

void Quadtree::subdivide(Node& node, int depth) {
    if (!canSubdivide(node, depth)) return;

    spawnChildren(node);

    // Re-insert items that fully fit into children.
    std::vector<Entry> toReinsert;
    toReinsert.swap(node.items);
    for (const auto& e : toReinsert) {
        insert(node, e.bounds, e.user, depth);
    }
}

int Quadtree::pickChild(const Node& node, const AABB& bounds) const {
    for (int i = 0; i < 4; ++i) {
        if (fitsChild(node, bounds, i)) {
            return i;
        }
    }
    return -1;
}

bool Quadtree::fitsChild(const Node& node, const AABB& bounds, int childIndex) const {
    if (childIndex < 0 || childIndex >= 4) return false;
    const auto& child = node.children[childIndex];
    if (!child) return false;

    return bounds.getMin().x >= child->bounds.getMin().x - kEpsilon &&
           bounds.getMin().y >= child->bounds.getMin().y - kEpsilon &&
           bounds.getMax().x <= child->bounds.getMax().x + kEpsilon &&
           bounds.getMax().y <= child->bounds.getMax().y + kEpsilon;
}

void Quadtree::insert(Node& node, const AABB& bounds, void* user, int depth) {
    if (!node.isLeaf()) {
        const int childIdx = pickChild(node, bounds);
        if (childIdx != -1) {
            insert(*node.children[childIdx], bounds, user, depth + 1);
            return;
        }
    }

    node.items.push_back(Entry{bounds, user});

    const bool shouldSubdivide = node.isLeaf() &&
                                 static_cast<int>(node.items.size()) > m_config.maxObjectsPerNode &&
                                 canSubdivide(node, depth);
    if (shouldSubdivide) {
        subdivide(node, depth);
    }
}

void Quadtree::query(const Node& node, const AABB& queryBounds, std::vector<void*>& out) const {
    if (!node.bounds.overlaps(queryBounds)) {
        return;
    }

    for (const auto& e : node.items) {
        if (e.bounds.overlaps(queryBounds)) {
            out.push_back(e.user);
        }
    }

    if (node.isLeaf()) {
        return;
    }
    for (const auto& child : node.children) {
        if (child) {
            query(*child, queryBounds, out);
        }
    }
}

bool Quadtree::remove(Node& node, void* user) {
    const auto it = std::find_if(node.items.begin(), node.items.end(),
                                 [&](const Entry& e) { return e.user == user; });
    if (it != node.items.end()) {
        node.items.erase(it);
        return true;
    }

    if (node.isLeaf()) return false;

    bool removed = false;
    for (auto& child : node.children) {
        if (child && remove(*child, user)) {
            removed = true;
            break;
        }
    }

    if (removed) {
        maybeMerge(node, 0);
    }
    return removed;
}

void Quadtree::maybeMerge(Node& node, int depth) {
    if (node.isLeaf()) return;

    int total = static_cast<int>(node.items.size());
    for (const auto& child : node.children) {
        if (child) {
            total += static_cast<int>(child->items.size());
        }
    }

    const bool shouldMerge = total <= m_config.maxObjectsPerNode || depth >= m_config.maxDepth;
    if (!shouldMerge) return;

    for (auto& child : node.children) {
        if (child) {
            node.items.insert(node.items.end(), child->items.begin(), child->items.end());
            child.reset();
        }
    }
}
