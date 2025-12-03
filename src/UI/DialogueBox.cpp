#include "DialogueBox.hpp"

#include <algorithm>

namespace UI {

DialogueBox::DialogueBox(std::string id, UITransform transform, UIEffect effect, GameObjects::Texture* texture)
    : UIElement(std::move(id), transform, effect, texture) {}

std::string DialogueBox::wrapText(const std::string& text, int maxCharsPerLine) {
    if (maxCharsPerLine <= 0) return text;
    std::string out;
    int currentLen = 0;
    std::string word;
    for (size_t i = 0; i <= text.size(); ++i) {
        const char c = (i < text.size()) ? text[i] : ' ';
        if (c == ' ' || c == '\n' || i == text.size()) {
            const int wordLen = static_cast<int>(word.size());
            if (currentLen > 0 && currentLen + 1 + wordLen > maxCharsPerLine) {
                out.push_back('\n');
                currentLen = 0;
            } else if (currentLen > 0) {
                out.push_back(' ');
                ++currentLen;
            }
            out += word;
            currentLen += wordLen;
            word.clear();
            if (c == '\n') {
                out.push_back('\n');
                currentLen = 0;
            }
        } else {
            word.push_back(c);
        }
    }
    return out;
}

void DialogueBox::buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const {
    if (!m_visible) return;

    // Panel and border
    const glm::vec4 rect = bounds(canvasSize);
    const float x0 = rect.x;
    const float y0 = rect.y;
    const float x1 = rect.z;
    const float y1 = rect.w;

    if (m_drawPanel) {
        // Border (drawn first, slightly expanded)
        const float borderThickness = 2.0f;
        UIRenderCommand border{};
        border.rect = {x0 - borderThickness, y0 - borderThickness, x1 + borderThickness, y1 + borderThickness};
        border.color = m_borderColor;
        border.zIndex = m_zIndex;
        out.push_back(border);

        // Background
        UIRenderCommand bg{};
        bg.rect = rect;
        bg.color = m_bgColor;
        bg.zIndex = m_zIndex + 0.1f;
        out.push_back(bg);
    }

    const float innerX0 = x0 + m_padding.x + m_textOffset.x;
    const float lineHeight = 14.0f * m_fontScale;
    float textY = y1 - m_padding.y - m_textOffset.y - lineHeight;
    const float textWidth = (x1 - x0) - (m_padding.x * 2.0f);
    const int maxCharsPerLine = std::max(1, static_cast<int>(textWidth / (8.0f * m_fontScale)));

    // Speaker line
    if (!m_speaker.empty()) {
        UIRenderCommand speaker{};
        speaker.rect = {innerX0, textY, innerX0, textY}; // position stored in rect.x/y; width/height unused for text
        speaker.color = glm::vec4{m_speakerColor, 1.0f};
        speaker.text = m_speaker + ":";
        speaker.textScale = m_fontScale;
        speaker.zIndex = m_zIndex + 0.2f;
        out.push_back(speaker);
        textY -= lineHeight + m_speakerSpacing * m_fontScale;
    }

    // Body
    if (!m_text.empty()) {
        UIRenderCommand body{};
        body.rect = {innerX0, textY, innerX0, textY};
        body.color = glm::vec4{m_bodyColor, 1.0f};
        body.text = wrapText(m_text, maxCharsPerLine);
        body.textScale = m_fontScale;
        body.zIndex = m_zIndex + 0.2f;
        out.push_back(body);
    }
}

} // namespace UI
