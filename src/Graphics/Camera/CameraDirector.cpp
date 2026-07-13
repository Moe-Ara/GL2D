#include "Graphics/Camera/CameraDirector.hpp"

#include "Graphics/Camera/Camera.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

bool finiteBounds(const glm::vec4& bounds) {
    return std::isfinite(bounds.x) && std::isfinite(bounds.y) &&
           std::isfinite(bounds.z) && std::isfinite(bounds.w);
}

void validateRegion(const CameraFramingRegion& region) {
    if (!finiteBounds(region.bounds) || region.bounds.x > region.bounds.z ||
        region.bounds.y > region.bounds.w) {
        throw std::invalid_argument(
            "CameraFramingRegion bounds must be a finite min/max AABB");
    }
    if (!std::isfinite(region.blendMargin) || region.blendMargin < 0.0f) {
        throw std::invalid_argument(
            "CameraFramingRegion blend margin must be finite and non-negative");
    }
    if (region.zoom && (!std::isfinite(*region.zoom) || *region.zoom <= 0.0f)) {
        throw std::invalid_argument(
            "CameraFramingRegion zoom must be finite and positive");
    }
    if (region.damping &&
        (!std::isfinite(*region.damping) || *region.damping < 0.0f)) {
        throw std::invalid_argument(
            "CameraFramingRegion damping must be finite and non-negative");
    }
    if (region.lookAheadMultiplier && (!std::isfinite(*region.lookAheadMultiplier) ||
                                       *region.lookAheadMultiplier < 0.0f)) {
        throw std::invalid_argument(
            "CameraFramingRegion look-ahead must be finite and non-negative");
    }
    if (region.lockY && !std::isfinite(*region.lockY)) {
        throw std::invalid_argument("CameraFramingRegion lockY must be finite");
    }
}

float mixToward(float base, float target, float weight) {
    return base + (target - base) * weight;
}

} // namespace

void CameraDirector::setBaseline(const Baseline& baseline) {
    if (!std::isfinite(baseline.zoom) || baseline.zoom <= 0.0f ||
        !std::isfinite(baseline.damping) || baseline.damping < 0.0f ||
        !std::isfinite(baseline.lookAheadMultiplier) ||
        baseline.lookAheadMultiplier < 0.0f) {
        throw std::invalid_argument(
            "CameraDirector baseline requires positive zoom and non-negative "
            "damping/look-ahead");
    }
    m_baseline = baseline;
}

int CameraDirector::addRegion(const CameraFramingRegion& region) {
    validateRegion(region);
    const int handle = m_nextHandle++;
    m_entries.push_back(Entry{handle, region});
    return handle;
}

void CameraDirector::removeRegion(int handle) {
    std::erase_if(m_entries,
                  [handle](const Entry& entry) { return entry.handle == handle; });
}

void CameraDirector::clearRegions() {
    m_entries.clear();
}

float CameraDirector::regionWeight(const CameraFramingRegion& region,
                                   const glm::vec2& focus) {
    const glm::vec4& b = region.bounds;
    if (focus.x < b.x || focus.x > b.z || focus.y < b.y || focus.y > b.w) {
        return 0.0f;
    }
    if (region.blendMargin <= 0.0f) {
        return 1.0f;
    }
    const float edgeDistance = std::min(
        std::min(focus.x - b.x, b.z - focus.x),
        std::min(focus.y - b.y, b.w - focus.y));
    return std::clamp(edgeDistance / region.blendMargin, 0.0f, 1.0f);
}

CameraDirector::Resolved CameraDirector::resolve(const glm::vec2& focus) const {
    Resolved resolved{m_baseline.zoom, m_baseline.damping,
                      m_baseline.lookAheadMultiplier, std::nullopt, 0.0f};

    // Blend lower priorities first so higher priorities compose over them;
    // equal priorities compose in authoring order.
    std::vector<const Entry*> active;
    active.reserve(m_entries.size());
    for (const Entry& entry : m_entries) {
        if (regionWeight(entry.region, focus) > 0.0f) {
            active.push_back(&entry);
        }
    }
    std::stable_sort(active.begin(), active.end(),
                     [](const Entry* a, const Entry* b) {
                         return a->region.priority < b->region.priority;
                     });

    for (const Entry* entry : active) {
        const CameraFramingRegion& region = entry->region;
        const float weight = regionWeight(region, focus);
        if (region.zoom) {
            resolved.zoom = mixToward(resolved.zoom, *region.zoom, weight);
        }
        if (region.damping) {
            resolved.damping = mixToward(resolved.damping, *region.damping, weight);
        }
        if (region.lookAheadMultiplier) {
            resolved.lookAheadMultiplier = mixToward(
                resolved.lookAheadMultiplier, *region.lookAheadMultiplier, weight);
        }
        if (region.lockY) {
            const float current = resolved.lockY.value_or(*region.lockY);
            resolved.lockY = mixToward(current, *region.lockY, weight);
            resolved.lockYWeight = std::max(resolved.lockYWeight, weight);
        }
    }
    return resolved;
}

void CameraDirector::apply(Camera& camera, const glm::vec2& focus) const {
    const Resolved resolved = resolve(focus);
    camera.setZoom(resolved.zoom);
    camera.setDamping(resolved.damping);
    camera.setLookAheadMultiplier(resolved.lookAheadMultiplier);
}

void CameraDirector::constrain(Camera& camera, const glm::vec2& focus) const {
    const Resolved resolved = resolve(focus);
    if (!resolved.lockY || resolved.lockYWeight <= 0.0f) {
        return;
    }
    Transform& transform = camera.getTransform();
    glm::vec2 position = transform.Position;
    position.y = mixToward(position.y, *resolved.lockY, resolved.lockYWeight);
    transform.setPos(position);
}
