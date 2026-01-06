//
// Created by Mohamad on 08/12/2025.
//

#ifndef GL2D_FEELINGSCOMPONENT_HPP
#define GL2D_FEELINGSCOMPONENT_HPP

#include <optional>

#include "GameObjects/IComponent.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"
#include "FeelingsSystem/FeelingsSystem.hpp"

class FeelingsComponent : public IUpdatableComponent {
public:
    explicit FeelingsComponent(FeelingsSystem::FeelingsSystem &feelings,
                               FeelingsSystem::FeelingSnapshot snapshot = {});
    ~FeelingsComponent() override = default;

    FeelingsComponent(const FeelingsComponent &other) = delete;
    FeelingsComponent &operator=(const FeelingsComponent &other) = delete;
    FeelingsComponent(FeelingsComponent &&other) = delete;
    FeelingsComponent &operator=(FeelingsComponent &&other) = delete;

    void update(Entity &owner, double dt) override;

    void setSnapshot(FeelingsSystem::FeelingSnapshot snapshot);
    void triggerFeeling();

private:
    FeelingsSystem::FeelingsSystem &m_feelings;
    FeelingsSystem::FeelingSnapshot m_snapshot;
    bool m_pendingApply{true};
};

#endif //GL2D_FEELINGSCOMPONENT_HPP
