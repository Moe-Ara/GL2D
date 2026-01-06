#ifndef PLAYER_CONTROLLER_HPP
#define PLAYER_CONTROLLER_HPP

#include <glm/vec2.hpp>

#include "GameObjects/Entity.hpp"
#include "CharacterController.hpp"
#include "InputSystem/InputService.hpp"
#include "InputSystem/InputTypes.hpp"

class PlayerController : public CharacterController {
public:
    explicit PlayerController(InputService &inputService)
            : m_inputService(inputService) {}
    ~PlayerController() override = default;

    void resetInputState();

private:
    Intent gatherIntent(Entity& entity, double dt) override;
    void consumeActionEvent(const ActionEvent &event);

    InputService &m_inputService;
    bool m_moveLeft{false};
    bool m_moveRight{false};
    bool m_jumpQueued{false};
    float m_axisX{0.0f};
    bool m_axisUpdated{false};
    bool m_moveUp{false};
    bool m_moveDown{false};
    float m_climbAxis{0.0f};
    bool m_climbAxisUpdated{false};
};

#endif
