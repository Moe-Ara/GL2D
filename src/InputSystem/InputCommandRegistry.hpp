#ifndef GL2D_INPUTCOMMANDREGISTRY_HPP
#define GL2D_INPUTCOMMANDREGISTRY_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "IInputCommand.hpp"

class InputCommandRegistry {
public:
    InputCommandRegistry() = default;
    ~InputCommandRegistry() = default;

    void registerCommand(const std::string& actionName, std::shared_ptr<IInputCommand> command);
    void clearCommands(const std::string& actionName);
    void clearAll();
    void dispatch(const std::vector<ActionEvent>& events, InputContext baseContext) const;

private:
    std::unordered_map<std::string, std::vector<std::shared_ptr<IInputCommand>>> m_commands;
};

#endif //GL2D_INPUTCOMMANDREGISTRY_HPP
