#include "NavRaster.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
bool isFinite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}
}

NavRaster::NavRaster(int width, int height, float cellSize, const glm::vec2& origin)
    : m_width(width), m_height(height), m_cellSize(cellSize), m_origin(origin) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("NavRaster dimensions must be positive");
    }
    if (!std::isfinite(cellSize) || cellSize <= 0.0f) {
        throw std::invalid_argument("NavRaster cell size must be finite and positive");
    }
    if (!isFinite(origin)) {
        throw std::invalid_argument("NavRaster origin must be finite");
    }
    const double maxX = static_cast<double>(origin.x) +
                        static_cast<double>(width) * cellSize;
    const double maxY = static_cast<double>(origin.y) +
                        static_cast<double>(height) * cellSize;
    const double floatLimit = std::numeric_limits<float>::max();
    if (!std::isfinite(maxX) || !std::isfinite(maxY) ||
        maxX < -floatLimit || maxX > floatLimit ||
        maxY < -floatLimit || maxY > floatLimit) {
        throw std::overflow_error("NavRaster world bounds are not representable as floats");
    }

    const auto widthSize = static_cast<std::size_t>(width);
    const auto heightSize = static_cast<std::size_t>(height);
    if (widthSize > std::numeric_limits<std::size_t>::max() / heightSize) {
        throw std::length_error("NavRaster cell count overflows size_t");
    }
    const std::size_t count = widthSize * heightSize;
    if (count > m_walkableCells.max_size()) {
        throw std::length_error("NavRaster is too large");
    }
    m_walkableCells.assign(count, std::uint8_t{1});
}

bool NavRaster::inBounds(int x, int y) const
{
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

std::size_t NavRaster::indexUnchecked(int x, int y) const
{
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) +
           static_cast<std::size_t>(x);
}

bool NavRaster::isWalkable(int x, int y) const
{
    if (!inBounds(x, y))
    {
        return false;
    }
    return m_walkableCells[indexUnchecked(x, y)] != 0;
}

void NavRaster::setWalkable(int x, int y, bool walkable)
{
    if (!inBounds(x, y))
    {
        throw std::out_of_range("NavRaster cell coordinates are out of bounds");
    }
    m_walkableCells[indexUnchecked(x, y)] = walkable ? 1 : 0;
}

glm::vec2 NavRaster::cellCenter(int x, int y) const
{
    if (!inBounds(x, y)) {
        throw std::out_of_range("NavRaster cell coordinates are out of bounds");
    }
    return glm::vec2(m_origin.x + (x + 0.5f) * m_cellSize,
                     m_origin.y + (y + 0.5f) * m_cellSize);
}

NavAABB NavRaster::cellBounds(int x, int y) const
{
    if (!inBounds(x, y)) {
        throw std::out_of_range("NavRaster cell coordinates are out of bounds");
    }
    glm::vec2 min = glm::vec2(m_origin.x + x * m_cellSize,
                              m_origin.y + y * m_cellSize);
    glm::vec2 max = min + glm::vec2(m_cellSize, m_cellSize);
    return NavAABB{min, max};
}

bool NavRaster::worldToCell(const glm::vec2& worldPos, int& outX, int& outY) const
{
    outX = -1;
    outY = -1;
    if (empty() || !isFinite(worldPos)) {
        return false;
    }

    const double cellX = std::floor((static_cast<double>(worldPos.x) - m_origin.x) /
                                    m_cellSize);
    const double cellY = std::floor((static_cast<double>(worldPos.y) - m_origin.y) /
                                    m_cellSize);
    if (cellX < static_cast<double>(std::numeric_limits<int>::min()) ||
        cellX > static_cast<double>(std::numeric_limits<int>::max()) ||
        cellY < static_cast<double>(std::numeric_limits<int>::min()) ||
        cellY > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }

    const int x = static_cast<int>(cellX);
    const int y = static_cast<int>(cellY);
    if (!inBounds(x, y)) return false;
    outX = x;
    outY = y;
    return true;
}

glm::vec2 NavRaster::cellToWorld(int x, int y) const
{
    if (!inBounds(x, y)) {
        throw std::out_of_range("NavRaster cell coordinates are out of bounds");
    }
    return glm::vec2(m_origin.x + x * m_cellSize,
                     m_origin.y + y * m_cellSize);
}

NavRaster NavRaster::buildFromPredicate(int width, int height, float cellSize, const glm::vec2& origin,
                                        const std::function<bool(const NavAABB&)>& isWalkablePredicate)
{
    if (!isWalkablePredicate) {
        throw std::invalid_argument("NavRaster walkability predicate is empty");
    }
    NavRaster raster(width, height, cellSize, origin);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            NavAABB cellAABB = raster.cellBounds(x, y);
            raster.setWalkable(x, y, isWalkablePredicate(cellAABB));
        }
    }
    return raster;
}
