#ifndef GL2D_UIRENDERER_HPP
#define GL2D_UIRENDERER_HPP

#include <vector>
#include <string>
#include "UI/UIElements.hpp"

namespace UI {

// Simple immediate-mode UI renderer for UIRenderCommand lists.
class UIRenderer {
public:
    // Draws UI commands in screen space (origin bottom-left) over the current backbuffer.
    static void render(const std::vector<UIRenderCommand>& commands, int fbWidth, int fbHeight);
    // Load a TrueType font for UI text (fallbacks to built-in bitmap if load fails).
    static bool setFont(const std::string& ttfPath, float pixelHeight = 32.0f);
};

} // namespace UI

#endif // GL2D_UIRENDERER_HPP
