#ifndef BEHAVIOUR_TREE_HPP
#define BEHAVIOUR_TREE_HPP
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <ostream>

enum struct NodeType {
    Selector,
    Sequence,
    Action,
    Condition
};

enum struct NodeStatus {
    Success,
    Failure,
    Running
};

inline std::ostream& operator<<(std::ostream& os, const NodeStatus status) {
    switch (status) {
        case NodeStatus::Success: os << "Success"; break;
        case NodeStatus::Failure: os << "Failure"; break;
        case NodeStatus::Running: os << "Running"; break;
    }
    return os;
}

template<typename TContext>
struct BTNode {
    NodeType type{NodeType::Selector};
    std::vector<std::unique_ptr<BTNode>> children;
    // Leaf callbacks: return Success/Failure/Running with typed context pointer.
    std::function<NodeStatus(TContext*)> tickFn;
    // Memory to resume running children in composites.
    int runningChild{-1};
};

// Typed behaviour tree with common decorators.
template<typename TContext>
class BehaviourTree {
public:
    using Node = BTNode<TContext>;

    BehaviourTree() = default;

    void setRoot(std::unique_ptr<Node> root) { m_root = std::move(root); }
    NodeStatus tick(TContext* ctx = nullptr, float dt = 0.0f);

    static std::unique_ptr<Node> makeSelector();
    static std::unique_ptr<Node> makeSequence();
    static std::unique_ptr<Node> makeAction(std::function<NodeStatus(TContext*)> fn);
    static std::unique_ptr<Node> makeCondition(std::function<bool(TContext*)> fn);

    // Decorators
    static std::unique_ptr<Node> makeInverter(std::unique_ptr<Node> child);
    static std::unique_ptr<Node> makeSucceeder(std::unique_ptr<Node> child);
    static std::unique_ptr<Node> makeFailer(std::unique_ptr<Node> child);
    static std::unique_ptr<Node> makeCooldown(std::unique_ptr<Node> child, float seconds);
    static std::unique_ptr<Node> makeRepeater(std::unique_ptr<Node> child, int count = -1); // -1 = forever

    void reset();

private:
    NodeStatus tickNode(Node& node, TContext* ctx, float dt);

    std::unique_ptr<Node> m_root;
    // Per-node timers for cooldowns; keyed by pointer.
    std::unordered_map<Node*, float> m_cooldowns;
};

#include "BehaviourTree.inl"

#endif // BEHAVIOUR_TREE_HPP
