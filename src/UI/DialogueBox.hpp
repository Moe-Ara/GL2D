#ifndef GL2D_DIALOGUEBOX_HPP
#define GL2D_DIALOGUEBOX_HPP

#include "UIElements.hpp"

namespace UI {

// Simple dialogue box UI element that produces render commands for a panel and text.
class DialogueBox : public UIElement {
public:
    explicit DialogueBox(std::string id,
                         UITransform transform = {},
                         UIEffect effect = {},
                         GameObjects::Texture* texture = nullptr);

    void setSpeaker(const std::string& speaker) { m_speaker = speaker; }
    void setText(const std::string& text) { m_text = text; }
    void setPadding(float pad) { m_padding = {pad, pad}; }
    void setPadding(const glm::vec2& pad) { m_padding = pad; }
    void setTextOffset(const glm::vec2& offset) { m_textOffset = offset; }
    void setFontScale(float scale) { m_fontScale = scale; }
    void setDrawPanel(bool draw) { m_drawPanel = draw; }
    void setSpeakerSpacing(float spacing) { m_speakerSpacing = spacing; }
    void setColors(const glm::vec4& bg, const glm::vec4& border, const glm::vec3& speaker, const glm::vec3& body) {
        m_bgColor = bg;
        m_borderColor = border;
        m_speakerColor = speaker;
        m_bodyColor = body;
    }

    void buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const override;

private:
    static std::string wrapText(const std::string& text, int maxCharsPerLine);

    std::string m_speaker;
    std::string m_text;
    glm::vec2 m_padding{12.0f, 12.0f};
    glm::vec2 m_textOffset{0.0f, 0.0f};
    float m_fontScale{1.4f};
    bool m_drawPanel{true};
    float m_speakerSpacing{6.0f}; // extra vertical space between speaker and body (unscaled)
    glm::vec4 m_bgColor{0.03f, 0.03f, 0.05f, 0.9f};
    glm::vec4 m_borderColor{0.25f, 0.7f, 1.0f, 1.0f};
    glm::vec3 m_speakerColor{0.45f, 0.9f, 1.0f};
    glm::vec3 m_bodyColor{0.95f, 0.95f, 0.95f};
};

} // namespace UI

#endif // GL2D_DIALOGUEBOX_HPP
