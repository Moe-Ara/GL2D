#include "InputBindingsLoader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <GLFW/glfw3.h>

namespace InputSystem {

    namespace {

        std::string toLower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        std::unordered_map<std::string, int> buildKeyboardMap() {
            std::unordered_map<std::string, int> table;
            table["KEY_SPACE"] = GLFW_KEY_SPACE;
            table["KEY_A"] = GLFW_KEY_A;
            table["KEY_D"] = GLFW_KEY_D;
            table["KEY_W"] = GLFW_KEY_W;
            table["KEY_S"] = GLFW_KEY_S;
            table["KEY_Q"] = GLFW_KEY_Q;
            table["KEY_E"] = GLFW_KEY_E;
            table["KEY_Z"] = GLFW_KEY_Z;
            table["KEY_X"] = GLFW_KEY_X;
            table["KEY_C"] = GLFW_KEY_C;
            table["KEY_V"] = GLFW_KEY_V;
            table["KEY_B"] = GLFW_KEY_B;
            table["KEY_F"] = GLFW_KEY_F;
            table["KEY_R"] = GLFW_KEY_R;
            table["KEY_T"] = GLFW_KEY_T;
            table["KEY_Y"] = GLFW_KEY_Y;
            table["KEY_U"] = GLFW_KEY_U;
            table["KEY_I"] = GLFW_KEY_I;
            table["KEY_O"] = GLFW_KEY_O;
            table["KEY_P"] = GLFW_KEY_P;
            table["KEY_L"] = GLFW_KEY_L;
            table["KEY_K"] = GLFW_KEY_K;
            table["KEY_J"] = GLFW_KEY_J;
            table["KEY_H"] = GLFW_KEY_H;
            table["KEY_G"] = GLFW_KEY_G;
            table["KEY_1"] = GLFW_KEY_1;
            table["KEY_2"] = GLFW_KEY_2;
            table["KEY_3"] = GLFW_KEY_3;
            table["KEY_4"] = GLFW_KEY_4;
            table["KEY_5"] = GLFW_KEY_5;
            table["KEY_6"] = GLFW_KEY_6;
            table["KEY_7"] = GLFW_KEY_7;
            table["KEY_8"] = GLFW_KEY_8;
            table["KEY_9"] = GLFW_KEY_9;
            table["KEY_0"] = GLFW_KEY_0;
            table["KEY_ENTER"] = GLFW_KEY_ENTER;
            table["KEY_ESCAPE"] = GLFW_KEY_ESCAPE;
            table["KEY_TAB"] = GLFW_KEY_TAB;
            table["KEY_LEFT_SHIFT"] = GLFW_KEY_LEFT_SHIFT;
            table["KEY_RIGHT_SHIFT"] = GLFW_KEY_RIGHT_SHIFT;
            table["KEY_LEFT_CTRL"] = GLFW_KEY_LEFT_CONTROL;
            table["KEY_RIGHT_CTRL"] = GLFW_KEY_RIGHT_CONTROL;
            table["KEY_LEFT_ALT"] = GLFW_KEY_LEFT_ALT;
            table["KEY_RIGHT_ALT"] = GLFW_KEY_RIGHT_ALT;
            table["KEY_LEFT"] = GLFW_KEY_LEFT;
            table["KEY_RIGHT"] = GLFW_KEY_RIGHT;
            table["KEY_UP"] = GLFW_KEY_UP;
            table["KEY_DOWN"] = GLFW_KEY_DOWN;
            return table;
        }

        std::unordered_map<std::string, int> buildMouseMap() {
            std::unordered_map<std::string, int> table;
            table["BUTTON_LEFT"] = GLFW_MOUSE_BUTTON_LEFT;
            table["BUTTON_RIGHT"] = GLFW_MOUSE_BUTTON_RIGHT;
            table["BUTTON_MIDDLE"] = GLFW_MOUSE_BUTTON_MIDDLE;
            table["BUTTON_4"] = GLFW_MOUSE_BUTTON_4;
            table["BUTTON_5"] = GLFW_MOUSE_BUTTON_5;
            table["BUTTON_6"] = GLFW_MOUSE_BUTTON_6;
            table["BUTTON_7"] = GLFW_MOUSE_BUTTON_7;
            table["BUTTON_8"] = GLFW_MOUSE_BUTTON_8;
            table["SCROLL"] = 1; // matches InputService scroll controlId
            table["CURSOR"] = 0; // matches InputService cursor movement controlId
            return table;
        }

        std::unordered_map<std::string, int> buildGamepadButtonMap() {
            std::unordered_map<std::string, int> table;
            table["BUTTON_A"] = GLFW_GAMEPAD_BUTTON_A;
            table["BUTTON_B"] = GLFW_GAMEPAD_BUTTON_B;
            table["BUTTON_X"] = GLFW_GAMEPAD_BUTTON_X;
            table["BUTTON_Y"] = GLFW_GAMEPAD_BUTTON_Y;
            table["BUTTON_LEFT_BUMPER"] = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
            table["BUTTON_RIGHT_BUMPER"] = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
            table["BUTTON_BACK"] = GLFW_GAMEPAD_BUTTON_BACK;
            table["BUTTON_START"] = GLFW_GAMEPAD_BUTTON_START;
            table["BUTTON_GUIDE"] = GLFW_GAMEPAD_BUTTON_GUIDE;
            table["BUTTON_LEFT_THUMB"] = GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
            table["BUTTON_RIGHT_THUMB"] = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
            table["BUTTON_DPAD_UP"] = GLFW_GAMEPAD_BUTTON_DPAD_UP;
            table["BUTTON_DPAD_RIGHT"] = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
            table["BUTTON_DPAD_DOWN"] = GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
            table["BUTTON_DPAD_LEFT"] = GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
            return table;
        }

        std::unordered_map<std::string, int> buildGamepadAxisMap() {
            std::unordered_map<std::string, int> table;
            table["AXIS_LEFT_X"] = GLFW_GAMEPAD_AXIS_LEFT_X;
            table["AXIS_LEFT_Y"] = GLFW_GAMEPAD_AXIS_LEFT_Y;
            table["AXIS_RIGHT_X"] = GLFW_GAMEPAD_AXIS_RIGHT_X;
            table["AXIS_RIGHT_Y"] = GLFW_GAMEPAD_AXIS_RIGHT_Y;
            table["AXIS_LEFT_TRIGGER"] = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER;
            table["AXIS_RIGHT_TRIGGER"] = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;
            return table;
        }

        const std::unordered_map<std::string, int> &keyboardMap() {
            static const auto table = buildKeyboardMap();
            return table;
        }

        const std::unordered_map<std::string, int> &mouseMap() {
            static const auto table = buildMouseMap();
            return table;
        }

        const std::unordered_map<std::string, int> &gamepadButtonMap() {
            static const auto table = buildGamepadButtonMap();
            return table;
        }

        const std::unordered_map<std::string, int> &gamepadAxisMap() {
            static const auto table = buildGamepadAxisMap();
            return table;
        }

    } // namespace

    InputBindings InputBindingsLoader::loadFromFile(const std::string &filepath) {
        std::ifstream input(filepath);
        if (!input.is_open()) {
            throw Utils::JsonParseException("Failed to open input bindings file: " + filepath);
        }
        std::stringstream buffer;
        buffer << input.rdbuf();
        auto root = Utils::JsonValue::parse(buffer.str());
        return parseDocument(root);
    }

    InputBindings InputBindingsLoader::parseDocument(const Utils::JsonValue &root) {
        if (!root.isObject()) {
            throw Utils::JsonParseException("Input bindings root must be an object");
        }
        InputBindings bindings;
        if (!root.hasKey("actions")) {
            throw Utils::JsonParseException("Input bindings missing 'actions' array");
        }
        for (const auto &actionNode: root.at("actions").asArray()) {
            bindings.actions.push_back(parseAction(actionNode));
        }

        if (root.hasKey("commands")) {
            for (const auto &commandNode: root.at("commands").asArray()) {
                bindings.commands.push_back(parseCommand(commandNode));
            }
        }

        if (!root.hasKey("profiles")) {
            throw Utils::JsonParseException("Input bindings missing 'profiles' array");
        }
        for (const auto &profileNode: root.at("profiles").asArray()) {
            bindings.profiles.push_back(parseProfile(profileNode));
        }
        return bindings;
    }

    ActionDefinition InputBindingsLoader::parseAction(const Utils::JsonValue &node) {
        if (!node.isObject()) {
            throw Utils::JsonParseException("Action entry must be an object");
        }
        ActionDefinition action{};
        if (!node.hasKey("name")) {
            throw Utils::JsonParseException("Action entry missing 'name'");
        }
        action.name = node.at("name").asString();
        if (node.hasKey("type")) {
            action.type = node.at("type").asString();
        } else {
            action.type = "button";
        }
        if (node.hasKey("description")) {
            action.description = node.at("description").asString();
        }
        if (node.hasKey("animationCondition")) {
            action.animationCondition = node.at("animationCondition").asString();
        }
        return action;
    }

    CommandDefinition InputBindingsLoader::parseCommand(const Utils::JsonValue &node) {
        if (!node.isObject()) {
            throw Utils::JsonParseException("Command entry must be an object");
        }
        CommandDefinition command{};
        if (!node.hasKey("name")) {
            throw Utils::JsonParseException("Command entry missing 'name'");
        }
        if (!node.hasKey("action")) {
            throw Utils::JsonParseException("Command entry missing 'action'");
        }
        command.name = node.at("name").asString();
        command.action = node.at("action").asString();
        if (node.hasKey("params")) {
            const auto &paramNode = node.at("params");
            if (!paramNode.isObject()) {
                throw Utils::JsonParseException("Command 'params' must be an object");
            }
            for (const auto &entry: paramNode.asObject()) {
                command.parameters[entry.first] = entry.second.asString();
            }
        }
        return command;
    }

    ProfileDefinition InputBindingsLoader::parseProfile(const Utils::JsonValue &node) {
        if (!node.isObject()) {
            throw Utils::JsonParseException("Profile entry must be an object");
        }
        ProfileDefinition profile{};
        if (!node.hasKey("name")) {
            throw Utils::JsonParseException("Profile entry missing 'name'");
        }
        profile.name = node.at("name").asString();
        if (!node.hasKey("bindings")) {
            throw Utils::JsonParseException("Profile '" + profile.name + "' missing 'bindings'");
        }
        for (const auto &bindingNode: node.at("bindings").asArray()) {
            profile.bindings.push_back(parseBinding(bindingNode));
        }
        return profile;
    }

    ControlBinding InputBindingsLoader::parseBinding(const Utils::JsonValue &node) {
        if (!node.isObject()) {
            throw Utils::JsonParseException("Binding entry must be an object");
        }
        ControlBinding binding{};
        if (!node.hasKey("device") || !node.hasKey("control") || !node.hasKey("action")) {
            throw Utils::JsonParseException("Binding entry must contain 'device', 'control', and 'action'");
        }
        binding.controlToken = node.at("control").asString();
        binding.actionName = node.at("action").asString();
        auto deviceToken = node.at("device").asString();
        binding.deviceType = parseDeviceType(deviceToken);
        if (binding.deviceType == InputDeviceType::Unknown) {
            throw Utils::JsonParseException("Unknown device type '" + deviceToken + "' in binding");
        }
        binding.controlId = resolveControlId(binding.deviceType, binding.controlToken, binding.isAxis);
        return binding;
    }

    InputDeviceType InputBindingsLoader::parseDeviceType(const std::string &deviceToken) {
        auto lower = toLower(deviceToken);
        if (lower == "keyboard") return InputDeviceType::Keyboard;
        if (lower == "mouse") return InputDeviceType::Mouse;
        if (lower == "gamepad" || lower == "controller") return InputDeviceType::Gamepad;
        if (lower == "touch") return InputDeviceType::Touch;
        return InputDeviceType::Unknown;
    }

    int InputBindingsLoader::resolveControlId(InputDeviceType deviceType, const std::string &controlToken, bool& isAxisOut) {
        isAxisOut = false;
        switch (deviceType) {
            case InputDeviceType::Keyboard: {
                auto it = keyboardMap().find(controlToken);
                if (it != keyboardMap().end()) return it->second;
                break;
            }
            case InputDeviceType::Mouse: {
                auto it = mouseMap().find(controlToken);
                if (it != mouseMap().end()) return it->second;
                break;
            }
            case InputDeviceType::Gamepad: {
                auto buttonIt = gamepadButtonMap().find(controlToken);
                if (buttonIt != gamepadButtonMap().end()) return buttonIt->second;
                auto axisIt = gamepadAxisMap().find(controlToken);
                if (axisIt != gamepadAxisMap().end()) {
                    isAxisOut = true;
                    return axisIt->second;
                }
                break;
            }
            default:
                break;
        }
        throw Utils::JsonParseException("Unknown control token '" + controlToken + "'");
    }

} // namespace InputSystem
