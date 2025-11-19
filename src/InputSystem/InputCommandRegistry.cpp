#include "InputCommandRegistry.hpp"

void InputCommandRegistry::registerCommand(const std::string &actionName,
                                           std::shared_ptr<IInputCommand> command) {
    if (!command) return;
    m_commands[actionName].push_back(std::move(command));
}

void InputCommandRegistry::clearCommands(const std::string &actionName) {
    m_commands.erase(actionName);
}

void InputCommandRegistry::clearAll() {
    m_commands.clear();
}

void InputCommandRegistry::dispatch(const std::vector<ActionEvent> &events,
                                    InputContext baseContext) const {
    for (const auto &event: events) {
        auto it = m_commands.find(event.actionName);
        if (it == m_commands.end()) {
            continue;
        }
        InputContext ctx = baseContext;
        ctx.action = &event;
        for (const auto &command: it->second) {
            if (command) {
                command->execute(ctx);
            }
        }
    }
}
