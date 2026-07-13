#ifndef GL2D_TILEMAPRENDERER_HPP
#define GL2D_TILEMAPRENDERER_HPP

#include <glm/mat4x4.hpp>

#include <memory>

class Scene;

namespace Rendering {

// Context-owned tilemap renderer. Its mesh cache and GPU resources live for
// exactly as long as the Renderer instance that owns it.
class TilemapRenderer {
public:
    TilemapRenderer();
    ~TilemapRenderer();

    TilemapRenderer(const TilemapRenderer&) = delete;
    TilemapRenderer& operator=(const TilemapRenderer&) = delete;
    TilemapRenderer(TilemapRenderer&&) = delete;
    TilemapRenderer& operator=(TilemapRenderer&&) = delete;

    void render(Scene& scene, const glm::mat4& viewProjection);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Rendering

#endif // GL2D_TILEMAPRENDERER_HPP
