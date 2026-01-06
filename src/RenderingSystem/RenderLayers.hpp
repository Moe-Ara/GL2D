//
// RenderLayers.hpp
//

#ifndef GL2D_RENDER_LAYERS_HPP
#define GL2D_RENDER_LAYERS_HPP

namespace Rendering {

enum class RenderLayer : int {
    BackgroundFar = 0,
    BackgroundMid = 10,
    BackgroundNear = 20,
    Gameplay = 30,
    Foreground = 40,
    UI = 100
};

} // namespace Rendering

#endif // GL2D_RENDER_LAYERS_HPP
