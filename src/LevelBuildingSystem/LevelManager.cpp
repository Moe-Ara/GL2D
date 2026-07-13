//
// LevelManager.cpp
//

#include "LevelManager.hpp"
#include <utility>

bool LevelManager::loadLevel(const std::string &path) {
    try {
        auto lvl = std::make_unique<Level>(LevelLoader::loadFromFile(path));
        m_current = std::move(lvl);
        m_lastError.clear();
        return true;
    } catch (const std::exception& error) {
        m_lastError = error.what();
        return false;
    } catch (...) {
        m_lastError = "Unknown level loading failure: " + path;
        return false;
    }
}

void LevelManager::unloadLevel() {
    m_current.reset();
}
