//
// Created by Mohamad on 18/11/2025.
//

#include "JumpCommand.hpp"

#include <utility>

#include "Graphics/Animation/IAnimationConditionSink.hpp"

JumpCommand::JumpCommand(std::string conditionName)
        : m_conditionName(std::move(conditionName)) {
}

void JumpCommand::execute(const InputContext &context) {
    if (!context.action || !context.animationSink) {
        return;
    }
    const bool pressed = context.action->eventType == InputEventType::ButtonPressed;
    const bool released = context.action->eventType == InputEventType::ButtonReleased;
    if (!(pressed || released)) {
        return;
    }
    context.animationSink->setCondition(m_conditionName, pressed);
    if (pressed) {
        context.animationSink->triggerEvent(m_conditionName);
    }
}
