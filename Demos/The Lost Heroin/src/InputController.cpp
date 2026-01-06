//
// Created by Mohamad on 06/01/2026.
//

#include "InputController.hpp"

#include "Debug/DebugOverlay.hpp"
#include "InputSystem/InputService.hpp"
#include "InputSystem/InputTypes.hpp"

InputController::InputController(InputService* inputService)
    : m_inputService(inputService) {}

void InputController::update() {
    if (!m_inputService) {
        return;
    }
    m_inputService->update();
    m_inputService->pollEvents();
    for (const auto &evt : m_inputService->getActionEvents()) {
        if (evt.eventType == InputEventType::ButtonPressed &&
            evt.actionName == "ToggleDebugGizmos") {
            DebugOverlay::toggle();
        }
    }
}
