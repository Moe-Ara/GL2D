#ifndef INAV_MESH_HPP
#define INAV_MESH_HPP

#include <glm/vec2.hpp>

#include "NavPath.hpp"
#include "NavRaster.hpp"

class INavMesh {
public:
    virtual ~INavMesh() = default;

    virtual void buildFromRaster(const NavRaster& raster) = 0;
    [[nodiscard]] virtual NavPath findPath(const glm::vec2& start,
                                           const glm::vec2& end) const = 0;
    virtual void clear() = 0;
};

#endif // INAV_MESH_HPP
