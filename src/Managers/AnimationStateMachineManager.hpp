//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_ANIMATIONSTATEMACHINEMANAGER_HPP
#define GL2D_ANIMATIONSTATEMACHINEMANAGER_HPP


#include <unordered_map>
#include "Graphics/Animation/AnimationStateMachine.hpp"

class AnimationStateMachineManager {
public:
    AnimationStateMachineManager()=delete;

    virtual ~AnimationStateMachineManager()=delete;

    AnimationStateMachineManager(const AnimationStateMachineManager &other) = delete;

    AnimationStateMachineManager &operator=(const AnimationStateMachineManager &other) = delete;

    AnimationStateMachineManager(AnimationStateMachineManager &&other) = delete;

    AnimationStateMachineManager &operator=(AnimationStateMachineManager &&other) = delete;
    static void registerAnimationSM(const std::string& id, Graphics::AnimationStateMachine* animSM);
    static Graphics::AnimationStateMachine* get(const std::string& id);
    static bool contains(const std::string& id);
private:
    static std::unordered_map<std::string,Graphics::AnimationStateMachine*> m_AnimationSMs;
};


#endif //GL2D_ANIMATIONSTATEMACHINEMANAGER_HPP
