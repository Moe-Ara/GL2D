#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <optional>
#include <vector>

class Camera;

// A world-space framing region. While the follow focus (usually the player)
// is inside `bounds`, the region's authored parameters blend over the
// director's baseline. `blendMargin` is the distance from the region's edge
// over which its influence fades in (0 = hard edge). Higher `priority`
// regions blend after (over) lower ones; equal priorities blend in authoring
// order.
struct CameraFramingRegion {
    glm::vec4 bounds{0.0f}; // minX, minY, maxX, maxY (world units)
    int priority{0};
    float blendMargin{0.0f};
    std::optional<float> zoom;
    std::optional<float> damping;
    std::optional<float> lookAheadMultiplier;
    // Rail constraint: pins the camera's Y after update (weighted), giving
    // locked-horizon tracking shots without giving up X follow.
    std::optional<float> lockY;
};

// Blends authored framing regions over a baseline camera configuration.
// Call apply() with the follow focus before Camera::update() each frame, and
// constrain() after it to enforce rail constraints. While a director drives a
// camera, it owns zoom/damping/look-ahead: do not set those elsewhere.
class CameraDirector {
public:
    struct Baseline {
        float zoom{1.0f};
        float damping{6.0f};
        float lookAheadMultiplier{0.0f};
    };

    void setBaseline(const Baseline& baseline);
    [[nodiscard]] const Baseline& baseline() const noexcept { return m_baseline; }

    // Returns a handle usable with removeRegion.
    int addRegion(const CameraFramingRegion& region);
    void removeRegion(int handle);
    void clearRegions();

    void apply(Camera& camera, const glm::vec2& focus) const;
    void constrain(Camera& camera, const glm::vec2& focus) const;

    // Exposed for tests and tooling: the blended values for a focus position.
    struct Resolved {
        float zoom;
        float damping;
        float lookAheadMultiplier;
        std::optional<float> lockY;
        float lockYWeight{0.0f};
    };
    [[nodiscard]] Resolved resolve(const glm::vec2& focus) const;

private:
    struct Entry {
        int handle;
        CameraFramingRegion region;
    };

    [[nodiscard]] static float regionWeight(const CameraFramingRegion& region,
                                            const glm::vec2& focus);

    Baseline m_baseline{};
    std::vector<Entry> m_entries;
    int m_nextHandle{1};
};
