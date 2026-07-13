#ifndef NAV_RASTER_HPP
#define NAV_RASTER_HPP
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <glm/vec2.hpp>

#include "NavAABB.hpp"

class NavRaster {
public:
    NavRaster() = default;
    NavRaster(int width, int height, float cellSize, const glm::vec2& origin);

    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    [[nodiscard]] float cellSize() const { return m_cellSize; }
    [[nodiscard]] const glm::vec2& origin() const { return m_origin; }
    [[nodiscard]] bool empty() const { return m_walkableCells.empty(); }
    [[nodiscard]] bool inBounds(int x, int y) const;
    [[nodiscard]] bool isWalkable(int x, int y) const;
    void setWalkable(int x, int y, bool walkable);
    [[nodiscard]] glm::vec2 cellCenter(int x, int y) const;
    [[nodiscard]] NavAABB cellBounds(int x, int y) const;
    [[nodiscard]] bool worldToCell(const glm::vec2& worldPos, int& outX, int& outY) const;
    [[nodiscard]] glm::vec2 cellToWorld(int x, int y) const;

    template<typename Func>
    void forEachCell(Func&& func) const {
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                func(x, y, isWalkable(x, y));
            }
        }
    }
    static NavRaster buildFromPredicate(int width, int height, float cellSize, const glm::vec2& origin,
                                        const std::function<bool(const NavAABB&)>& isWalkablePredicate);

private:
    [[nodiscard]] std::size_t indexUnchecked(int x, int y) const;

    int m_width{0};
    int m_height{0};
    float m_cellSize{0.0f};
    glm::vec2 m_origin{0.0f};
    std::vector<std::uint8_t> m_walkableCells;
};
#endif // NAV_RASTER_HPP
