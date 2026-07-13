#include "PolyNavMesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>

#include <glm/geometric.hpp>

#include "Physics/Collision/AABB.hpp"

namespace {
constexpr float kEps = 1e-4f;

bool isFinite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

float signedArea(const glm::vec2& apex, const glm::vec2& a, const glm::vec2& b) {
    return (a.x - apex.x) * (b.y - apex.y) -
           (a.y - apex.y) * (b.x - apex.x);
}

bool nearlyEqual(const glm::vec2& a, const glm::vec2& b) {
    const glm::vec2 delta = a - b;
    return glm::dot(delta, delta) <= kEps * kEps;
}
}

void PolyNavMesh::buildFromRaster(const NavRaster& raster) {
    clear();
    auto regions = buildRegions(raster);
    buildPolygonsFromRegion(raster, regions);
    buildAdjacencies();
}

void PolyNavMesh::clear() {
    m_polys.clear();
}

NavPath PolyNavMesh::findPath(const glm::vec2& start, const glm::vec2& end) const {
    if (!isFinite(start) || !isFinite(end)) return {};
    const int startPoly = findPolyContainingPoint(start);
    const int endPoly = findPolyContainingPoint(end);
    if (startPoly < 0 || endPoly < 0) {
        return {};
    }
    auto polyPath = findPathPolys(startPoly, endPoly);
    if (polyPath.empty()) {
        return {};
    }
    return buildPathWithFunnel(polyPath, start, end);
}

std::vector<NavRegion> PolyNavMesh::buildRegions(const NavRaster& raster) const {
    std::vector<NavRegion> regions;
    const int w = raster.width();
    const int h = raster.height();
    std::vector<std::uint8_t> visited(static_cast<std::size_t>(w) *
                                      static_cast<std::size_t>(h), 0);
    auto idx = [w](int x, int y) {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
               static_cast<std::size_t>(x);
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!raster.isWalkable(x, y) || visited[idx(x, y)]) continue;

            int maxW = 0;
            while (x + maxW < w && raster.isWalkable(x + maxW, y) && !visited[idx(x + maxW, y)]) {
                ++maxW;
            }

            int maxH = 1;
            bool canGrow = true;
            while (y + maxH < h && canGrow) {
                for (int dx = 0; dx < maxW; ++dx) {
                    if (!raster.isWalkable(x + dx, y + maxH) || visited[idx(x + dx, y + maxH)]) {
                        canGrow = false;
                        break;
                    }
                }
                if (canGrow) {
                    ++maxH;
                }
            }

            for (int dy = 0; dy < maxH; ++dy) {
                for (int dx = 0; dx < maxW; ++dx) {
                    visited[idx(x + dx, y + dy)] = 1;
                }
            }

            NavRegion r{};
            r.xMin = x;
            r.xMax = x + maxW - 1;
            r.yMin = y;
            r.yMax = y + maxH - 1;
            regions.push_back(r);
        }
    }
    return regions;
}

void PolyNavMesh::buildPolygonsFromRegion(const NavRaster& raster, const std::vector<NavRegion>& regions) {
    m_polys.clear();
    m_polys.reserve(regions.size());
    const float cellSize = raster.cellSize();
    const glm::vec2 origin = raster.origin();

    for (size_t i = 0; i < regions.size(); ++i) {
        const auto& r = regions[i];
        const float minX = origin.x + static_cast<float>(r.xMin) * cellSize;
        const float maxX = origin.x + static_cast<float>(r.xMax + 1) * cellSize;
        const float minY = origin.y + static_cast<float>(r.yMin) * cellSize;
        const float maxY = origin.y + static_cast<float>(r.yMax + 1) * cellSize;

        NavPoly poly{};
        poly.id = static_cast<int>(i);
        poly.vertices.push_back(glm::vec2{minX, minY});
        poly.vertices.push_back(glm::vec2{maxX, minY});
        poly.vertices.push_back(glm::vec2{maxX, maxY});
        poly.vertices.push_back(glm::vec2{minX, maxY});
        poly.bounds = AABB(glm::vec2{minX, minY}, glm::vec2{maxX, maxY});
        m_polys.push_back(std::move(poly));
    }
}

void PolyNavMesh::buildAdjacencies() {
    const float eps = kEps;
    for (auto& p : m_polys) {
        p.neighbors.clear();
        p.portals.clear();
    }
    for (size_t i = 0; i < m_polys.size(); ++i) {
        for (size_t j = i + 1; j < m_polys.size(); ++j) {
            glm::vec2 a0, a1;
            if (sharesEdge(m_polys[i], m_polys[j], eps, a0, a1)) {
                const int ni = static_cast<int>(j);
                const int nj = static_cast<int>(i);
                m_polys[i].neighbors.push_back(ni);
                m_polys[j].neighbors.push_back(nj);

                const glm::vec2 centerI = computePolyCenter(m_polys[i]);
                const glm::vec2 centerJ = computePolyCenter(m_polys[j]);
                // The funnel implementation consumes clockwise portal winding
                // (its "left" endpoint is the lower signed-area endpoint in
                // GL2D's x/y coordinate system).
                const bool a0IsLeftFromI = signedArea(centerI, centerJ, a0) <=
                                           signedArea(centerI, centerJ, a1);
                m_polys[i].portals.push_back(
                    NavPortal{ni, a0IsLeftFromI ? a0 : a1, a0IsLeftFromI ? a1 : a0});
                m_polys[j].portals.push_back(
                    NavPortal{nj, a0IsLeftFromI ? a1 : a0, a0IsLeftFromI ? a0 : a1});
            }
        }
    }
}

int PolyNavMesh::findPolyContainingPoint(const glm::vec2& point) const {
    for (size_t i = 0; i < m_polys.size(); ++i) {
        if (m_polys[i].bounds.contains(point)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<int> PolyNavMesh::findPathPolys(int startPolyIdx, int endPolyIdx) const {
    auto heuristic = [&](int a, int b) {
        const glm::vec2 ca = computePolyCenter(m_polys[a]);
        const glm::vec2 cb = computePolyCenter(m_polys[b]);
        return glm::length(cb - ca);
    };

    std::priority_queue<std::pair<float, int>,
                        std::vector<std::pair<float, int>>,
                        std::greater<>> open;
    std::vector<float> gScores(m_polys.size(), std::numeric_limits<float>::infinity());
    std::vector<int> parents(m_polys.size(), -1);
    std::vector<std::uint8_t> closed(m_polys.size(), 0);

    gScores[static_cast<std::size_t>(startPolyIdx)] = 0.0f;
    open.push({heuristic(startPolyIdx, endPolyIdx), startPolyIdx});
    bool reachedEnd = false;

    while (!open.empty()) {
        const auto [_, currentIdx] = open.top();
        open.pop();
        const auto current = static_cast<std::size_t>(currentIdx);
        if (closed[current]) continue;
        if (currentIdx == endPolyIdx) {
            reachedEnd = true;
            break;
        }
        closed[current] = 1;

        for (int nei : m_polys[currentIdx].neighbors) {
            const auto neighbor = static_cast<std::size_t>(nei);
            if (closed[neighbor]) continue;
            const float tentativeG = gScores[current] + heuristic(currentIdx, nei);
            if (tentativeG < gScores[neighbor] - kEps) {
                parents[neighbor] = currentIdx;
                gScores[neighbor] = tentativeG;
                open.push({tentativeG + heuristic(nei, endPolyIdx), nei});
            }
        }
    }

    std::vector<int> path;
    if (!reachedEnd) return path;
    int cur = endPolyIdx;
    while (cur != -1) {
        path.push_back(cur);
        cur = parents[static_cast<std::size_t>(cur)];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

glm::vec2 PolyNavMesh::computePolyCenter(const NavPoly& poly) const {
    glm::vec2 sum{0.0f};
    for (const auto& v : poly.vertices) {
        sum += v;
    }
    const float inv = poly.vertices.empty() ? 0.0f : 1.0f / static_cast<float>(poly.vertices.size());
    return sum * inv;
}

bool PolyNavMesh::sharesEdge(const NavPoly& a, const NavPoly& b, float epsilon, glm::vec2& outA, glm::vec2& outB) const {
    const AABB aBox = a.bounds;
    const AABB bBox = b.bounds;
    const glm::vec2 aMin = aBox.getMin();
    const glm::vec2 aMax = aBox.getMax();
    const glm::vec2 bMin = bBox.getMin();
    const glm::vec2 bMax = bBox.getMax();

    // Check vertical adjacency (right of A to left of B or vice versa).
    if (std::abs(aMax.x - bMin.x) <= epsilon) {
        const float y0 = std::max(aMin.y, bMin.y);
        const float y1 = std::min(aMax.y, bMax.y);
        if (y1 - y0 > epsilon) {
            const float boundary = (aMax.x + bMin.x) * 0.5f;
            outA = glm::vec2{boundary, y0};
            outB = glm::vec2{boundary, y1};
            return true;
        }
    }
    if (std::abs(bMax.x - aMin.x) <= epsilon) {
        const float y0 = std::max(aMin.y, bMin.y);
        const float y1 = std::min(aMax.y, bMax.y);
        if (y1 - y0 > epsilon) {
            const float boundary = (bMax.x + aMin.x) * 0.5f;
            outA = glm::vec2{boundary, y0};
            outB = glm::vec2{boundary, y1};
            return true;
        }
    }
    // Check horizontal adjacency (top of A to bottom of B or vice versa).
    if (std::abs(aMax.y - bMin.y) <= epsilon) {
        const float x0 = std::max(aMin.x, bMin.x);
        const float x1 = std::min(aMax.x, bMax.x);
        if (x1 - x0 > epsilon) {
            const float boundary = (aMax.y + bMin.y) * 0.5f;
            outA = glm::vec2{x0, boundary};
            outB = glm::vec2{x1, boundary};
            return true;
        }
    }
    if (std::abs(bMax.y - aMin.y) <= epsilon) {
        const float x0 = std::max(aMin.x, bMin.x);
        const float x1 = std::min(aMax.x, bMax.x);
        if (x1 - x0 > epsilon) {
            const float boundary = (bMax.y + aMin.y) * 0.5f;
            outA = glm::vec2{x0, boundary};
            outB = glm::vec2{x1, boundary};
            return true;
        }
    }
    return false;
}

bool PolyNavMesh::getPortal(int aIdx, int bIdx, glm::vec2& outLeft, glm::vec2& outRight) const {
    if (aIdx < 0 || aIdx >= static_cast<int>(m_polys.size())) return false;
    for (const auto& p : m_polys[aIdx].portals) {
        if (p.neighbor == bIdx) {
            outLeft = p.left;
            outRight = p.right;
            return true;
        }
    }
    return false;
}

NavPath PolyNavMesh::buildPathWithFunnel(const std::vector<int>& polyPath,
                                         const glm::vec2& start,
                                         const glm::vec2& end) const {
    NavPath path;
    if (polyPath.empty()) return path;
    if (polyPath.size() == 1) {
        path.points = nearlyEqual(start, end) ? std::vector<glm::vec2>{start}
                                              : std::vector<glm::vec2>{start, end};
        return path;
    }

    struct PortalSeg { glm::vec2 left; glm::vec2 right; };
    std::vector<PortalSeg> portals;
    portals.reserve(polyPath.size() + 1);
    portals.push_back({start, start});

    for (size_t i = 0; i + 1 < polyPath.size(); ++i) {
        glm::vec2 p0, p1;
        if (!getPortal(polyPath[i], polyPath[i + 1], p0, p1)) {
            throw std::logic_error("Navmesh path corridor is missing a shared portal");
        }
        portals.push_back({p0, p1});
    }
    portals.push_back({end, end});

    glm::vec2 apex = portals[0].left;
    glm::vec2 left = portals[0].left;
    glm::vec2 right = portals[0].right;
    size_t apexIndex = 0, leftIndex = 0, rightIndex = 0;

    path.points.clear();
    path.points.push_back(apex);

    for (size_t i = 1; i < portals.size(); ++i) {
        const glm::vec2 newLeft = portals[i].left;
        const glm::vec2 newRight = portals[i].right;

        // Tighten right
        if (signedArea(apex, right, newRight) <= 0.0f) {
            if (nearlyEqual(apex, right) || signedArea(apex, left, newRight) > 0.0f) {
                right = newRight;
                rightIndex = i;
            } else {
                apex = left;
                if (!nearlyEqual(path.points.back(), apex)) path.points.push_back(apex);
                apexIndex = leftIndex;
                left = apex;
                right = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                i = apexIndex;
                continue;
            }
        }

        // Tighten left
        if (signedArea(apex, left, newLeft) >= 0.0f) {
            if (nearlyEqual(apex, left) || signedArea(apex, right, newLeft) < 0.0f) {
                left = newLeft;
                leftIndex = i;
            } else {
                apex = right;
                if (!nearlyEqual(path.points.back(), apex)) path.points.push_back(apex);
                apexIndex = rightIndex;
                left = apex;
                right = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                i = apexIndex;
                continue;
            }
        }
    }

    if (path.points.empty() || glm::length(path.points.back() - end) > kEps) {
        path.points.push_back(end);
    }
    return path;
}
