//
// RenderTarget.hpp
//

#ifndef GL2D_RENDERTARGET_HPP
#define GL2D_RENDERTARGET_HPP

#include <GL/glew.h>

namespace Rendering {

// Simple off-screen render target (color-only) for post-processing passes.
class RenderTarget {
public:
    RenderTarget() = default;
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    RenderTarget(RenderTarget&&) = delete;
    RenderTarget& operator=(RenderTarget&&) = delete;

    void initialize(int width, int height);
    void resize(int width, int height);
    void bind();
    void unbind();

    GLuint colorTexture() const { return m_colorTex; }
    GLuint normalTexture() const { return m_normalTex; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isInitialized() const { return m_fbo != 0; }

private:
    void destroy();
    void allocate(int width, int height);

    GLuint m_fbo{0};
    GLuint m_colorTex{0};
    GLuint m_normalTex{0};
    int m_width{0};
    int m_height{0};
};

} // namespace Rendering

#endif // GL2D_RENDERTARGET_HPP
