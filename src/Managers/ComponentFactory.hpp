//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_COMPONENTFACTORY_HPP
#define GL2D_COMPONENTFACTORY_HPP

#include <memory>
#include "GameObjects/Prefabs/Prefab.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/AnimatorComponent.hpp"
#include "GameObjects/Components/AnimationStateMachineComponent.hpp"
#include "GameObjects/Components/TilemapComponent.hpp"
#include "GameObjects/Components/TriggerComponent.hpp"

class ComponentFactory {
public:
    ComponentFactory() = delete;
    ~ComponentFactory() = delete;
    ComponentFactory(const ComponentFactory &other) = delete;
    ComponentFactory &operator=(const ComponentFactory &other) = delete;
    ComponentFactory(ComponentFactory &&other) = delete;
    ComponentFactory &operator=(ComponentFactory &&other) = delete;

    static std::unique_ptr<IComponent> create(const ComponentSpec& specs);
};


#endif //GL2D_COMPONENTFACTORY_HPP
