//
// Created by Mohamad on 18/11/2025.
//

#include "InputEventBus.hpp"

#include <algorithm>

ListenerID InputEventBus::addListener(const std::string &action,
                                      std::function<void(const ActionEvent &)> callback) {
    if (!callback) return 0;
    const ListenerID id = m_nextId++;
    m_listeners[action].push_back({id, std::move(callback)});
    return id;
}

void InputEventBus::removeListener(ListenerID id) {
    if (id == 0) return;
    for (auto &[action, listeners]: m_listeners) {
        listeners.erase(std::remove_if(listeners.begin(), listeners.end(),
                                       [id](const ListenerEntry &entry) { return entry.id == id; }),
                        listeners.end());
    }
}

void InputEventBus::publish(const ActionEvent &actionEvent) {
    auto it = m_listeners.find(actionEvent.actionName);
    if (it == m_listeners.end()) return;
    for (auto &entry: it->second) {
        entry.callback(actionEvent);
    }
}
