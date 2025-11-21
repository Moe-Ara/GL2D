//
// Created by Codex on 22/11/2025.
//

#include "TilemapManager.hpp"

void TilemapManager::registerTilemap(const std::string &id, std::shared_ptr<TilemapData> data) {
    auto& map = maps();
    map[id] = std::move(data);
}

std::shared_ptr<TilemapData> TilemapManager::get(const std::string &id) {
    auto& map = maps();
    auto it = map.find(id);
    if (it != map.end()) {
        return it->second;
    }
    return nullptr;
}

bool TilemapManager::contains(const std::string &id) {
    auto& map = maps();
    return map.find(id) != map.end();
}

std::unordered_map<std::string, std::shared_ptr<TilemapData>> &TilemapManager::maps() {
    static std::unordered_map<std::string, std::shared_ptr<TilemapData>> s_maps;
    return s_maps;
}
