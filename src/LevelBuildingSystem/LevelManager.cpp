//
// LevelManager.cpp
//

#include "LevelManager.hpp"
#include <utility>

bool LevelManager::loadLevel(const std::string &path) {
    try {
        auto lvl = std::make_unique<Level>(LevelLoader::loadFromFile(path));
        m_current = std::move(lvl);
        return true;
    } catch (const std::exception&) {
        // TODO: log error
        return false;
    }
}

void LevelManager::unloadLevel() {
    m_current.reset();
}

void LevelManager::update(double dt) {
    (void)dt;
    if (m_nextPath && !m_current) {
        loadLevel(*m_nextPath);
        m_nextPath.reset();
    }
    m_transitionTimer += dt;
}
