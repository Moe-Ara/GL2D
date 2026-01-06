//
// Created by Mohamad on 06/01/2026.
//

#ifndef THE_LOST_HEROIN_INPUT_CONTROLLER_HPP
#define THE_LOST_HEROIN_INPUT_CONTROLLER_HPP

class InputService;

class InputController {
public:
    explicit InputController(InputService* inputService);
    void update();

private:
    InputService* m_inputService{nullptr};
};

#endif // THE_LOST_HEROIN_INPUT_CONTROLLER_HPP
