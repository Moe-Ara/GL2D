#ifndef GL2D_INPUTBINDINGSLOADER_HPP
#define GL2D_INPUTBINDINGSLOADER_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "InputTypes.hpp"
#include "Utils/SimpleJson.hpp"

namespace InputSystem {

    struct ActionDefinition {
        std::string name;
        std::string type; // e.g., button, axis1d, axis2d
        std::string description;
        std::string animationCondition;
    };

    struct CommandDefinition {
        std::string name;
        std::string action;
        std::unordered_map<std::string, std::string> parameters;
    };

    struct ControlBinding {
        InputDeviceType deviceType{InputDeviceType::Unknown};
        int controlId{-1};                // Numeric code resolved during loading
        std::string controlToken;         // Original token from JSON (e.g., "KEY_A")
        std::string actionName;
    };

    struct ProfileDefinition {
        std::string name;
        std::vector<ControlBinding> bindings;
    };

    struct InputBindings {
        std::vector<ActionDefinition> actions;
        std::vector<CommandDefinition> commands;
        std::vector<ProfileDefinition> profiles;
    };

    class InputBindingsLoader {
    public:
        static InputBindings loadFromFile(const std::string &filepath);

    private:
        static InputBindings parseDocument(const Utils::JsonValue &root);
        static ActionDefinition parseAction(const Utils::JsonValue &node);
        static CommandDefinition parseCommand(const Utils::JsonValue &node);
        static ProfileDefinition parseProfile(const Utils::JsonValue &node);
        static ControlBinding parseBinding(const Utils::JsonValue &node);

        static InputDeviceType parseDeviceType(const std::string &deviceToken);
        static int resolveControlId(InputDeviceType deviceType, const std::string &controlToken);
    };

} // namespace InputSystem

#endif //GL2D_INPUTBINDINGSLOADER_HPP
