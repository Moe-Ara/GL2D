//
// Created by Mohamad on 16/11/2025.
//

#include "InputService.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
    constexpr int kKeyboardDeviceId = -1;
    constexpr int kMouseDeviceId = -2;
    constexpr float kAxisEpsilon = 0.001f;
}

InputService *InputService::s_activeService = nullptr;

InputService::InputService() = default;

InputService::~InputService() {
    if (s_activeService == this) {
        s_activeService = nullptr;
    }
}

void InputService::initialize(GLFWwindow *window) {
    if (!window) {
        throw std::runtime_error("InputService::initialize received a null GLFWwindow");
    }
    m_window = window;
    s_activeService = this;
    glfwSetWindowUserPointer(window, this);

    registerBuiltinDevices();

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetJoystickCallback(joystickCallback);

    detectConnectedGamepads();
}

void InputService::update() {
    detectConnectedGamepads();

    const double timestamp = glfwGetTime();
    for (const auto &[deviceId, device]: m_devices) {
        if (device.type != InputDeviceType::Gamepad || !device.connected) {
            continue;
        }
        if (!glfwJoystickIsGamepad(deviceId)) {
            continue;
        }
        GLFWgamepadstate state{};
        if (glfwGetGamepadState(deviceId, &state) == GLFW_TRUE) {
            processGamepadState(deviceId, state, timestamp);
        }
    }
}

const std::vector<InputEvent> &InputService::pollEvents() {
    if (!m_pendingEvents.empty()) {
        m_eventBuffer.swap(m_pendingEvents);
        m_pendingEvents.clear();
    } else {
        m_eventBuffer.clear();
    }
    resolveActionEvents();
    return m_eventBuffer;
}

void InputService::flushEvents() {
    m_eventBuffer.clear();
}

const std::unordered_map<int, InputDevice> &InputService::getConnectedDevices() const {
    return m_devices;
}

void InputService::loadBindingsFromFile(const std::string &filepath, const std::string &defaultProfile) {
    setBindings(InputSystem::InputBindingsLoader::loadFromFile(filepath), defaultProfile);
}

void InputService::setBindings(InputSystem::InputBindings bindings, const std::string &defaultProfile) {
    m_bindings = std::move(bindings);
    m_hasBindings = true;
    if (!defaultProfile.empty()) {
        m_activeProfile = defaultProfile;
    } else if (m_activeProfile.empty() && !m_bindings.profiles.empty()) {
        m_activeProfile = m_bindings.profiles.front().name;
    }
    rebuildBindingLookup();
    resolveActionEvents();
}

bool InputService::setActiveProfile(const std::string &profileName) {
    if (!m_hasBindings || profileName.empty()) {
        return false;
    }
    auto profile = findProfile(profileName);
    if (!profile) {
        return false;
    }
    m_activeProfile = profileName;
    rebuildBindingLookup();
    resolveActionEvents();
    return true;
}

void InputService::registerBuiltinDevices() {
    m_devices[kKeyboardDeviceId] = InputDevice{kKeyboardDeviceId, InputDeviceType::Keyboard, "Keyboard", true};
    m_devices[kMouseDeviceId] = InputDevice{kMouseDeviceId, InputDeviceType::Mouse, "Mouse", true};
}

void InputService::detectConnectedGamepads() {
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
        const bool present = glfwJoystickPresent(jid) == GLFW_TRUE;
        const bool isGamepad = present && glfwJoystickIsGamepad(jid);
        auto it = m_devices.find(jid);
        if (isGamepad) {
            if (it == m_devices.end() || !it->second.connected) {
                handleGamepadConnected(jid);
            }
        } else {
            if (it != m_devices.end() && it->second.type == InputDeviceType::Gamepad && it->second.connected) {
                handleGamepadDisconnected(jid);
            }
        }
    }
}

void InputService::handleGamepadConnected(int jid) {
    if (!glfwJoystickIsGamepad(jid)) {
        return;
    }
    InputDevice device{};
    device.id = jid;
    device.type = InputDeviceType::Gamepad;
    const char *gamepadName = glfwGetGamepadName(jid);
    device.name = gamepadName ? gamepadName : "Gamepad";
    device.connected = true;
    m_devices[jid] = device;
    m_gamepadStates[jid] = GamepadState{};
    std::cout << "[Input] Gamepad connected: " << device.name << " (id=" << jid << ")" << std::endl;
}

void InputService::handleGamepadDisconnected(int jid) {
    auto it = m_devices.find(jid);
    if (it != m_devices.end()) {
        std::cout << "[Input] Gamepad disconnected: " << it->second.name << " (id=" << jid << ")" << std::endl;
        m_devices.erase(it);
    } else {
        std::cout << "[Input] Gamepad disconnected (id=" << jid << ")" << std::endl;
    }
    m_gamepadStates.erase(jid);
}

void InputService::processGamepadState(int jid, const GLFWgamepadstate &state, double timestamp) {
    auto &snapshot = m_gamepadStates[jid];
    if (!snapshot.initialized) {
        for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i) {
            snapshot.buttons[i] = state.buttons[i];
        }
        for (int axis = 0; axis <= GLFW_GAMEPAD_AXIS_LAST; ++axis) {
            snapshot.axes[axis] = state.axes[axis];
        }
        snapshot.initialized = true;
        return;
    }

    for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i) {
        const unsigned char previous = snapshot.buttons[i];
        const unsigned char current = state.buttons[i];
        if (previous == current) continue;

        InputEvent event{};
        event.type = current == GLFW_PRESS ? InputEventType::ButtonPressed : InputEventType::ButtonReleased;
        event.deviceType = InputDeviceType::Gamepad;
        event.deviceId = jid;
        event.controlId = i;
        event.axisValue = glm::vec2(current == GLFW_PRESS ? 1.0f : 0.0f, 0.0f);
        event.timestampSeconds = timestamp;
        enqueueEvent(event);
        snapshot.buttons[i] = current;
    }

    for (int axis = 0; axis <= GLFW_GAMEPAD_AXIS_LAST; ++axis) {
        const float previous = snapshot.axes[axis];
        const float current = state.axes[axis];
        if (std::fabs(previous - current) < kAxisEpsilon) continue;

        InputEvent event{};
        event.type = InputEventType::AxisChanged;
        event.deviceType = InputDeviceType::Gamepad;
        event.deviceId = jid;
        event.controlId = axis;
        event.axisValue = glm::vec2(current, 0.0f);
        event.timestampSeconds = timestamp;
        enqueueEvent(event);
        snapshot.axes[axis] = current;
    }
}

void InputService::enqueueEvent(InputEvent event) {
    m_pendingEvents.push_back(event);
}

InputService *InputService::getServiceFromWindow(GLFWwindow *window) {
    return static_cast<InputService *>(glfwGetWindowUserPointer(window));
}

void InputService::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void) scancode;
    (void) mods;
    auto *service = getServiceFromWindow(window);
    if (!service) return;

    if (action != GLFW_PRESS && action != GLFW_RELEASE && action != GLFW_REPEAT) {
        return;
    }

    InputEvent event{};
    event.type = (action == GLFW_RELEASE) ? InputEventType::ButtonReleased : InputEventType::ButtonPressed;
    event.deviceType = InputDeviceType::Keyboard;
    event.deviceId = kKeyboardDeviceId;
    event.controlId = key;
    event.axisValue = glm::vec2(action == GLFW_RELEASE ? 0.0f : 1.0f, 0.0f);
    event.timestampSeconds = glfwGetTime();
    service->enqueueEvent(event);
}

void InputService::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    (void) mods;
    auto *service = getServiceFromWindow(window);
    if (!service) return;
    if (action != GLFW_PRESS && action != GLFW_RELEASE) {
        return;
    }

    InputEvent event{};
    event.type = (action == GLFW_RELEASE) ? InputEventType::ButtonReleased : InputEventType::ButtonPressed;
    event.deviceType = InputDeviceType::Mouse;
    event.deviceId = kMouseDeviceId;
    event.controlId = button;
    event.axisValue = glm::vec2(action == GLFW_RELEASE ? 0.0f : 1.0f, 0.0f);
    event.timestampSeconds = glfwGetTime();
    service->enqueueEvent(event);
}

void InputService::cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    auto *service = getServiceFromWindow(window);
    if (!service) return;

    glm::vec2 current(static_cast<float>(xpos), static_cast<float>(ypos));
    if (!service->m_cursorInitialized) {
        service->m_lastCursorPos = current;
        service->m_cursorInitialized = true;
        return;
    }

    glm::vec2 delta = current - service->m_lastCursorPos;
    service->m_lastCursorPos = current;
    if (delta.x == 0.0f && delta.y == 0.0f) return;

    InputEvent event{};
    event.type = InputEventType::AxisChanged;
    event.deviceType = InputDeviceType::Mouse;
    event.deviceId = kMouseDeviceId;
    event.controlId = 0; // Cursor move
    event.axisValue = delta;
    event.timestampSeconds = glfwGetTime();
    service->enqueueEvent(event);
}

void InputService::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    auto *service = getServiceFromWindow(window);
    if (!service) return;
    InputEvent event{};
    event.type = InputEventType::AxisChanged;
    event.deviceType = InputDeviceType::Mouse;
    event.deviceId = kMouseDeviceId;
    event.controlId = 1; // Scroll wheel
    event.axisValue = glm::vec2(static_cast<float>(xoffset), static_cast<float>(yoffset));
    event.timestampSeconds = glfwGetTime();
    service->enqueueEvent(event);
}

void InputService::joystickCallback(int jid, int event) {
    auto *service = s_activeService;
    if (!service) return;
    if (event == GLFW_CONNECTED) {
        service->handleGamepadConnected(jid);
    } else if (event == GLFW_DISCONNECTED) {
        service->handleGamepadDisconnected(jid);
    }
}

void InputService::resolveActionEvents() {
    m_actionEvents.clear();
    if (!m_hasBindings || m_bindingLookup.empty()) {
        return;
    }
    for (const auto &evt: m_eventBuffer) {
        BindingKey key{evt.deviceType, evt.controlId};
        auto it = m_bindingLookup.find(key);
        if (it == m_bindingLookup.end()) continue;
        for (const auto &actionName: it->second) {
            ActionEvent actionEvt{};
            actionEvt.actionName = actionName;
            actionEvt.eventType = evt.type;
            actionEvt.value = evt.axisValue;
            actionEvt.timestampSeconds = evt.timestampSeconds;
            actionEvt.sourceEvent = evt;
            m_actionEvents.push_back(actionEvt);
        }
    }
}

void InputService::rebuildBindingLookup() {
    m_bindingLookup.clear();
    if (!m_hasBindings || m_bindings.profiles.empty()) {
        return;
    }
    if (m_activeProfile.empty() || !findProfile(m_activeProfile)) {
        m_activeProfile = m_bindings.profiles.front().name;
    }
    const auto *profile = findProfile(m_activeProfile);
    if (!profile) {
        return;
    }
    for (const auto &binding: profile->bindings) {
        BindingKey key{binding.deviceType, binding.controlId};
        m_bindingLookup[key].push_back(binding.actionName);
    }
}

const InputSystem::ProfileDefinition *
InputService::findProfile(const std::string &profileName) const {
    for (const auto &profile: m_bindings.profiles) {
        if (profile.name == profileName) {
            return &profile;
        }
    }
    return nullptr;
}
