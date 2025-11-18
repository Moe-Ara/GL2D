//
// Created by Mohamad on 18/11/2025.
//

#ifndef GL2D_JUMPCOMMAND_HPP
#define GL2D_JUMPCOMMAND_HPP


#include <string>

#include "InputSystem/IInputCommand.hpp"

class JumpCommand : public IInputCommand {
public:
    explicit JumpCommand(std::string conditionName);
    ~JumpCommand() override = default;

    JumpCommand(const JumpCommand &other) = delete;
    JumpCommand &operator=(const JumpCommand &other) = delete;
    JumpCommand(JumpCommand &&other) = delete;
    JumpCommand &operator=(JumpCommand &&other) = delete;

    void execute(const InputContext &context) override;

private:
    std::string m_conditionName;
};


#endif //GL2D_JUMPCOMMAND_HPP
