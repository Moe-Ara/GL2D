//
// TilesetManager.hpp
//

#ifndef GL2D_TILESETMANAGER_HPP
#define GL2D_TILESETMANAGER_HPP

#include "LevelBuildingSystem/Tileset.hpp"
#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <unordered_map>

class TilesetManager {
public:
    TilesetManager() = delete;
    ~TilesetManager() = delete;
    TilesetManager(const TilesetManager&) = delete;
    TilesetManager& operator=(const TilesetManager&) = delete;
    TilesetManager(TilesetManager&&) = delete;
    TilesetManager& operator=(TilesetManager&&) = delete;

    // Loads or returns cached tileset with explicit atlas layout parameters.
    static std::shared_ptr<Tileset> load(const std::string& id,
                                         const std::string& texturePath,
                                         glm::ivec2 tileSize,
                                         int rows,
                                         int cols,
                                         int margin = 0,
                                         int spacing = 0,
                                         std::vector<bool> solid = {});

    // Loads or returns cached tileset, computing rows/cols from image size and tileSize.
    static std::shared_ptr<Tileset> loadFromImage(const std::string& id,
                                                  const std::string& texturePath,
                                                  glm::ivec2 tileSize,
                                                  int margin = 0,
                                                  int spacing = 0,
                                                  std::vector<bool> solid = {});

    // Looks up a tileset by id; nullptr if missing.
    static std::shared_ptr<Tileset> get(const std::string& id);
    static bool contains(const std::string& id);
    static void registerTileset(const std::string& id, std::shared_ptr<Tileset> tileset);
    static void clear();

private:
    static std::unordered_map<std::string, std::shared_ptr<Tileset>>& tilesets();
    static std::shared_ptr<Tileset> buildTileset(const std::string& id,
                                                 const std::string& texturePath,
                                                 glm::ivec2 tileSize,
                                                 int rows,
                                                 int cols,
                                                 int margin,
                                                 int spacing,
                                                 std::vector<bool> solid);
};

#endif // GL2D_TILESETMANAGER_HPP
