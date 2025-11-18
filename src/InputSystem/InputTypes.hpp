#ifndef GL2D_INPUTTYPES_HPP
#define GL2D_INPUTTYPES_HPP

#include <string>

#include <glm/vec2.hpp>

class InputService;
class IAnimationConditionSink;

enum class InputEventType {
    ButtonPressed,
    ButtonReleased,
    AxisChanged
};

enum class InputDeviceType {
    Keyboard,
    Mouse,
    Gamepad,
    Touch,
    Unknown
};

struct InputEvent {
    InputEventType type{};
    InputDeviceType deviceType{InputDeviceType::Unknown};
    int deviceId{0};
    int controlId{0};
    glm::vec2 axisValue{0.0f, 0.0f};
    double timestampSeconds{0.0};
};

struct InputDevice {
    int id{0};
    InputDeviceType type{InputDeviceType::Unknown};
    std::string name;
    bool connected{false};
};

struct ActionEvent {
    std::string actionName;
    InputEventType eventType{InputEventType::ButtonPressed};
    glm::vec2 value{0.0f, 0.0f};
    double timestampSeconds{0.0};
    InputEvent sourceEvent;
};

struct InputContext {
    const ActionEvent* action{nullptr};
    InputService* inputService{nullptr};
    IAnimationConditionSink* animationSink{nullptr};
    double deltaTime{0.0};
    void* userData{nullptr};
};
#endif //GL2D_INPUTTYPES_HPP
