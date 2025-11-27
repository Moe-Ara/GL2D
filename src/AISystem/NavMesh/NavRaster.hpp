#ifndef NAV_RASTER_HPP
#define NAV_RASTER_HPP
#include <vector>
#include <glm/vec2.hpp>
#include <functional>
#include "NavAABB.hpp"
#include <cstdint>
class NavRaster
{
    public:
    NavRaster()=default;
    NavRaster(int width, int height, float cellSize, const glm::vec2& origin)
        : m_width(width), m_height(height), m_cellSize(cellSize), m_origin(origin), m_walkableCells(width*height, 1)
    {
        if (m_width <=0 || m_height <=0 || m_cellSize <=0.0f)
        {
            this->m_width = 0;
            this->m_height = 0;
            this->m_cellSize = 0.0f;
            m_walkableCells.clear();
        }
    }
    int getm_width() const { return m_width; }
    int getm_height() const { return m_height; }
    float getm_cellsize() const { return m_cellSize; }
    glm::vec2 getm_origin() const { return m_origin; }
    bool inBounds(int x, int y) const;
    size_t getIndex(int x, int y) const;
    bool isWalkable(int x, int y) const;
    void setWalkable(int x, int y, bool walkable);
    glm::vec2 cellCenter(int x, int y) const;
    NavAABB cellBounds(int x, int y) const;
    void worldToCell(const glm::vec2& worldPos, int& outX, int& outY) const;
    glm::vec2 cellToWorld(int x, int y) const;
    template<typename Func>
    void forEachCell(Func func) const {
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                func(x, y, isWalkable(x, y));
            }
        }
    }
    static NavRaster buildFromPredicate(int width, int height, float cellSize, const glm::vec2& origin,
                                        const std::function<bool(const NavAABB&)>& isWalkablePredicate);
    private:

    int m_width;
    int m_height;
    float m_cellSize;
    glm::vec2 m_origin; 
    std::vector<uint8_t> m_walkableCells; 

};
#endif //NAV_RASTER_HPP
