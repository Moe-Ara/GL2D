#pragma once

#include <GL/glew.h>

namespace Rendering {

class ColorRenderTarget {
public:
    ColorRenderTarget() = default;
    ~ColorRenderTarget();

    ColorRenderTarget(const ColorRenderTarget&) = delete;
    ColorRenderTarget& operator=(const ColorRenderTarget&) = delete;
    ColorRenderTarget(ColorRenderTarget&&) = delete;
    ColorRenderTarget& operator=(ColorRenderTarget&&) = delete;

    void resize(int width, int height);
    void bind() const;
    static void unbind();

    [[nodiscard]] bool isInitialized() const noexcept { return m_framebuffer != 0; }
    [[nodiscard]] GLuint texture() const noexcept { return m_texture; }
    [[nodiscard]] int width() const noexcept { return m_width; }
    [[nodiscard]] int height() const noexcept { return m_height; }

private:
    void destroy() noexcept;
    void allocate(int width, int height);

    GLuint m_framebuffer{0};
    GLuint m_texture{0};
    int m_width{0};
    int m_height{0};
};

} // namespace Rendering
