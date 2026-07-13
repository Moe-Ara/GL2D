// GameObjects/Entity.hpp
#ifndef GL2D_ENTITY_HPP
#define GL2D_ENTITY_HPP

#include <vector>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <atomic>
#include <cstdint>
#include <stdexcept>
#include "GameObjects/IComponent.hpp"

class Entity {
public:
    Entity();

    virtual ~Entity() = default;

    Entity(const Entity &) = delete;

    Entity &operator=(const Entity &) = delete;

    Entity(Entity &&) = delete;

    Entity &operator=(Entity &&) = delete;


    template<typename T, typename... Args>
    T &addComponent(Args &&... args);

    template<typename T>
    T *getComponent();

    template<typename T>
    const T *getComponent() const;

    void addComponent(std::unique_ptr<IComponent> component);

    [[nodiscard]] const std::vector<std::unique_ptr<IComponent>> &components() const;


    virtual void update(double dt);

    uint64_t getId() const { return m_id; }

private:
    template<typename T>
    T *lookupComponent() const;

    std::vector<std::unique_ptr<IComponent>> m_components{};
    uint64_t m_id{0};
    static std::atomic<uint64_t> s_nextId;
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
T *Entity::lookupComponent() const {
    static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");
    // Components are almost always stored as their concrete type, so an exact
    // typeid compare (a pointer compare on the common ABI) resolves the hot
    // path without dynamic_cast. The polymorphic scan remains as a fallback
    // for callers querying through a base class.
    for (const auto &component : m_components) {
        IComponent *raw = component.get();
        if (typeid(*raw) == typeid(T)) {
            return static_cast<T *>(raw);
        }
    }
    for (const auto &component : m_components) {
        if (auto *casted = dynamic_cast<T *>(component.get())) {
            return casted;
        }
    }
    return nullptr;
}

template<typename T>
T *Entity::getComponent() {
    return lookupComponent<T>();
}

template<typename T>
const T *Entity::getComponent() const {
    return lookupComponent<T>();
}

inline void Entity::addComponent(std::unique_ptr<IComponent> component) {
    if (!component) {
        throw std::invalid_argument("Entity::addComponent requires a non-null component");
    }
    m_components.push_back(std::move(component));
}

#endif // GL2D_ENTITY_HPP
