//
// LevelManager.hpp
//

#ifndef GL2D_LEVELMANAGER_HPP
#define GL2D_LEVELMANAGER_HPP

#include <memory>
#include <string>
#include <utility>
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

    [[nodiscard]] const Level* currentLevel() const noexcept {
        return m_current.get();
    }
    [[nodiscard]] const std::string& lastError() const noexcept {
        return m_lastError;
    }
    [[nodiscard]] std::unique_ptr<Level> takeCurrent() noexcept {
        return std::move(m_current);
    }

private:
    std::unique_ptr<Level> m_current{};
    std::string m_lastError{};
};

#endif //GL2D_LEVELMANAGER_HPP
