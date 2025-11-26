//
// Created by Mohamad on 26/11/2025.
//

#include "UI/UIElements.hpp"

#include <algorithm>

#include <glm/common.hpp>

namespace UI {

glm::vec2 UITransform::worldPosition(const glm::vec2& canvasSize) const {
    return anchor * canvasSize + position - size * pivot;
}

glm::vec4 UITransform::bounds(const glm::vec2& canvasSize) const {
    const glm::vec2 pos = worldPosition(canvasSize);
    return {pos.x, pos.y, pos.x + size.x, pos.y + size.y};
}

bool UITransform::contains(const glm::vec2& point, const glm::vec2& canvasSize) const {
    const auto rect = bounds(canvasSize);
    return point.x >= rect.x && point.x <= rect.z &&
           point.y >= rect.y && point.y <= rect.w;
}

void UIEffect::reset() {
    current = normal;
}

void UIEffect::update(UIState state, float dt) {
    glm::vec4 target = normal;
    switch (state) {
        case UIState::Hovered: target = hover; break;
        case UIState::Pressed: target = pressed; break;
        case UIState::Disabled: target = disabled; break;
        case UIState::Idle: default: break;
    }

    const float t = glm::clamp(dt * lerpSpeed, 0.0f, 1.0f);
    current = glm::mix(current, target, t);
}

UIElement::UIElement(std::string id,
                     UITransform transform,
                     UIEffect effect,
                     GameObjects::Texture* texture,
                     float zIndex)
    : m_id(std::move(id)),
      m_transform(transform),
      m_effect(effect),
      m_texture(texture),
      m_zIndex(zIndex) {
    m_effect.reset();
}

void UIElement::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (!m_enabled) {
        m_state = UIState::Disabled;
    } else if (m_state == UIState::Disabled) {
        m_state = UIState::Idle;
    }
}

void UIElement::addChild(const std::shared_ptr<UIElement>& child) {
    if (!child) return;
    child->m_parent = this;
    m_children.push_back(child);
}

bool UIElement::contains(const glm::vec2& point, const glm::vec2& canvasSize) const {
    return m_transform.contains(point, canvasSize);
}

glm::vec4 UIElement::bounds(const glm::vec2& canvasSize) const {
    return m_transform.bounds(canvasSize);
}

void UIElement::updateTree(float dt, const UIPointerState& pointer, const glm::vec2& canvasSize) {
    if (!m_visible) {
        return;
    }

    updateSelf(dt, pointer, canvasSize);
    applyEffect(dt);

    for (auto& child : m_children) {
        if (child) {
            child->updateTree(dt, pointer, canvasSize);
        }
    }
}

void UIElement::collectRenderTree(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const {
    if (!m_visible) {
        return;
    }

    buildRenderCommands(out, canvasSize);
    for (const auto& child : m_children) {
        if (child) {
            child->collectRenderTree(out, canvasSize);
        }
    }
}

void UIElement::updateSelf(float /*dt*/, const UIPointerState& /*pointer*/, const glm::vec2& /*canvasSize*/) {
    if (!m_enabled) {
        m_state = UIState::Disabled;
    } else if (m_state == UIState::Disabled) {
        m_state = UIState::Idle;
    }
}

void UIElement::buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const {
    UIRenderCommand cmd{};
    cmd.rect = m_transform.bounds(canvasSize);
    cmd.color = m_effect.current;
    cmd.texture = m_texture;
    cmd.zIndex = m_zIndex;
    out.push_back(cmd);
}

void UIElement::applyEffect(float dt) {
    m_effect.update(m_state, dt);
}

UICheckbox::UICheckbox(std::string id, UITransform transform, UIEffect effect, GameObjects::Texture* texture)
    : UIElement(std::move(id), transform, effect, texture) {}

void UICheckbox::updateSelf(float /*dt*/, const UIPointerState& pointer, const glm::vec2& canvasSize) {
    if (!m_enabled) {
        setState(UIState::Disabled);
        return;
    }

    const bool hovering = contains(pointer.position, canvasSize);
    if (hovering) {
        setState(pointer.down ? UIState::Pressed : UIState::Hovered);
    } else {
        setState(UIState::Idle);
    }

    if (hovering && pointer.released) {
        checked = !checked;
        if (onToggle) {
            onToggle(checked);
        }
    }
}

void UICheckbox::buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const {
    UIElement::buildRenderCommands(out, canvasSize);
    if (!checked) {
        return;
    }

    const auto rect = bounds(canvasSize);
    const float availableX = std::max(0.0f, rect.z - rect.x);
    const float availableY = std::max(0.0f, rect.w - rect.y);
    const float inset = std::min(m_checkInset, std::min(availableX, availableY) * 0.45f);

    UIRenderCommand check{};
    check.rect = {rect.x + inset, rect.y + inset, rect.z - inset, rect.w - inset};
    check.color = m_checkColor * m_effect.current;
    check.texture = nullptr;
    check.zIndex = m_zIndex + 0.01f;
    out.push_back(check);
}

UIBar::UIBar(std::string id, UITransform transform, UIEffect effect, GameObjects::Texture* texture)
    : UIElement(std::move(id), transform, effect, texture) {}

void UIBar::setValue(float v) {
    m_value = std::clamp(v, 0.0f, 1.0f);
}

void UIBar::buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const {
    const auto rect = bounds(canvasSize);
    const glm::vec4 effectColor = m_effect.current;

    if (background.a > 0.0f) {
        UIRenderCommand bg{};
        bg.rect = rect;
        bg.color = background * effectColor;
        bg.texture = m_texture;
        bg.zIndex = m_zIndex;
        out.push_back(bg);
    }

    if (m_value <= 0.0f) {
        return;
    }

    UIRenderCommand fillCmd{};
    fillCmd.rect = rect;
    fillCmd.rect.z = rect.x + (rect.z - rect.x) * m_value;
    fillCmd.color = fill * effectColor;
    fillCmd.texture = m_texture;
    fillCmd.zIndex = m_zIndex + 0.01f;
    out.push_back(fillCmd);
}

UIIcon::UIIcon(std::string id, UITransform transform, GameObjects::Texture* texture, UIEffect effect)
    : UIElement(std::move(id), transform, effect, texture) {}

void UIIcon::buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const {
    UIRenderCommand cmd{};
    cmd.rect = bounds(canvasSize);
    cmd.color = tint * m_effect.current;
    cmd.texture = m_texture;
    cmd.zIndex = m_zIndex;
    out.push_back(cmd);
}

UIItem::UIItem(std::string id, UITransform transform, UIEffect effect, GameObjects::Texture* texture)
    : UIElement(std::move(id), transform, effect, texture) {}

void UIItem::updateSelf(float /*dt*/, const UIPointerState& pointer, const glm::vec2& canvasSize) {
    if (!m_enabled) {
        setState(UIState::Disabled);
        return;
    }

    const bool hovering = contains(pointer.position, canvasSize);
    const bool pressed = hovering && pointer.down;

    if (hovering) {
        setState(pressed ? UIState::Pressed : UIState::Hovered);
    } else if (selected) {
        setState(UIState::Pressed);
    } else {
        setState(UIState::Idle);
    }

    if (hovering && pointer.released && selectable) {
        selected = true;
        if (onActivate) {
            onActivate();
        }
    }
}

void UIItem::buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const {
    glm::vec4 color = m_effect.current;
    if (selected) {
        color = glm::mix(color, glm::vec4(0.3f, 0.75f, 1.0f, 1.0f), 0.35f);
    }

    UIRenderCommand cmd{};
    cmd.rect = bounds(canvasSize);
    cmd.color = color;
    cmd.texture = m_texture;
    cmd.zIndex = m_zIndex;
    out.push_back(cmd);
}

UIMenu::UIMenu(std::string id, UITransform transform, UIEffect effect, LayoutDirection direction, float spacing)
    : UIElement(std::move(id), transform, effect, nullptr),
      m_direction(direction),
      m_spacing(spacing) {}

void UIMenu::relayout() {
    glm::vec2 cursor{0.0f};
    for (const auto& child : children()) {
        if (!child) continue;
        auto& tx = child->transform();
        tx.anchor = {0.0f, 0.0f};
        tx.pivot = {0.0f, 0.0f};
        tx.position = cursor;

        if (m_direction == LayoutDirection::Vertical) {
            cursor.y += tx.size.y + m_spacing;
        } else {
            cursor.x += tx.size.x + m_spacing;
        }
    }
}

void UIMenu::updateSelf(float /*dt*/, const UIPointerState& /*pointer*/, const glm::vec2& /*canvasSize*/) {
    if (!m_enabled) {
        setState(UIState::Disabled);
    } else if (m_state == UIState::Disabled) {
        setState(UIState::Idle);
    }
}

void UIScreen::update(float dt, const UIPointerState& pointer) {
    for (auto& root : roots) {
        if (root) {
            root->updateTree(dt, pointer, canvasSize);
        }
    }
}

std::vector<UIRenderCommand> UIScreen::collectRenderCommands() const {
    std::vector<UIRenderCommand> commands;
    for (const auto& root : roots) {
        if (root) {
            root->collectRenderTree(commands, canvasSize);
        }
    }
    return commands;
}

} // namespace UI
