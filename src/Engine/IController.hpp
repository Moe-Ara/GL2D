//
// Created by Mohamad on 19/11/2025.
//

#ifndef GL2D_ICONTROLLER_HPP
#define GL2D_ICONTROLLER_HPP

#include "GameObjects/Entity.hpp"

class IController {
public:
  virtual ~IController() = default;
  virtual void update(Entity &entity, double dt) = 0;
};
#endif // GL2D_ICONTROLLER_HPP
