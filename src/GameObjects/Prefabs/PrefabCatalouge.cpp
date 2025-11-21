//
// Created by Mohamad on 21/11/2025.
//

#include <stdexcept>
#include "PrefabCatalouge.hpp"


void PrefabCatalouge::registerPrefab(const Prefab &prefab) {
    auto& map = prefabs();
    const auto& id = prefab.id;
    auto [it, inserted] = map.emplace(id, std::move(prefab));
    if (!inserted) {
        it->second = std::move(map.at(id));
    }
}

std::unordered_map<std::string, Prefab> &PrefabCatalouge::prefabs() {
    static std::unordered_map<std::string, Prefab> s_prefabs;
    return s_prefabs;
}

const Prefab &PrefabCatalouge::get(const std::string &id) {
    auto& map = prefabs();
    auto it = map.find(id);
    if (it == map.end()) {
        throw std::runtime_error("PrefabCatalouge: missing prefab with id: " + id);
    }
    return it->second;
}

bool PrefabCatalouge::contains(const std::string &id) {
    auto& map = prefabs();
    return map.find(id) != map.end();
}
