#include "PolyNavMesh.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include <GL/glew.h>

#include "Physics/Collision/AABB.hpp"

namespace {
constexpr float kEps = 1e-4f;
}

void PolyNavMesh::buildFromRaster(const NavRaster& raster) {
    clear();
    auto regions = buildRegions(raster);
    buildPolygonsFromRegion(raster, regions);
    buildAdjacencies();
}

void PolyNavMesh::rebuildRegion(const NavRaster& raster, const NavAABB& /*region*/) {
    buildFromRaster(raster);
}

void PolyNavMesh::clear() {
    m_polys.clear();
}

const std::vector<NavPoly>& PolyNavMesh::getPolys() const {
    return m_polys;
}

NavPath PolyNavMesh::findPath(const glm::vec2& start, const glm::vec2& end) {
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

void PolyNavMesh::debugDraw() {
    glUseProgram(0);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0f);

    glColor3f(0.0f, 1.0f, 0.0f);
    for (const auto& poly : m_polys) {
        glBegin(GL_LINE_LOOP);
        for (const auto& v : poly.vertices) {
            glVertex2f(v.x, v.y);
        }
        glEnd();
    }

    glColor3f(0.0f, 0.5f, 1.0f);
    glBegin(GL_LINES);
    for (const auto& poly : m_polys) {
        for (const auto& portal : poly.portals) {
            glVertex2f(portal.left.x, portal.left.y);
            glVertex2f(portal.right.x, portal.right.y);
        }
    }
    glEnd();
}

std::vector<NavRegion> PolyNavMesh::buildRegions(const NavRaster& raster) const {
    std::vector<NavRegion> regions;
    const int w = raster.getm_width();
    const int h = raster.getm_height();
    std::vector<uint8_t> visited(static_cast<size_t>(w * h), 0);
    auto idx = [w](int x, int y) { return static_cast<size_t>(y * w + x); };

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
    const float cellSize = raster.getm_cellsize();
    const glm::vec2 origin = raster.getm_origin();

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
                m_polys[i].portals.push_back(NavPortal{ni, a0, a1});
                m_polys[j].portals.push_back(NavPortal{nj, a0, a1});
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
    struct Node {
        int idx;
        int parent;
        float g;
        float f;
    };
    auto heuristic = [&](int a, int b) {
        const glm::vec2 ca = computePolyCenter(m_polys[a]);
        const glm::vec2 cb = computePolyCenter(m_polys[b]);
        return glm::length(cb - ca);
    };

    std::priority_queue<std::pair<float, int>,
                        std::vector<std::pair<float, int>>,
                        std::greater<>> open;
    std::unordered_map<int, Node> nodes;
    std::unordered_set<int> closed;

    Node start{startPolyIdx, -1, 0.0f, heuristic(startPolyIdx, endPolyIdx)};
    nodes[startPolyIdx] = start;
    open.push({start.f, startPolyIdx});

    while (!open.empty()) {
        const auto [_, currentIdx] = open.top();
        open.pop();
        if (closed.find(currentIdx) != closed.end()) continue;
        if (currentIdx == endPolyIdx) {
            break;
        }
        closed.insert(currentIdx);

        const auto& current = nodes[currentIdx];
        for (int nei : m_polys[currentIdx].neighbors) {
            if (closed.find(nei) != closed.end()) continue;
            const float tentativeG = current.g + heuristic(currentIdx, nei);
            auto it = nodes.find(nei);
            if (it == nodes.end() || tentativeG < it->second.g - kEps) {
                Node n{};
                n.idx = nei;
                n.parent = currentIdx;
                n.g = tentativeG;
                n.f = tentativeG + heuristic(nei, endPolyIdx);
                nodes[nei] = n;
                open.push({n.f, nei});
            }
        }
    }

    std::vector<int> path;
    auto itEnd = nodes.find(endPolyIdx);
    if (itEnd == nodes.end()) {
        return path;
    }
    int cur = endPolyIdx;
    while (cur != -1) {
        path.push_back(cur);
        cur = nodes[cur].parent;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

NavPath PolyNavMesh::buildPathFromPolyCenters(const std::vector<int>& polyPath) const {
    NavPath path;
    path.points.reserve(polyPath.size());
    for (int idx : polyPath) {
        if (idx < 0 || idx >= static_cast<int>(m_polys.size())) continue;
        path.points.push_back(computePolyCenter(m_polys[idx]));
    }
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

    // Check vertical adjacency (right of A to left of B or vice versa)
    if (std::abs(aMax.x - bMin.x) <= epsilon || std::abs(bMax.x - aMin.x) <= epsilon) {
        const float y0 = std::max(aMin.y, bMin.y);
        const float y1 = std::min(aMax.y, bMax.y);
        if (y1 - y0 > epsilon) {
            outA = glm::vec2{(aMax.x + bMin.x) * 0.5f, y0};
            outB = glm::vec2{(aMax.x + bMin.x) * 0.5f, y1};
            return true;
        }
    }
    // Check horizontal adjacency (top of A to bottom of B or vice versa)
    if (std::abs(aMax.y - bMin.y) <= epsilon || std::abs(bMax.y - aMin.y) <= epsilon) {
        const float x0 = std::max(aMin.x, bMin.x);
        const float x1 = std::min(aMax.x, bMax.x);
        if (x1 - x0 > epsilon) {
            outA = glm::vec2{x0, (aMax.y + bMin.y) * 0.5f};
            outB = glm::vec2{x1, (aMax.y + bMin.y) * 0.5f};
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

static float cross2(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

NavPath PolyNavMesh::buildPathWithFunnel(const std::vector<int>& polyPath,
                                         const glm::vec2& start,
                                         const glm::vec2& end) const {
    NavPath path;
    if (polyPath.empty()) return path;
    if (polyPath.size() == 1) {
        path.points = {start, end};
        return path;
    }

    struct PortalSeg { glm::vec2 left; glm::vec2 right; };
    std::vector<PortalSeg> portals;
    portals.reserve(polyPath.size() + 1);
    portals.push_back({start, start});

    for (size_t i = 0; i + 1 < polyPath.size(); ++i) {
        glm::vec2 p0, p1;
        if (!getPortal(polyPath[i], polyPath[i + 1], p0, p1)) {
            p0 = computePolyCenter(m_polys[polyPath[i]]);
            p1 = computePolyCenter(m_polys[polyPath[i + 1]]);
        }
        if (cross2(start, p0, p1) > 0.0f) {
            portals.push_back({p0, p1});
        } else {
            portals.push_back({p1, p0});
        }
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
        if (cross2(apex, right, newRight) <= 0.0f) {
            if (cross2(apex, left, newRight) < 0.0f) {
                // Advance apex to left
                apex = left;
                path.points.push_back(apex);
                apexIndex = leftIndex;
                left = apex;
                right = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                i = apexIndex;
                continue;
            } else {
                right = newRight;
                rightIndex = i;
            }
        }

        // Tighten left
        if (cross2(apex, left, newLeft) >= 0.0f) {
            if (cross2(apex, right, newLeft) > 0.0f) {
                // Advance apex to right
                apex = right;
                path.points.push_back(apex);
                apexIndex = rightIndex;
                left = apex;
                right = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                i = apexIndex;
                continue;
            } else {
                left = newLeft;
                leftIndex = i;
            }
        }
    }

    if (path.points.empty() || glm::length(path.points.back() - end) > kEps) {
        path.points.push_back(end);
    }
    return path;
}
