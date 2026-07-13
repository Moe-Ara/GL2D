//
// DebugOverlay.hpp
//

#ifndef GL2D_DEBUGOVERLAY_HPP
#define GL2D_DEBUGOVERLAY_HPP

#include "Engine/Scene.hpp"
#include "Graphics/Camera/Camera.hpp"

namespace Rendering {
class Renderer;
}

class DebugOverlay {
public:
    static void toggle() { s_enabled = !s_enabled; }
    static void setEnabled(bool enabled) { s_enabled = enabled; }
    [[nodiscard]] static bool enabled() { return s_enabled; }
    static void render(const Scene& scene,
                       const Camera& camera,
                       Rendering::Renderer& renderer);
private:
    static bool s_enabled;
};

#endif //GL2D_DEBUGOVERLAY_HPP
