//
// LevelLoader.hpp
//

#ifndef GL2D_LEVELLOADER_HPP
#define GL2D_LEVELLOADER_HPP

#include <string>
#include <vector>
#include "LevelSchema.hpp"
#include "Level.hpp"

class LevelLoader {
public:
    LevelLoader() = delete;
    ~LevelLoader() = delete;
    LevelLoader(const LevelLoader&) = delete;
    LevelLoader& operator=(const LevelLoader&) = delete;
    LevelLoader(LevelLoader&&) = delete;
    LevelLoader& operator=(LevelLoader&&) = delete;

    // Load from an already-parsed LevelData (useful for editor/testing).
    static Level loadFromData(const LevelData& data);

    // Stub for file-based load; integrate a JSON loader later.
    static Level loadFromFile(const std::string& path);
};

#endif //GL2D_LEVELLOADER_HPP
