#ifndef AI_MANAGER_HPP
#define AI_MANAGER_HPP

#include <memory>
#include <vector>

#include "AISystem/AIEntity.hpp"

// Simple scheduler that spreads AI updates over frames at a fixed tick interval.
class AIManager {
public:
    void setTickInterval(float seconds) { m_tickInterval = seconds; }

    void addEntity(std::unique_ptr<AIEntity> ai) {
        if (!ai) return;
        m_entitiesView.push_back(ai.get());
        m_nodes.push_back(Node{std::move(ai), 0.0f});
    }

    void update(float dt) {
        for (auto& n : m_nodes) {
            n.accum += dt;
            if (n.accum >= m_tickInterval) {
                n.ai->update(n.accum);
                n.accum = 0.0f;
            }
        }
    }

    const std::vector<AIEntity*>& entities() const { return m_entitiesView; }

private:
    struct Node {
        std::unique_ptr<AIEntity> ai;
        float accum{0.0f};
    };

    float m_tickInterval{0.1f};
    std::vector<Node> m_nodes;
    std::vector<AIEntity*> m_entitiesView;
};

#endif // AI_MANAGER_HPP
