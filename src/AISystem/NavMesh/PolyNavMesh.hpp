#ifndef POLY_NAV_MESH_HPP
#define POLY_NAV_MESH_HPP
#include <vector>
#include "NavRaster.hpp"
#include "NavPoly.hpp"
#include "INavMesh.hpp"
#include "NavRegion.hpp"
class PolyNavMesh: public INavMesh
{
    public:
    PolyNavMesh()=default;
    void buildFromRaster(const NavRaster& raster) override;
    void rebuildRegion(const NavRaster& raster, const NavAABB& region) override;
    void clear() override;
    NavPath findPath(const glm::vec2& start, const glm::vec2& end) override;
    void debugDraw() override;
    const std::vector<NavPoly>& getPolys() const;
    private:
    std::vector<NavPoly> m_polys;

    private:
    std::vector<NavRegion> buildRegions(const NavRaster& raster) const;
    void buildPolygonsFromRegion(const NavRaster& raster, const std::vector<NavRegion>& regions);
    void buildAdjacencies();
    int findPolyContainingPoint(const glm::vec2& point) const;
    std::vector<int> findPathPolys(int startPolyIdx, int endPolyIdx) const;
    NavPath buildPathFromPolyCenters(const std::vector<int>& polyPath) const;
    NavPath buildPathWithFunnel(const std::vector<int>& polyPath,
                                const glm::vec2& start,
                                const glm::vec2& end) const;

    glm::vec2 computePolyCenter(const NavPoly& poly) const;
    bool sharesEdge(const NavPoly& a, const NavPoly& b,float epsilon, glm::vec2& outA, glm::vec2& outB) const;
    bool getPortal(int aIdx, int bIdx, glm::vec2& outLeft, glm::vec2& outRight) const;
};


#endif //POLY_NAV_MESH_HPP
