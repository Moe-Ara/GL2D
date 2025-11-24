#ifndef PLAYER_CONTROLLER_HPP
#define PLAYER_CONTROLLER_HPP

#include <glm/vec2.hpp>

#include "GameObjects/Entity.hpp"
#include "IController.hpp"
#include "InputSystem/InputService.hpp"
#include "InputSystem/InputTypes.hpp"

class PlayerController : public IController {
public:
    explicit PlayerController(InputService &inputService)
            : m_inputService(inputService) {}
    ~PlayerController() override = default;

    void update(Entity &entity, double dt) override;

private:
    void consumeActionEvent(const ActionEvent &event);

    InputService &m_inputService;
    glm::vec2 m_velocity{0.0f};
    float m_groundLevel{0.0f};
    bool m_groundInitialized{false};
    bool m_moveLeft{false};
    bool m_moveRight{false};
    bool m_jumpQueued{false};
    bool m_isGrounded{true};
    float m_moveSpeed{150.0f};
    float m_acceleration{2000.0f};
    float m_deceleration{2000.0f};
    float m_jumpImpulse{550.0f};
    float m_gravity{1800.0f};
};

#endif
