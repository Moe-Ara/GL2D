//
// PrefabFactory.hpp
//

#ifndef GL2D_PREFABFACTORY_HPP
#define GL2D_PREFABFACTORY_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <glm/vec2.hpp>
#include "Prefab.hpp"
#include "GameObjects/Entity.hpp"

class PrefabFactory {
public:
    PrefabFactory() = delete;
    ~PrefabFactory() = delete;
    PrefabFactory(const PrefabFactory&) = delete;
    PrefabFactory& operator=(const PrefabFactory&) = delete;
    PrefabFactory(PrefabFactory&&) = delete;
    PrefabFactory& operator=(PrefabFactory&&) = delete;

    // Instantiate a prefab definition with optional overrides and transform.
    static std::unique_ptr<Entity> instantiate(const Prefab& prefab,
                                               const glm::vec2& position = {0.0f, 0.0f},
                                               const glm::vec2& scale = {1.0f, 1.0f},
                                               float rotation = 0.0f,
                                               const std::unordered_map<std::string, ComponentSpec>& overrides = {});

    // Convenience overload: lookup by id through PrefabCatalouge; returns nullptr if missing.
    static std::unique_ptr<Entity> instantiate(const std::string& prefabId,
                                               const glm::vec2& position = {0.0f, 0.0f},
                                               const glm::vec2& scale = {1.0f, 1.0f},
                                               float rotation = 0.0f,
                                               const std::unordered_map<std::string, ComponentSpec>& overrides = {});
};

#endif //GL2D_PREFABFACTORY_HPP
