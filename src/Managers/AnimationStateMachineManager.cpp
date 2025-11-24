//
// Created by Mohamad on 21/11/2025.
//

#include "AnimationStateMachineManager.hpp"

std::unordered_map<std::string, Graphics::AnimationStateMachine*> AnimationStateMachineManager::m_AnimationSMs{};

void AnimationStateMachineManager::registerAnimationSM(const std::string &id, Graphics::AnimationStateMachine *animSM) {
m_AnimationSMs[id]=animSM;
}

Graphics::AnimationStateMachine *AnimationStateMachineManager::get(const std::string &id) {
    auto it= m_AnimationSMs.find(id);
    if(it!=m_AnimationSMs.end()){
    return it->second;
    }
    return nullptr;
}

bool AnimationStateMachineManager::contains(const std::string &id) {
    return m_AnimationSMs.contains(id);
}
