//
// TriggerComponent.hpp
//

#ifndef GL2D_TRIGGERCOMPONENT_HPP
#define GL2D_TRIGGERCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include <glm/vec2.hpp>
#include <string>
#include <unordered_map>

enum class TriggerActivationMode {
    OnEnter,
    OnExit,
    WhileInside,
    Manual
};

class TriggerComponent : public IUpdatableComponent {
public:
    TriggerComponent() = default;
    ~TriggerComponent() override = default;

    TriggerComponent(const TriggerComponent&) = delete;
    TriggerComponent& operator=(const TriggerComponent&) = delete;
    TriggerComponent(TriggerComponent&&) = delete;
    TriggerComponent& operator=(TriggerComponent&&) = delete;

    void update(Entity& owner, double dt) override;

    // Basic AABB shape definition
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{0.0f, 0.0f};
    TriggerActivationMode activation{TriggerActivationMode::OnEnter};
    std::string eventId{};
    std::unordered_map<std::string, float> params{};
};

#endif //GL2D_TRIGGERCOMPONENT_HPP
