//
// Created by Codex on 22/11/2025.
//

#ifndef GL2D_TILEMAPMANAGER_HPP
#define GL2D_TILEMAPMANAGER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include "GameObjects/Components/TilemapComponent.hpp"

class TilemapManager {
public:
    TilemapManager() = delete;
    ~TilemapManager() = delete;
    TilemapManager(const TilemapManager&) = delete;
    TilemapManager& operator=(const TilemapManager&) = delete;
    TilemapManager(TilemapManager&&) = delete;
    TilemapManager& operator=(TilemapManager&&) = delete;

    static void registerTilemap(const std::string& id, std::shared_ptr<TilemapData> data);
    static std::shared_ptr<TilemapData> get(const std::string& id);
    static bool contains(const std::string& id);
    // Load a tilemap from a simple JSON file:
    // { "tilesetId": "...", "width": N, "height": M, "tileSize": [w,h], "tiles": [ ...indices... ] }
    static std::shared_ptr<TilemapData> loadFromFile(const std::string& id, const std::string& path);

private:
    static std::unordered_map<std::string, std::shared_ptr<TilemapData>>& maps();
};

#endif //GL2D_TILEMAPMANAGER_HPP
