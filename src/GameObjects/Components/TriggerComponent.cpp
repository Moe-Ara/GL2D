//
// TriggerComponent.cpp
//

#include "TriggerComponent.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

TriggerComponent::TriggerComponent(std::string eventId,
                                   TriggerActivationMode activation,
                                   Parameters params)
    : m_activation(activation) {
    setEventId(std::move(eventId));
    setParameters(std::move(params));
}

void TriggerComponent::setEventId(std::string eventId) {
    if (eventId.empty()) {
        throw std::invalid_argument("Trigger event id cannot be empty");
    }
    m_eventId = std::move(eventId);
}

void TriggerComponent::setParameters(Parameters params) {
    for (const auto& [name, value] : params) {
        if (name.empty() || !std::isfinite(value)) {
            throw std::invalid_argument(
                "Trigger parameters require non-empty names and finite values");
        }
    }
    m_params = std::move(params);
}

void TriggerComponent::activateManual(Entity& owner, Entity* other) const {
    if (m_activation != TriggerActivationMode::Manual) {
        throw std::logic_error(
            "activateManual requires TriggerActivationMode::Manual");
    }
    emit(owner, other);
}

void TriggerComponent::handleEnter(Entity& owner, Entity& other) const {
    if (m_activation == TriggerActivationMode::OnEnter) {
        emit(owner, &other);
    }
}

void TriggerComponent::handleStay(Entity& owner, Entity& other) const {
    if (m_activation == TriggerActivationMode::WhileInside) {
        emit(owner, &other);
    }
}

void TriggerComponent::handleExit(Entity& owner, Entity& other) const {
    if (m_activation == TriggerActivationMode::OnExit) {
        emit(owner, &other);
    }
}

void TriggerComponent::emit(Entity& owner, Entity* other) const {
    if (m_eventId.empty()) {
        throw std::logic_error(
            "TriggerComponent cannot activate without an event id");
    }
    if (m_callback) {
        m_callback(owner, other, *this);
    }
}
