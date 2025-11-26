#pragma once

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Physics/Collision/AABB.hpp"

// Lightweight quadtree for broad-phase queries. Stores user pointers plus
// their bounds; caller keeps bounds updated when inserting/updating.
class Quadtree {
public:
    struct Config {
        int maxDepth{6};
        int maxObjectsPerNode{6};
        float minSize{1.0f};
    };

    struct Entry {
        AABB bounds;
        void* user{nullptr};
    };

    Quadtree(const AABB& worldBounds, Config config = {});

    void clear();
    void setBounds(const AABB& worldBounds);
    void insert(const AABB& bounds, void* user);
    void update(const AABB& oldBounds, const AABB& newBounds, void* user);
    void remove(void* user);

    // Gather all users whose bounds overlap query.
    void query(const AABB& queryBounds, std::vector<void*>& out) const;

    // Debug draw: caller supplies a function to draw an AABB with a color.
    void debugDraw(const std::function<void(const AABB&, const glm::vec3&)>& drawFn,
                   const glm::vec3& color = {0.0f, 1.0f, 0.0f}) const;

    // Convenience: set world bounds using min/max and optionally prebuild a full grid to max depth.
    void setBounds(const glm::vec2& min, const glm::vec2& max, bool prebuild = false);
    void prebuild(); // eagerly subdivide from the root to maxDepth (or minSize).

private:
    struct Node {
        AABB bounds;
        std::vector<Entry> items;
        std::array<std::unique_ptr<Node>, 4> children{};
        bool isLeaf() const {
            return !children[0] && !children[1] && !children[2] && !children[3];
        }
    };

    bool canSubdivide(const Node& node, int depth) const;
    void spawnChildren(Node& node);
    void prebuild(Node& node, int depth);
    void subdivide(Node& node, int depth);
    int pickChild(const Node& node, const AABB& bounds) const;
    bool fitsChild(const Node& node, const AABB& bounds, int childIndex) const;
    void insert(Node& node, const AABB& bounds, void* user, int depth);
    void query(const Node& node, const AABB& queryBounds, std::vector<void*>& out) const;
    bool remove(Node& node, void* user);
    void maybeMerge(Node& node, int depth);

    Config m_config{};
    Node m_root{};
};
