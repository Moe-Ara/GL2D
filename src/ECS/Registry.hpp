#pragma once

#include "ECS/Entity.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ECS {

// Owns entity identities and cache-friendly, type-separated component storage.
// Structural component changes are rejected during queries. Entity destruction is
// deferred until the outermost query completes, keeping component references valid.
class Registry {
public:
    Registry() = default;
    ~Registry() = default;

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = delete;
    Registry& operator=(Registry&&) = delete;

    [[nodiscard]] Entity create() {
        requireStructuralChangesAllowed("create an entity");

        Entity::Index index;
        if (m_freeIndices.empty()) {
            if (m_slots.size() >= Entity::invalidIndex()) {
                throw std::overflow_error("ECS entity index capacity exhausted");
            }
            index = static_cast<Entity::Index>(m_slots.size());
            m_slots.push_back(Slot{});
        } else {
            index = m_freeIndices.back();
            m_freeIndices.pop_back();
        }

        Slot& slot = m_slots[index];
        slot.alive = true;
        ++m_aliveCount;
        return {index, slot.generation};
    }

    // Returns false for stale or foreign handles. During a query, destruction is
    // queued and the entity is excluded from subsequent rows in that query.
    bool destroy(Entity entity) {
        if (!isAliveSlot(entity) || isPendingDestroy(entity)) {
            return false;
        }
        if (m_iterationDepth != 0) {
            m_pendingDestroy.push_back(entity);
            return true;
        }
        destroyImmediately(entity);
        return true;
    }

    [[nodiscard]] bool alive(Entity entity) const noexcept {
        return isAliveSlot(entity) && !isPendingDestroy(entity);
    }

    [[nodiscard]] std::size_t size() const noexcept { return m_aliveCount - m_pendingDestroy.size(); }
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    void clear() {
        requireStructuralChangesAllowed("clear the registry");
        for (auto& [_, storage] : m_storages) {
            storage->clear();
        }
        m_freeIndices.clear();
        m_freeIndices.reserve(m_slots.size());
        for (Entity::Index index = 0; index < m_slots.size(); ++index) {
            Slot& slot = m_slots[index];
            if (slot.alive) {
                slot.alive = false;
                advanceGeneration(slot);
            }
            m_freeIndices.push_back(index);
        }
        m_pendingDestroy.clear();
        m_aliveCount = 0;
    }

    template<typename Component, typename... Args>
    Component& emplace(Entity entity, Args&&... args) {
        static_assert(std::movable<Component>, "ECS components must be movable values");
        requireStructuralChangesAllowed("add a component");
        requireAlive(entity);
        auto& components = assureStorage<Component>();
        if (components.contains(entity)) {
            throw std::logic_error("Entity already owns the requested component type");
        }
        return components.emplace(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    bool remove(Entity entity) {
        requireStructuralChangesAllowed("remove a component");
        requireAlive(entity);
        auto* components = findStorage<Component>();
        return components && components->erase(entity.index());
    }

    template<typename Component>
    [[nodiscard]] bool has(Entity entity) const noexcept {
        if (!alive(entity)) {
            return false;
        }
        const auto* components = findStorage<Component>();
        return components && components->contains(entity);
    }

    template<typename Component>
    Component* tryGet(Entity entity) noexcept {
        if (!alive(entity)) {
            return nullptr;
        }
        auto* components = findStorage<Component>();
        return components ? components->tryGet(entity) : nullptr;
    }

    template<typename Component>
    const Component* tryGet(Entity entity) const noexcept {
        if (!alive(entity)) {
            return nullptr;
        }
        const auto* components = findStorage<Component>();
        return components ? components->tryGet(entity) : nullptr;
    }

    template<typename Component>
    Component& get(Entity entity) {
        if (Component* component = tryGet<Component>(entity)) {
            return *component;
        }
        throw std::out_of_range("Entity does not own the requested component type");
    }

    template<typename Component>
    const Component& get(Entity entity) const {
        if (const Component* component = tryGet<Component>(entity)) {
            return *component;
        }
        throw std::out_of_range("Entity does not own the requested component type");
    }

    template<typename First, typename... Rest, typename Function>
    void each(Function&& function) {
        const auto storages = std::tuple{findStorage<First>(), findStorage<Rest>()...};
        const bool allPresent = std::apply([](const auto*... storage) {
            return ((storage != nullptr) && ...);
        }, storages);
        if (!allPresent) {
            return;
        }
        const auto candidates = std::apply([](auto*... storage) {
            return std::array<IStorage*, sizeof...(storage)>{storage...};
        }, storages);
        const IStorage* primary = *std::ranges::min_element(
            candidates, {}, [](const IStorage* storage) { return storage->size(); });

        ++m_iterationDepth;
        try {
            for (const Entity entity : primary->entities()) {
                if (isPendingDestroy(entity)) {
                    continue;
                }
                const bool matches = std::apply([entity](const auto*... storage) {
                    return (storage->contains(entity) && ...);
                }, storages);
                if (!matches) {
                    continue;
                }
                std::apply([&](auto*... storage) {
                    std::invoke(function, entity, *storage->tryGet(entity)...);
                }, storages);
            }
        } catch (...) {
            finishIteration();
            throw;
        }
        finishIteration();
    }

private:
    struct Slot {
        Entity::Generation generation{1};
        bool alive{false};
    };

    class IStorage {
    public:
        virtual ~IStorage() = default;
        virtual bool erase(Entity::Index index) = 0;
        virtual void clear() = 0;
        [[nodiscard]] virtual std::size_t size() const noexcept = 0;
        [[nodiscard]] virtual const std::vector<Entity>& entities() const noexcept = 0;
    };

    template<typename Component>
    class Storage final : public IStorage {
    public:
        template<typename... Args>
        Component& emplace(Entity entity, Args&&... args) {
            ensureSparse(entity.index());
            m_entities.push_back(entity);
            try {
                m_components.emplace_back(std::forward<Args>(args)...);
            } catch (...) {
                m_entities.pop_back();
                throw;
            }
            m_sparse[entity.index()] = m_components.size();
            return m_components.back();
        }

        [[nodiscard]] bool contains(Entity entity) const noexcept {
            if (entity.index() >= m_sparse.size()) {
                return false;
            }
            const std::size_t packed = m_sparse[entity.index()];
            return packed != 0 && m_entities[packed - 1] == entity;
        }

        Component* tryGet(Entity entity) noexcept {
            return contains(entity) ? &m_components[m_sparse[entity.index()] - 1] : nullptr;
        }

        const Component* tryGet(Entity entity) const noexcept {
            return contains(entity) ? &m_components[m_sparse[entity.index()] - 1] : nullptr;
        }

        bool erase(Entity::Index index) override {
            if (index >= m_sparse.size() || m_sparse[index] == 0) {
                return false;
            }
            const std::size_t denseIndex = m_sparse[index] - 1;
            const std::size_t lastIndex = m_components.size() - 1;
            if (denseIndex != lastIndex) {
                m_components[denseIndex] = std::move(m_components[lastIndex]);
                m_entities[denseIndex] = m_entities[lastIndex];
                m_sparse[m_entities[denseIndex].index()] = denseIndex + 1;
            }
            m_components.pop_back();
            m_entities.pop_back();
            m_sparse[index] = 0;
            return true;
        }

        void clear() override {
            m_sparse.clear();
            m_entities.clear();
            m_components.clear();
        }

        [[nodiscard]] std::size_t size() const noexcept override { return m_entities.size(); }
        [[nodiscard]] const std::vector<Entity>& entities() const noexcept override { return m_entities; }

    private:
        void ensureSparse(Entity::Index index) {
            if (index >= m_sparse.size()) {
                m_sparse.resize(static_cast<std::size_t>(index) + 1, 0);
            }
        }

        // A zero sparse entry means absent; otherwise it stores dense index + 1.
        std::vector<std::size_t> m_sparse;
        std::vector<Entity> m_entities;
        std::vector<Component> m_components;
    };

    template<typename Component>
    Storage<Component>& assureStorage() {
        const std::type_index type = typeid(Component);
        auto [it, inserted] = m_storages.try_emplace(type);
        if (inserted) {
            it->second = std::make_unique<Storage<Component>>();
        }
        return *static_cast<Storage<Component>*>(it->second.get());
    }

    template<typename Component>
    Storage<Component>* findStorage() noexcept {
        const auto it = m_storages.find(typeid(Component));
        return it == m_storages.end() ? nullptr : static_cast<Storage<Component>*>(it->second.get());
    }

    template<typename Component>
    const Storage<Component>* findStorage() const noexcept {
        const auto it = m_storages.find(typeid(Component));
        return it == m_storages.end() ? nullptr : static_cast<const Storage<Component>*>(it->second.get());
    }

    [[nodiscard]] bool isAliveSlot(Entity entity) const noexcept {
        return entity && entity.index() < m_slots.size() &&
               m_slots[entity.index()].alive &&
               m_slots[entity.index()].generation == entity.generation();
    }

    [[nodiscard]] bool isPendingDestroy(Entity entity) const noexcept {
        if (m_pendingDestroy.empty()) {
            return false;
        }
        return std::ranges::find(m_pendingDestroy, entity) != m_pendingDestroy.end();
    }

    void requireAlive(Entity entity) const {
        if (!alive(entity)) {
            throw std::out_of_range("ECS entity handle is stale or invalid");
        }
    }

    void requireStructuralChangesAllowed(const char* operation) const {
        if (m_iterationDepth != 0) {
            throw std::logic_error(std::string("Cannot ") + operation + " during an ECS query");
        }
    }

    static void advanceGeneration(Slot& slot) noexcept {
        ++slot.generation;
        if (slot.generation == 0) {
            slot.generation = 1;
        }
    }

    void destroyImmediately(Entity entity) {
        for (auto& [_, storage] : m_storages) {
            storage->erase(entity.index());
        }
        Slot& slot = m_slots[entity.index()];
        slot.alive = false;
        advanceGeneration(slot);
        m_freeIndices.push_back(entity.index());
        --m_aliveCount;
    }

    void finishIteration() {
        --m_iterationDepth;
        if (m_iterationDepth == 0) {
            auto pending = std::move(m_pendingDestroy);
            m_pendingDestroy.clear();
            for (const Entity entity : pending) {
                if (isAliveSlot(entity)) {
                    destroyImmediately(entity);
                }
            }
        }
    }

    std::vector<Slot> m_slots;
    std::vector<Entity::Index> m_freeIndices;
    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> m_storages;
    std::vector<Entity> m_pendingDestroy;
    std::size_t m_aliveCount{0};
    std::uint32_t m_iterationDepth{0};
};

} // namespace ECS
