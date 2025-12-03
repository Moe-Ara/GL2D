#pragma once

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeSelector() {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Selector;
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeSequence() {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Sequence;
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeAction(std::function<NodeStatus(TContext*)> fn) {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Action;
    n->tickFn = std::move(fn);
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeCondition(std::function<bool(TContext*)> fn) {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Condition;
    n->tickFn = [fn = std::move(fn)](TContext* ctx) {
        return fn(ctx) ? NodeStatus::Success : NodeStatus::Failure;
    };
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeInverter(std::unique_ptr<Node> child) {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Action;
    n->children.push_back(std::move(child));
    n->tickFn = [](TContext* ctx) { return NodeStatus::Failure; }; // placeholder; logic in tick
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeSucceeder(std::unique_ptr<Node> child) {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Action;
    n->children.push_back(std::move(child));
    n->tickFn = [](TContext* ctx) { return NodeStatus::Success; };
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeFailer(std::unique_ptr<Node> child) {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Action;
    n->children.push_back(std::move(child));
    n->tickFn = [](TContext* ctx) { return NodeStatus::Failure; };
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeCooldown(std::unique_ptr<Node> child, float seconds) {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Action;
    n->children.push_back(std::move(child));
    n->tickFn = [seconds](TContext* /*ctx*/) {
        // Actual timing handled in tickNode using m_cooldowns.
        return NodeStatus::Running;
    };
    // Abuse runningChild to store desired cooldown in seconds when leaf tickFn is executed.
    n->runningChild = static_cast<int>(seconds * 1000.0f); // store milliseconds as int.
    return n;
}

template<typename TContext>
std::unique_ptr<typename BehaviourTree<TContext>::Node> BehaviourTree<TContext>::makeRepeater(std::unique_ptr<Node> child, int count) {
    auto n = std::make_unique<Node>();
    n->type = NodeType::Action;
    n->children.push_back(std::move(child));
    n->tickFn = [count](TContext* ctx) {
        return NodeStatus::Failure;
    };
    return n;
}

template<typename TContext>
NodeStatus BehaviourTree<TContext>::tick(TContext* ctx, float dt) {
    if (!m_root) return NodeStatus::Failure;
    return tickNode(*m_root, ctx, dt);
}

template<typename TContext>
void BehaviourTree<TContext>::reset() {
    std::function<void(Node&)> walk = [&](Node& n) {
        n.runningChild = -1;
        for (auto& c : n.children) {
            if (c) walk(*c);
        }
    };
    if (m_root) walk(*m_root);
}

template<typename TContext>
NodeStatus BehaviourTree<TContext>::tickNode(Node& node, TContext* ctx, float dt) {
    switch (node.type) {
        case NodeType::Selector: {
            int startIdx = std::max(0, node.runningChild);
            node.runningChild = -1;
            for (int i = startIdx; i < static_cast<int>(node.children.size()); ++i) {
                auto& child = node.children[i];
                if (!child) continue;
                const NodeStatus s = tickNode(*child, ctx, dt);
                if (s == NodeStatus::Success) return s;
                if (s == NodeStatus::Running) {
                    node.runningChild = i;
                    return s;
                }
            }
            return NodeStatus::Failure;
        }
        case NodeType::Sequence: {
            int startIdx = std::max(0, node.runningChild);
            node.runningChild = -1;
            for (int i = startIdx; i < static_cast<int>(node.children.size()); ++i) {
                auto& child = node.children[i];
                if (!child) continue;
                const NodeStatus s = tickNode(*child, ctx, dt);
                if (s == NodeStatus::Failure) return s;
                if (s == NodeStatus::Running) {
                    node.runningChild = i;
                    return s;
                }
            }
            return NodeStatus::Success;
        }
        case NodeType::Action:
        case NodeType::Condition: {
            if (node.tickFn) {
                // Cooldown decorator: node has a single child and tickFn returned Running, and runningChild encodes cooldown ms.
                if (!node.children.empty() && node.children[0] && node.tickFn(ctx) == NodeStatus::Running) {
                    const float cooldownSeconds = static_cast<float>(node.runningChild) / 1000.0f;
                    float& timer = m_cooldowns[&node];
                    if (timer > 0.0f) {
                        timer = std::max(0.0f, timer - dt);
                        if (timer > 0.0f) {
                            return NodeStatus::Failure;
                        }
                        // timer expired this tick; fall through and attempt child.
                    }
                    // Attempt child
                    auto childStatus = tickNode(*node.children[0], ctx, dt);
                    if (childStatus == NodeStatus::Success) {
                        timer = cooldownSeconds;
                        return NodeStatus::Success;
                    }
                    return childStatus;
                }
                return node.tickFn(ctx);
            }
            return NodeStatus::Failure;
        }
        default:
            return NodeStatus::Failure;
    }
}
