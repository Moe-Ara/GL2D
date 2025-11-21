//
// LevelManager.hpp
//

#ifndef GL2D_LEVELMANAGER_HPP
#define GL2D_LEVELMANAGER_HPP

#include <memory>
#include <string>
#include <optional>
#include "Level.hpp"
#include "LevelLoader.hpp"

class LevelManager {
public:
    LevelManager() = default;
    ~LevelManager() = default;

    LevelManager(const LevelManager&) = delete;
    LevelManager& operator=(const LevelManager&) = delete;
    LevelManager(LevelManager&&) = delete;
    LevelManager& operator=(LevelManager&&) = delete;

    bool loadLevel(const std::string& path);
    void unloadLevel();
    void update(double dt);

    const Level* currentLevel() const { return m_current.get(); }

private:
    std::unique_ptr<Level> m_current{};
    std::optional<std::string> m_nextPath{};
    double m_transitionTimer{0.0};
};

#endif //GL2D_LEVELMANAGER_HPP
