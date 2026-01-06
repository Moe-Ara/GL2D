// GameObjects/Entity.hpp
#ifndef GL2D_ENTITY_HPP
#define GL2D_ENTITY_HPP

#include <vector>
#include <memory>
#include <type_traits>
#include <atomic>
#include <cstdint>
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

    void addComponent(std::unique_ptr<IComponent> component);

    [[nodiscard]] const std::vector<std::unique_ptr<IComponent>> &components() const;


    virtual void update(double dt);

    uint64_t getId() const { return m_id; }

private:
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
