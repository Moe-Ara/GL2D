#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace AI {

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeSelector() {
    return std::unique_ptr<Node>(new Node(NodeType::Selector));
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeSequence() {
    return std::unique_ptr<Node>(new Node(NodeType::Sequence));
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeAction(std::function<NodeStatus(TContext*)> fn) {
    if (!fn) {
        throw std::invalid_argument("BehaviourTree action requires a callback");
    }
    auto n = std::unique_ptr<Node>(new Node(NodeType::Action));
    n->m_tickFn = std::move(fn);
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeCondition(std::function<bool(TContext*)> fn) {
    if (!fn) {
        throw std::invalid_argument("BehaviourTree condition requires a callback");
    }
    auto n = std::unique_ptr<Node>(new Node(NodeType::Condition));
    n->m_tickFn = [fn = std::move(fn)](TContext* ctx) {
        return fn(ctx) ? NodeStatus::Success : NodeStatus::Failure;
    };
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeInverter(std::unique_ptr<Node> child) {
    if (!child) {
        throw std::invalid_argument("BehaviourTree inverter requires a child");
    }
    auto n = std::unique_ptr<Node>(new Node(NodeType::Inverter));
    n->m_children.push_back(std::move(child));
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeSucceeder(std::unique_ptr<Node> child) {
    if (!child) {
        throw std::invalid_argument("BehaviourTree succeeder requires a child");
    }
    auto n = std::unique_ptr<Node>(new Node(NodeType::Succeeder));
    n->m_children.push_back(std::move(child));
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeFailer(std::unique_ptr<Node> child) {
    if (!child) {
        throw std::invalid_argument("BehaviourTree failer requires a child");
    }
    auto n = std::unique_ptr<Node>(new Node(NodeType::Failer));
    n->m_children.push_back(std::move(child));
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeCooldown(std::unique_ptr<Node> child, float seconds) {
    if (!child) {
        throw std::invalid_argument("BehaviourTree cooldown requires a child");
    }
    if (!std::isfinite(seconds) || seconds < 0.0f) {
        throw std::invalid_argument("BehaviourTree cooldown duration must be finite and non-negative");
    }
    auto n = std::unique_ptr<Node>(new Node(NodeType::Cooldown));
    n->m_children.push_back(std::move(child));
    n->m_durationSeconds = seconds;
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeRepeater(std::unique_ptr<Node> child, int count) {
    if (!child) {
        throw std::invalid_argument("BehaviourTree repeater requires a child");
    }
    if (count < -1) {
        throw std::invalid_argument("BehaviourTree repeat count must be -1 or non-negative");
    }
    auto n = std::unique_ptr<Node>(new Node(NodeType::Repeater));
    n->m_children.push_back(std::move(child));
    n->m_repeatLimit = count;
    return n;
}

template<typename TContext>
NodeStatus BehaviourTree<TContext>::tick(TContext* ctx, float dt) {
    if (!std::isfinite(dt) || dt < 0.0f) {
        throw std::invalid_argument("BehaviourTree tick delta must be finite and non-negative");
    }
    if (!m_root) return NodeStatus::Failure;
    return tickNode(*m_root, ctx, dt);
}

template<typename TContext>
void BehaviourTree<TContext>::reset() {
    if (m_root) resetNode(*m_root);
}

template<typename TContext>
void BehaviourTree<TContext>::resetNode(Node& node) {
    node.m_runningChild = 0;
    node.m_hasRunningChild = false;
    node.m_remainingSeconds = 0.0f;
    node.m_repetitions = 0;
    for (auto& child : node.m_children) {
        if (child) resetNode(*child);
    }
}

template<typename TContext>
NodeStatus BehaviourTree<TContext>::tickNode(Node& node, TContext* ctx, float dt) {
    switch (node.m_type) {
        case NodeType::Selector: {
            const std::size_t startIdx = node.m_hasRunningChild ? node.m_runningChild : 0;
            node.m_hasRunningChild = false;
            for (std::size_t i = startIdx; i < node.m_children.size(); ++i) {
                auto& child = node.m_children[i];
                const NodeStatus s = tickNode(*child, ctx, dt);
                if (s == NodeStatus::Success) return s;
                if (s == NodeStatus::Running) {
                    node.m_runningChild = i;
                    node.m_hasRunningChild = true;
                    return s;
                }
            }
            return NodeStatus::Failure;
        }
        case NodeType::Sequence: {
            const std::size_t startIdx = node.m_hasRunningChild ? node.m_runningChild : 0;
            node.m_hasRunningChild = false;
            for (std::size_t i = startIdx; i < node.m_children.size(); ++i) {
                auto& child = node.m_children[i];
                const NodeStatus s = tickNode(*child, ctx, dt);
                if (s == NodeStatus::Failure) return s;
                if (s == NodeStatus::Running) {
                    node.m_runningChild = i;
                    node.m_hasRunningChild = true;
                    return s;
                }
            }
            return NodeStatus::Success;
        }
        case NodeType::Action:
        case NodeType::Condition: {
            return node.m_tickFn ? node.m_tickFn(ctx) : NodeStatus::Failure;
        }
        case NodeType::Inverter: {
            const NodeStatus status = tickNode(*node.m_children.front(), ctx, dt);
            if (status == NodeStatus::Success) return NodeStatus::Failure;
            if (status == NodeStatus::Failure) return NodeStatus::Success;
            return NodeStatus::Running;
        }
        case NodeType::Succeeder: {
            const NodeStatus status = tickNode(*node.m_children.front(), ctx, dt);
            return status == NodeStatus::Running ? NodeStatus::Running : NodeStatus::Success;
        }
        case NodeType::Failer: {
            const NodeStatus status = tickNode(*node.m_children.front(), ctx, dt);
            return status == NodeStatus::Running ? NodeStatus::Running : NodeStatus::Failure;
        }
        case NodeType::Cooldown: {
            if (node.m_remainingSeconds > 0.0f) {
                node.m_remainingSeconds = std::max(0.0f, node.m_remainingSeconds - dt);
                if (node.m_remainingSeconds > 0.0f) return NodeStatus::Failure;
            }
            const NodeStatus status = tickNode(*node.m_children.front(), ctx, dt);
            if (status == NodeStatus::Success) {
                node.m_remainingSeconds = node.m_durationSeconds;
            }
            return status;
        }
        case NodeType::Repeater: {
            if (node.m_repeatLimit == 0) return NodeStatus::Success;

            const NodeStatus status = tickNode(*node.m_children.front(), ctx, dt);
            if (status == NodeStatus::Running) return NodeStatus::Running;

            ++node.m_repetitions;
            if (node.m_repeatLimit >= 0 && node.m_repetitions >= node.m_repeatLimit) {
                node.m_repetitions = 0;
                return NodeStatus::Success;
            }
            return NodeStatus::Running;
        }
    }
    return NodeStatus::Failure;
}

} // namespace AI
