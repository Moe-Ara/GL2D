#ifndef GL2D_CONTROLLERCOMPONENT_HPP
#define GL2D_CONTROLLERCOMPONENT_HPP

#include <memory>

#include "GameObjects/IComponent.hpp"
#include "Engine/IController.hpp"

// Bridges an IController (Player/AI) into the component update flow.
class ControllerComponent : public IUpdatableComponent {
public:
    explicit ControllerComponent(std::unique_ptr<IController> controller)
        : m_controller(std::move(controller)) {}

    void update(Entity& owner, double dt) override {
        if (m_controller && m_enabled) {
            m_controller->update(owner, dt);
        }
    }

    IController* controller() { return m_controller.get(); }
    const IController* controller() const { return m_controller.get(); }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

private:
    std::unique_ptr<IController> m_controller;
    bool m_enabled{true};
};

#endif // GL2D_CONTROLLERCOMPONENT_HPP
