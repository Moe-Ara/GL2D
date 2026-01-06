//
// Created by Mohamad on 05/12/2025.
//

#ifndef GL2D_CLIMBCONTROLLER_HPP
#define GL2D_CLIMBCONTROLLER_HPP


#include "IController.hpp"

class ClimbController : public IController {
public:
    ClimbController();

    virtual ~ClimbController();

    ClimbController(const ClimbController &other) = delete;

    ClimbController &operator=(const ClimbController &other) = delete;

    ClimbController(ClimbController &&other) = delete;

    ClimbController &operator=(ClimbController &&other) = delete;

    void update(Entity &entity, double dt) override;

private:
};


#endif //GL2D_CLIMBCONTROLLER_HPP
