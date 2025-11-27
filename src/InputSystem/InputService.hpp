//
// Created by Mohamad on 16/11/2025.
//

#ifndef GL2D_INPUTSERVICE_HPP
#define GL2D_INPUTSERVICE_HPP

#include <array>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "InputTypes.hpp"
#include "InputBindingsLoader.hpp"

class InputService {
public:
    InputService();
    ~InputService();

    void initialize(GLFWwindow* window);
    void update();

    const std::vector<InputEvent>& pollEvents();
    void flushEvents();

    const std::unordered_map<int, InputDevice>& getConnectedDevices() const;
    const std::vector<ActionEvent>& getActionEvents() const { return m_actionEvents; }

    void loadBindingsFromFile(const std::string& filepath, const std::string& defaultProfile = "");
    void setBindings(InputSystem::InputBindings bindings, const std::string& defaultProfile = "");
    bool setActiveProfile(const std::string& profileName);
    [[nodiscard]] const std::string& getActiveProfile() const { return m_activeProfile; }

    InputService(const InputService &other) = delete;

    InputService &operator=(const InputService &other) = delete;

    InputService(InputService &&other) = delete;

    InputService &operator=(InputService &&other) = delete;
private:
    struct GamepadState {
        std::array<unsigned char, GLFW_GAMEPAD_BUTTON_LAST + 1> buttons{};
        std::array<float, GLFW_GAMEPAD_AXIS_LAST + 1> axes{};
        bool initialized{false};
    };

    struct BindingKey {
        InputDeviceType deviceType{InputDeviceType::Unknown};
        int controlId{0};
        bool isAxis{false};
        bool operator==(const BindingKey& other) const {
            return deviceType == other.deviceType && controlId == other.controlId && isAxis == other.isAxis;
        }
    };

    struct BindingKeyHasher {
        std::size_t operator()(const BindingKey& key) const noexcept {
            return (static_cast<std::size_t>(key.controlId) << 4) ^
                   (static_cast<std::size_t>(key.deviceType) << 1) ^
                   static_cast<std::size_t>(key.isAxis);
        }
    };

    static void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow*, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow*, double xpos, double ypos);
    static void scrollCallback(GLFWwindow*, double xoffset, double yoffset);
    static void joystickCallback(int jid, int event);

    static InputService* getServiceFromWindow(GLFWwindow* window);
    void enqueueEvent(InputEvent event);
    void detectConnectedGamepads();
    void handleGamepadConnected(int jid);
    void handleGamepadDisconnected(int jid);
    bool hasConnectedGamepad() const;
    void processGamepadState(int jid, const GLFWgamepadstate& state, double timestamp);
    void resolveActionEvents();
    void rebuildBindingLookup();
    const InputSystem::ProfileDefinition* findProfile(const std::string& profileName) const;

    GLFWwindow* m_window{nullptr};
    std::vector<InputEvent> m_pendingEvents;
    std::vector<InputEvent> m_eventBuffer;
    std::vector<ActionEvent> m_actionEvents;
    std::unordered_map<int, InputDevice> m_devices;
    std::unordered_map<int, GamepadState> m_gamepadStates;
    glm::vec2 m_lastCursorPos{0.0f, 0.0f};
    bool m_cursorInitialized{false};
    void registerBuiltinDevices();
    static InputService* s_activeService;
    InputDeviceType m_lastProfileDevice{InputDeviceType::Unknown};

    InputSystem::InputBindings m_bindings;
    bool m_hasBindings{false};
    std::string m_activeProfile;
    std::unordered_map<BindingKey, std::vector<std::string>, BindingKeyHasher> m_bindingLookup;
};


#endif //GL2D_INPUTSERVICE_HPP
