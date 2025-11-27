#include "NavRaster.hpp"

bool NavRaster::inBounds(int x, int y) const
{
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

size_t NavRaster::getIndex(int x, int y) const
{
    return static_cast<size_t>(y * m_width + x);
}

bool NavRaster::isWalkable(int x, int y) const
{
    if (!inBounds(x, y))
    {
        return false;
    }
    return m_walkableCells[getIndex(x, y)] != 0;
}

void NavRaster::setWalkable(int x, int y, bool walkable)
{
    if (!inBounds(x, y))
    {
        return;
    }
    m_walkableCells[getIndex(x, y)] = walkable ? 1 : 0;
}

glm::vec2 NavRaster::cellCenter(int x, int y) const
{
    return glm::vec2(m_origin.x + (x + 0.5f) * m_cellSize,
                     m_origin.y + (y + 0.5f) * m_cellSize);
}

NavAABB NavRaster::cellBounds(int x, int y) const
{
    glm::vec2 min = glm::vec2(m_origin.x + x * m_cellSize,
                              m_origin.y + y * m_cellSize);
    glm::vec2 max = min + glm::vec2(m_cellSize, m_cellSize);
    return NavAABB{min, max};
}

void NavRaster::worldToCell(const glm::vec2& worldPos, int& outX, int& outY) const
{
    outX = static_cast<int>((worldPos.x - m_origin.x) / m_cellSize);
    outY = static_cast<int>((worldPos.y - m_origin.y) / m_cellSize);
    if (!inBounds(outX, outY))
    {
        outX = -1;
        outY = -1;
    }
}

glm::vec2 NavRaster::cellToWorld(int x, int y) const
{
    return glm::vec2(m_origin.x + x * m_cellSize,
                     m_origin.y + y * m_cellSize);
}

NavRaster NavRaster::buildFromPredicate(int width, int height, float cellSize, const glm::vec2& origin,
                                        const std::function<bool(const NavAABB&)>& isWalkablePredicate)
{
    NavRaster raster(width, height, cellSize, origin);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            NavAABB cellAABB = raster.cellBounds(x, y);
            bool walkable = isWalkablePredicate(cellAABB);
            raster.setWalkable(x, y, walkable);
        }
    }
    return raster;
}
