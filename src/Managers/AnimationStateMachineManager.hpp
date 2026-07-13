//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_ANIMATIONSTATEMACHINEMANAGER_HPP
#define GL2D_ANIMATIONSTATEMACHINEMANAGER_HPP


#include "Graphics/Animation/AnimationStateMachine.hpp"

#include <memory>
#include <string>
#include <unordered_map>

class AnimationStateMachineManager {
public:
    AnimationStateMachineManager()=delete;

    virtual ~AnimationStateMachineManager()=delete;

    AnimationStateMachineManager(const AnimationStateMachineManager &other) = delete;

    AnimationStateMachineManager &operator=(const AnimationStateMachineManager &other) = delete;

    AnimationStateMachineManager(AnimationStateMachineManager &&other) = delete;

    AnimationStateMachineManager &operator=(AnimationStateMachineManager &&other) = delete;
    static void registerStateMachine(
        const std::string& id,
        const std::shared_ptr<Graphics::AnimationStateMachine>& stateMachine);
    static std::shared_ptr<Graphics::AnimationStateMachine> get(const std::string& id);
    static bool contains(const std::string& id);
    static bool unregisterStateMachine(const std::string& id);
    static void clear() noexcept;
private:
    static std::unordered_map<
        std::string, std::weak_ptr<Graphics::AnimationStateMachine>> s_stateMachines;
};


#endif //GL2D_ANIMATIONSTATEMACHINEMANAGER_HPP
