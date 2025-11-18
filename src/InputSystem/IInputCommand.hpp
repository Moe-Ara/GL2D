//
// Created by Mohamad on 18/11/2025.
//

#ifndef GL2D_IINPUTCOMMAND_HPP
#define GL2D_IINPUTCOMMAND_HPP

#include "InputTypes.hpp"

class IInputCommand{
public:
   virtual ~IInputCommand() = default;
   virtual void execute(const InputContext&)=0;
};
#endif //GL2D_IINPUTCOMMAND_HPP
