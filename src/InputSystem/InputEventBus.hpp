//
// Created by Mohamad on 18/11/2025.
//

#ifndef GL2D_INPUTEVENTBUS_HPP
#define GL2D_INPUTEVENTBUS_HPP


#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "InputTypes.hpp"

using ListenerID=uint64_t;
class InputEventBus {
public:

    InputEventBus()=default;

    virtual ~InputEventBus()=default;

    InputEventBus(const InputEventBus &other) = delete;

    InputEventBus &operator=(const InputEventBus &other) = delete;

    InputEventBus(InputEventBus &&other) = delete;

    InputEventBus &operator=(InputEventBus &&other) = delete;

    ListenerID addListener(const std::string& action, std::function<void(const ActionEvent&)> callback);
    void removeListener(ListenerID id);
    void publish(const ActionEvent& actionEvent);
private:
    struct ListenerEntry{
        ListenerID id;
        std::function<void(const ActionEvent&)> callback;
    };
    ListenerID m_nextId{1};
    std::unordered_map<std::string,std::vector<ListenerEntry>> m_listeners;
};


#endif //GL2D_INPUTEVENTBUS_HPP
