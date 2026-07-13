#ifndef BEHAVIOUR_TREE_HPP
#define BEHAVIOUR_TREE_HPP
#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace AI {

enum struct NodeType {
    Selector,
    Sequence,
    Action,
    Condition,
    Inverter,
    Succeeder,
    Failer,
    Cooldown,
    Repeater
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

template<typename TContext> class BehaviourTree;

template<typename TContext>
class BTNode {
public:
    BTNode(const BTNode&) = delete;
    BTNode& operator=(const BTNode&) = delete;
    BTNode(BTNode&&) = delete;
    BTNode& operator=(BTNode&&) = delete;

    void addChild(std::unique_ptr<BTNode> child) {
        if (!child) {
            throw std::invalid_argument("BehaviourTree child cannot be null");
        }
        if (m_type != NodeType::Selector && m_type != NodeType::Sequence) {
            throw std::logic_error("Only BehaviourTree composites accept additional children");
        }
        m_children.push_back(std::move(child));
    }

    [[nodiscard]] NodeType type() const noexcept { return m_type; }
    [[nodiscard]] std::size_t childCount() const noexcept { return m_children.size(); }

private:
    friend class BehaviourTree<TContext>;
    explicit BTNode(NodeType type) : m_type(type) {}

    NodeType m_type;
    std::vector<std::unique_ptr<BTNode>> m_children;
    // Leaf callbacks: return Success/Failure/Running with typed context pointer.
    std::function<NodeStatus(TContext*)> m_tickFn;
    // Memory to resume running children in composites.
    std::size_t m_runningChild{0};
    bool m_hasRunningChild{false};
    // Decorator configuration and runtime state. These fields are intentionally
    // stored on the node so moving or replacing a tree cannot leave pointer-keyed
    // state behind.
    float m_durationSeconds{0.0f};
    float m_remainingSeconds{0.0f};
    int m_repeatLimit{-1};
    int m_repetitions{0};
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
    static void resetNode(Node& node);

    std::unique_ptr<Node> m_root;
};

} // namespace AI

#include "BehaviourTree.inl"

#endif // BEHAVIOUR_TREE_HPP
