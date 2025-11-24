//
// Created by Codex on 22/11/2025.
//

#include "TilemapManager.hpp"
#include "Utils/SimpleJson.hpp"
#include <fstream>
#include <sstream>

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

std::shared_ptr<TilemapData> TilemapManager::loadFromFile(const std::string &id, const std::string &path) {
    std::ifstream in(path);
    if (!in) {
        return nullptr;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    auto root = Utils::JsonValue::parse(buffer.str());
    auto data = std::make_shared<TilemapData>();

    if (root.hasKey("tilesetId")) {
        data->tilesetId = root.at("tilesetId").asString();
    }
    if (root.hasKey("width")) data->width = static_cast<int>(root.at("width").asNumber());
    if (root.hasKey("height")) data->height = static_cast<int>(root.at("height").asNumber());
    if (root.hasKey("tileSize")) {
        const auto &arr = root.at("tileSize").asArray();
        if (arr.size() >= 2) {
            data->tileSize = {static_cast<float>(arr[0].asNumber()), static_cast<float>(arr[1].asNumber())};
        }
    }
    if (root.hasKey("tiles") && root.at("tiles").isArray()) {
        const auto &arr = root.at("tiles").asArray();
        data->tiles.reserve(arr.size());
        for (const auto &val : arr) {
            data->tiles.push_back(static_cast<int>(val.asNumber()));
        }
    }
    if (data->width > 0 && data->height > 0 &&
        static_cast<int>(data->tiles.size()) != data->width * data->height) {
        // Size mismatch; clear to avoid bad reads.
        data->tiles.clear();
    }
    registerTilemap(id, data);
    return data;
}
