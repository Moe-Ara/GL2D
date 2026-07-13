#ifndef GL2D_UIRENDERER_HPP
#define GL2D_UIRENDERER_HPP

#include <memory>
#include <string>
#include <vector>

#include "UI/UIElements.hpp"

namespace UI {

// Core-profile UI renderer. The instance owns all GL resources and must be
// destroyed before its OpenGL context.
class UIRenderer {
public:
    explicit UIRenderer(const std::string& vertexShader = "Shaders/ui.vert",
                        const std::string& fragmentShader = "Shaders/ui.frag");
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;
    UIRenderer(UIRenderer&&) = delete;
    UIRenderer& operator=(UIRenderer&&) = delete;

    // Draws commands in screen space with a bottom-left origin.
    void render(const std::vector<UIRenderCommand>& commands,
                int framebufferWidth,
                int framebufferHeight);

    // Replaces the current TrueType atlas only after a new atlas is fully built.
    // Returns false when the file cannot be read or baked; bitmap fallback remains available.
    bool setFont(const std::string& ttfPath, float pixelHeight = 32.0f);
    [[nodiscard]] bool hasFont() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI

#endif // GL2D_UIRENDERER_HPP
