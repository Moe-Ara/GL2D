//
// Created by Mohamad on 18/11/2025.
//

#ifndef GL2D_IANIMATIONCONDITIONSINK_HPP
#define GL2D_IANIMATIONCONDITIONSINK_HPP

#include <string>

class IAnimationConditionSink{
public:
    virtual void setCondition(const std::string&,bool )=0;
    virtual void triggerEvent(const std::string&)=0;
};
#endif //GL2D_IANIMATIONCONDITIONSINK_HPP
