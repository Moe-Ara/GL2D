//
// TriggerComponent.hpp
//

#ifndef GL2D_TRIGGERCOMPONENT_HPP
#define GL2D_TRIGGERCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

enum class TriggerActivationMode {
    OnEnter,
    OnExit,
    WhileInside,
    Manual
};

// Data-driven activation attached to an entity whose ColliderComponent is a
// trigger. Geometry and overlap lifetime remain owned by the physics layer.
class TriggerComponent : public IComponent {
public:
    using Parameters = std::unordered_map<std::string, float>;
    using Callback = std::function<void(
        Entity& owner, Entity* other, const TriggerComponent& trigger)>;

    explicit TriggerComponent(
        std::string eventId,
        TriggerActivationMode activation = TriggerActivationMode::OnEnter,
        Parameters params = {});
    ~TriggerComponent() override = default;

    TriggerComponent(const TriggerComponent&) = delete;
    TriggerComponent& operator=(const TriggerComponent&) = delete;
    TriggerComponent(TriggerComponent&&) = delete;
    TriggerComponent& operator=(TriggerComponent&&) = delete;

    void setActivation(TriggerActivationMode activation) noexcept {
        m_activation = activation;
    }
    [[nodiscard]] TriggerActivationMode activation() const noexcept {
        return m_activation;
    }
    void setEventId(std::string eventId);
    [[nodiscard]] const std::string& eventId() const noexcept { return m_eventId; }
    void setParameters(Parameters params);
    [[nodiscard]] const Parameters& parameters() const noexcept { return m_params; }
    void setCallback(Callback callback) { m_callback = std::move(callback); }
    [[nodiscard]] bool hasCallback() const noexcept {
        return static_cast<bool>(m_callback);
    }

    // Manual triggers are deliberately explicit; collision-driven activation
    // calls the private enter/stay/exit hooks through ColliderComponent.
    void activateManual(Entity& owner, Entity* other = nullptr) const;

private:
    friend class ColliderComponent;
    void handleEnter(Entity& owner, Entity& other) const;
    void handleStay(Entity& owner, Entity& other) const;
    void handleExit(Entity& owner, Entity& other) const;
    void emit(Entity& owner, Entity* other) const;

    TriggerActivationMode m_activation{TriggerActivationMode::OnEnter};
    std::string m_eventId{};
    Parameters m_params{};
    Callback m_callback{};
};

#endif //GL2D_TRIGGERCOMPONENT_HPP
