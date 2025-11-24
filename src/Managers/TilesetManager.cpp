//
// TilesetManager.cpp
//

#include "TilesetManager.hpp"

#include "Managers/TextureManager.hpp"

namespace {
std::vector<glm::vec4> computeUVs(int rows, int cols, int margin, int spacing, glm::ivec2 tileSize, glm::ivec2 atlasSize) {
    std::vector<glm::vec4> result;
    if (rows <= 0 || cols <= 0 || tileSize.x <= 0 || tileSize.y <= 0 || atlasSize.x <= 0 || atlasSize.y <= 0) {
        return result;
    }
    result.reserve(rows * cols);
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const int x = margin + col * (tileSize.x + spacing);
            const int y = margin + row * (tileSize.y + spacing);
            const float u0 = static_cast<float>(x) / static_cast<float>(atlasSize.x);
            const float v0 = static_cast<float>(y) / static_cast<float>(atlasSize.y);
            const float u1 = static_cast<float>(x + tileSize.x) / static_cast<float>(atlasSize.x);
            const float v1 = static_cast<float>(y + tileSize.y) / static_cast<float>(atlasSize.y);
            result.push_back(glm::vec4{u0, v0, u1, v1});
        }
    }
    return result;
}
} // namespace

std::unordered_map<std::string, std::shared_ptr<Tileset>> &TilesetManager::tilesets() {
    static std::unordered_map<std::string, std::shared_ptr<Tileset>> s_tilesets;
    return s_tilesets;
}

std::shared_ptr<Tileset> TilesetManager::buildTileset(const std::string &id,
                                                      const std::string &texturePath,
                                                      glm::ivec2 tileSize,
                                                      int rows,
                                                      int cols,
                                                      int margin,
                                                      int spacing,
                                                      std::vector<bool> solid) {
    auto ts = std::make_shared<Tileset>();
    ts->id = id;
    ts->texturePath = texturePath;
    ts->tileSize = tileSize;
    ts->rows = rows;
    ts->cols = cols;
    ts->margin = margin;
    ts->spacing = spacing;
    ts->texture = Managers::TextureManager::loadTexture(texturePath);
    if (ts->texture) {
        // Texture stores width/height internally.
        const int atlasW = ts->texture->getWidth();
        const int atlasH = ts->texture->getHeight();
        ts->uvs = computeUVs(rows, cols, margin, spacing, tileSize, {atlasW, atlasH});
    }
    if (solid.empty()) {
        ts->solid.resize(static_cast<size_t>(rows * cols), false);
    } else {
        ts->solid = std::move(solid);
        ts->solid.resize(static_cast<size_t>(rows * cols), false);
    }
    return ts;
}

std::shared_ptr<Tileset> TilesetManager::load(const std::string &id, const std::string &texturePath, glm::ivec2 tileSize, int rows, int cols, int margin, int spacing, std::vector<bool> solid) {
    auto &map = tilesets();
    auto it = map.find(id);
    if (it != map.end()) {
        return it->second;
    }
    auto ts = buildTileset(id, texturePath, tileSize, rows, cols, margin, spacing, std::move(solid));
    map[id] = ts;
    return ts;
}

std::shared_ptr<Tileset> TilesetManager::loadFromImage(const std::string &id, const std::string &texturePath, glm::ivec2 tileSize, int margin, int spacing, std::vector<bool> solid) {
    auto &map = tilesets();
    auto it = map.find(id);
    if (it != map.end()) {
        return it->second;
    }
    // Temporarily load texture to infer rows/cols.
    auto tex = Managers::TextureManager::loadTexture(texturePath);
    if (!tex || tileSize.x <= 0 || tileSize.y <= 0) {
        return nullptr;
    }
    const int atlasW = tex->getWidth();
    const int atlasH = tex->getHeight();
    const int cols = (atlasW - 2 * margin + spacing) / (tileSize.x + spacing);
    const int rows = (atlasH - 2 * margin + spacing) / (tileSize.y + spacing);
    auto ts = buildTileset(id, texturePath, tileSize, rows, cols, margin, spacing, std::move(solid));
    ts->texture = tex;
    map[id] = ts;
    return ts;
}

std::shared_ptr<Tileset> TilesetManager::get(const std::string &id) {
    auto &map = tilesets();
    auto it = map.find(id);
    if (it != map.end()) {
        return it->second;
    }
    return nullptr;
}

bool TilesetManager::contains(const std::string &id) {
    auto &map = tilesets();
    return map.find(id) != map.end();
}

void TilesetManager::registerTileset(const std::string &id, std::shared_ptr<Tileset> tileset) {
    tilesets()[id] = std::move(tileset);
}

void TilesetManager::clear() {
    tilesets().clear();
}
