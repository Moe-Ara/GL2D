//
// TilemapRenderer.hpp
//

#ifndef GL2D_TILEMAPRENDERER_HPP
#define GL2D_TILEMAPRENDERER_HPP

#include <glm/mat4x4.hpp>

class Scene;
class Camera;

namespace Rendering {
    class TilemapRenderer {
    public:
        TilemapRenderer() = delete;
        ~TilemapRenderer() = delete;
        TilemapRenderer(const TilemapRenderer&) = delete;
        TilemapRenderer& operator=(const TilemapRenderer&) = delete;
        TilemapRenderer(TilemapRenderer&&) = delete;
        TilemapRenderer& operator=(TilemapRenderer&&) = delete;

        static void render(Scene& scene, Camera& camera, const glm::mat4& viewProj);
    };
}

#endif // GL2D_TILEMAPRENDERER_HPP
