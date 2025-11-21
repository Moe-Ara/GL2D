// GameObjects/Entity.hpp
#ifndef GL2D_ENTITY_HPP
#define GL2D_ENTITY_HPP

#include <vector>
#include <memory>
#include <type_traits>
#include "GameObjects/IComponent.hpp"

class Entity {
public:
    Entity() = default;

    ~Entity() = default;

    Entity(const Entity &) = delete;

    Entity &operator=(const Entity &) = delete;

    Entity(Entity &&) = delete;

    Entity &operator=(Entity &&) = delete;


    template<typename T, typename... Args>
    T &addComponent(Args &&... args);

    template<typename T>
    T *getComponent();

    void addComponent(std::unique_ptr<IComponent> component);

    [[nodiscard]] const std::vector<std::unique_ptr<IComponent>> &components() const;


    void update(double dt);

private:
    std::vector<std::unique_ptr<IComponent>> m_components{};
};

template<typename T, typename... Args>
T &Entity::addComponent(Args &&... args) {
    static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");
    auto comp = std::make_unique<T>(std::forward<Args>(args)...);
    T &ref = *comp;
    m_components.push_back(std::move(comp));
    return ref;
}

template<typename T>
T *Entity::getComponent() {
    static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");
    for (auto &c: m_components) {
        if (auto *casted = dynamic_cast<T *>(c.get())) {
            return casted;
        }
    }
    return nullptr;
}

inline void Entity::addComponent(std::unique_ptr<IComponent> component) {
    m_components.push_back(std::move(component));
}

#endif // GL2D_ENTITY_HPP
