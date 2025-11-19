//
// Created by Mohamad on 19/11/2025.
//

#ifndef GL2D_ENTITY_HPP
#define GL2D_ENTITY_HPP

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "GameObjects/Component.hpp"
#include "GameObjects/Sprite.hpp"
#include "Graphics/Animation/AnimationStateMachine.hpp"
#include "Graphics/Animation/Animator.hpp"
#include "Utils/EntityAttributes.hpp"

class IController;

class Entity {
public:
    Entity(EntityAttributes attributes, GameObjects::Sprite *sprite,
           Graphics::Animator *animator = nullptr,
           Graphics::AnimationStateMachine *animSM = nullptr)
            : m_attributes(attributes), m_sprite(sprite), m_animator(animator),
              m_animSM(animSM) {}

    virtual ~Entity();

    Entity(const Entity &other) = delete;
    Entity &operator=(const Entity &other) = delete;
    Entity(Entity &&other) = delete;
    Entity &operator=(Entity &&other) = delete;

    void setController(IController *controller);
    IController *getController() const;
    GameObjects::Sprite *getSprite() const;
    Graphics::Animator *getAnimator() const;
    Graphics::AnimationStateMachine *getAnimationSM() const;
    const EntityAttributes &getAttributes() const;
    EntityAttributes &getAttributes();

    template<typename TComponent, typename... TArgs>
    TComponent &addComponent(TArgs &&...args) {
        static_assert(std::is_base_of<IComponent, TComponent>::value,
                      "TComponent must derive from IComponent");
        auto component = std::make_unique<TComponent>(std::forward<TArgs>(args)...);
        auto *ptr = component.get();
        m_components[std::type_index(typeid(TComponent))] = std::move(component);
        return *ptr;
    }

    template<typename TComponent>
    TComponent *getComponent() {
        static_assert(std::is_base_of<IComponent, TComponent>::value,
                      "TComponent must derive from IComponent");
        auto it = m_components.find(std::type_index(typeid(TComponent)));
        if (it == m_components.end()) {
            return nullptr;
        }
        return static_cast<TComponent *>(it->second.get());
    }

    template<typename TComponent>
    const TComponent *getComponent() const {
        static_assert(std::is_base_of<IComponent, TComponent>::value,
                      "TComponent must derive from IComponent");
        auto it = m_components.find(std::type_index(typeid(TComponent)));
        if (it == m_components.end()) {
            return nullptr;
        }
        return static_cast<const TComponent *>(it->second.get());
    }

    template<typename TComponent>
    bool hasComponent() const {
        static_assert(std::is_base_of<IComponent, TComponent>::value,
                      "TComponent must derive from IComponent");
        return m_components.find(std::type_index(typeid(TComponent))) != m_components.end();
    }

    virtual void update(double deltaTime);
    void tickComponents(double deltaTime);
    virtual void render();

private:
    GameObjects::Sprite *m_sprite = nullptr;
    Graphics::Animator *m_animator = nullptr;
    Graphics::AnimationStateMachine *m_animSM = nullptr;
    IController *m_controller = nullptr;
    EntityAttributes m_attributes{};
    std::unordered_map<std::type_index, std::unique_ptr<IComponent>> m_components;
};

#endif // GL2D_ENTITY_HPP
