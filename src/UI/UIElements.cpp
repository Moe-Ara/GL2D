//
// Created by Mohamad on 26/11/2025.
//

#include "UI/UIElements.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glm/common.hpp>

namespace UI {
namespace {
bool finite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool finite(const glm::vec4& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z) && std::isfinite(value.w);
}

void validateTransform(const UITransform& transform, const glm::vec2& canvasSize) {
    if (!finite(canvasSize) || canvasSize.x < 0.0f || canvasSize.y < 0.0f ||
        !finite(transform.position) || !finite(transform.size) ||
        !finite(transform.anchor) || !finite(transform.pivot) ||
        transform.size.x < 0.0f || transform.size.y < 0.0f) {
        throw std::invalid_argument(
            "UI transform and canvas values must be finite; sizes cannot be negative");
    }
}

bool subtreeContains(const UIElement& root, const UIElement* target) {
    if (&root == target) return true;
    for (const auto& child : root.children()) {
        if (child && subtreeContains(*child, target)) return true;
    }
    return false;
}
}

glm::vec2 UITransform::worldPosition(const glm::vec2& canvasSize) const {
    if (!finite(canvasSize) || canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
        throw std::invalid_argument("UI canvas size must be finite and positive");
    }
    return worldPosition(canvasSize, {0.0f, 0.0f});
}

glm::vec2 UITransform::worldPosition(const glm::vec2& referenceSize,
                                     const glm::vec2& referenceOrigin) const {
    validateTransform(*this, referenceSize);
    if (!finite(referenceOrigin)) {
        throw std::invalid_argument("UI reference origin must be finite");
    }
    return referenceOrigin + anchor * referenceSize + position - size * pivot;
}

glm::vec4 UITransform::bounds(const glm::vec2& canvasSize) const {
    const glm::vec2 pos = worldPosition(canvasSize);
    return {pos.x, pos.y, pos.x + size.x, pos.y + size.y};
}

glm::vec4 UITransform::bounds(const glm::vec2& referenceSize,
                              const glm::vec2& referenceOrigin) const {
    const glm::vec2 pos = worldPosition(referenceSize, referenceOrigin);
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
    if (!std::isfinite(dt) || dt < 0.0f || !std::isfinite(lerpSpeed) || lerpSpeed < 0.0f ||
        !finite(normal) || !finite(hover) || !finite(pressed) ||
        !finite(disabled) || !finite(current)) {
        throw std::invalid_argument("UI effect colors, speed, and delta must be finite and non-negative");
    }
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
                     std::shared_ptr<GameObjects::Texture> texture,
                     float zIndex)
    : m_id(std::move(id)),
      m_transform(transform),
      m_effect(effect),
      m_texture(texture),
      m_zIndex(zIndex) {
    if (m_id.empty()) {
        throw std::invalid_argument("UI element id cannot be empty");
    }
    if (!std::isfinite(m_zIndex)) {
        throw std::invalid_argument("UI element z-index must be finite");
    }
    validateTransform(m_transform, {1.0f, 1.0f});
    m_effect.reset();
    m_effect.update(UIState::Idle, 0.0f);
}

void UIElement::setZIndex(float z) {
    if (!std::isfinite(z)) {
        throw std::invalid_argument("UI element z-index must be finite");
    }
    m_zIndex = z;
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
    if (!child) throw std::invalid_argument("UI child cannot be null");
    if (child.get() == this || subtreeContains(*child, this)) {
        throw std::logic_error("UI child relationship would create a cycle");
    }
    if (std::find(m_children.begin(), m_children.end(), child) != m_children.end()) {
        throw std::logic_error("UI child is already attached to this element");
    }
    if (!child->m_parentLifetime.expired()) {
        throw std::logic_error("UI child is already attached to another parent");
    }
    m_children.push_back(child);
    child->m_parentLifetime = m_attachmentLifetime;
}

bool UIElement::contains(const glm::vec2& point, const glm::vec2& canvasSize) const {
    return m_transform.contains(point, canvasSize);
}

glm::vec4 UIElement::bounds(const glm::vec2& canvasSize) const {
    return m_transform.bounds(canvasSize);
}

void UIElement::updateTree(float dt, const UIPointerState& pointer, const glm::vec2& canvasSize) {
    if (!std::isfinite(dt) || dt < 0.0f || !finite(pointer.position)) {
        throw std::invalid_argument("UI update delta and pointer position must be finite; delta cannot be negative");
    }
    if (!finite(canvasSize) || canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
        throw std::invalid_argument("UI canvas size must be finite and positive");
    }
    updateTree(dt, pointer, {0.0f, 0.0f}, canvasSize);
}

void UIElement::updateTree(float dt,
                           const UIPointerState& pointer,
                           const glm::vec2& referenceOrigin,
                           const glm::vec2& referenceSize) {
    validateTransform(m_transform, referenceSize);
    if (!m_visible) {
        return;
    }

    const glm::vec4 resolvedBounds = m_transform.bounds(referenceSize, referenceOrigin);
    updateSelf(dt, pointer, resolvedBounds);
    applyEffect(dt);

    const glm::vec2 childOrigin{resolvedBounds.x, resolvedBounds.y};
    const glm::vec2 childSize{resolvedBounds.z - resolvedBounds.x,
                              resolvedBounds.w - resolvedBounds.y};
    for (auto& child : m_children) {
        if (child) {
            child->updateTree(dt, pointer, childOrigin, childSize);
        }
    }
}

void UIElement::collectRenderTree(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const {
    if (!finite(canvasSize) || canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
        throw std::invalid_argument("UI canvas size must be finite and positive");
    }
    collectRenderTree(out, {0.0f, 0.0f}, canvasSize);
}

void UIElement::collectRenderTree(std::vector<UIRenderCommand>& out,
                                  const glm::vec2& referenceOrigin,
                                  const glm::vec2& referenceSize) const {
    validateTransform(m_transform, referenceSize);
    if (!m_visible) {
        return;
    }

    const glm::vec4 resolvedBounds = m_transform.bounds(referenceSize, referenceOrigin);
    buildRenderCommands(out, resolvedBounds);

    const glm::vec2 childOrigin{resolvedBounds.x, resolvedBounds.y};
    const glm::vec2 childSize{resolvedBounds.z - resolvedBounds.x,
                              resolvedBounds.w - resolvedBounds.y};
    for (const auto& child : m_children) {
        if (child) {
            child->collectRenderTree(out, childOrigin, childSize);
        }
    }
}

void UIElement::updateSelf(float /*dt*/, const UIPointerState& /*pointer*/, const glm::vec4& /*resolvedBounds*/) {
    if (!m_enabled) {
        m_state = UIState::Disabled;
    } else if (m_state == UIState::Disabled) {
        m_state = UIState::Idle;
    }
}

void UIElement::buildRenderCommands(std::vector<UIRenderCommand>& out,
                                    const glm::vec4& resolvedBounds) const {
    UIRenderCommand cmd{};
    cmd.rect = resolvedBounds;
    cmd.color = m_effect.current;
    cmd.texture = m_texture;
    cmd.zIndex = m_zIndex;
    out.push_back(cmd);
}

bool UIElement::containsPoint(const glm::vec2& point, const glm::vec4& resolvedBounds) {
    return point.x >= resolvedBounds.x && point.x <= resolvedBounds.z &&
           point.y >= resolvedBounds.y && point.y <= resolvedBounds.w;
}

void UIElement::applyEffect(float dt) {
    m_effect.update(m_state, dt);
}

UICheckbox::UICheckbox(std::string id, UITransform transform, UIEffect effect,
                       std::shared_ptr<GameObjects::Texture> texture)
    : UIElement(std::move(id), transform, effect, texture) {}

void UICheckbox::updateSelf(float /*dt*/, const UIPointerState& pointer,
                            const glm::vec4& resolvedBounds) {
    if (!m_enabled) {
        setState(UIState::Disabled);
        return;
    }

    const bool hovering = containsPoint(pointer.position, resolvedBounds);
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

void UICheckbox::buildRenderCommands(std::vector<UIRenderCommand>& out,
                                     const glm::vec4& resolvedBounds) const {
    UIElement::buildRenderCommands(out, resolvedBounds);
    if (!checked) {
        return;
    }

    const auto rect = resolvedBounds;
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

UIBar::UIBar(std::string id, UITransform transform, UIEffect effect,
             std::shared_ptr<GameObjects::Texture> texture)
    : UIElement(std::move(id), transform, effect, texture) {}

void UIBar::setValue(float v) {
    if (!std::isfinite(v)) {
        throw std::invalid_argument("UI bar value must be finite");
    }
    m_value = std::clamp(v, 0.0f, 1.0f);
}

void UIBar::buildRenderCommands(std::vector<UIRenderCommand>& out,
                                const glm::vec4& resolvedBounds) const {
    const auto rect = resolvedBounds;
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

UIIcon::UIIcon(std::string id, UITransform transform,
               std::shared_ptr<GameObjects::Texture> texture, UIEffect effect)
    : UIElement(std::move(id), transform, effect, texture) {}

void UIIcon::buildRenderCommands(std::vector<UIRenderCommand>& out,
                                 const glm::vec4& resolvedBounds) const {
    UIRenderCommand cmd{};
    cmd.rect = resolvedBounds;
    cmd.color = tint * m_effect.current;
    cmd.texture = m_texture;
    cmd.zIndex = m_zIndex;
    out.push_back(cmd);
}

UIItem::UIItem(std::string id, UITransform transform, UIEffect effect,
               std::shared_ptr<GameObjects::Texture> texture)
    : UIElement(std::move(id), transform, effect, texture) {}

void UIItem::updateSelf(float /*dt*/, const UIPointerState& pointer,
                        const glm::vec4& resolvedBounds) {
    if (!m_enabled) {
        setState(UIState::Disabled);
        return;
    }

    const bool hovering = containsPoint(pointer.position, resolvedBounds);
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

void UIItem::buildRenderCommands(std::vector<UIRenderCommand>& out,
                                 const glm::vec4& resolvedBounds) const {
    glm::vec4 color = m_effect.current;
    if (selected) {
        color = glm::mix(color, glm::vec4(0.3f, 0.75f, 1.0f, 1.0f), 0.35f);
    }

    UIRenderCommand cmd{};
    cmd.rect = resolvedBounds;
    cmd.color = color;
    cmd.texture = m_texture;
    cmd.zIndex = m_zIndex;
    out.push_back(cmd);
}

UIMenu::UIMenu(std::string id, UITransform transform, UIEffect effect, LayoutDirection direction, float spacing)
    : UIElement(std::move(id), transform, effect, nullptr),
      m_direction(direction),
      m_spacing(0.0f) {
    setSpacing(spacing);
}

void UIMenu::setSpacing(float spacing) {
    if (!std::isfinite(spacing) || spacing < 0.0f) {
        throw std::invalid_argument("UI menu spacing must be finite and non-negative");
    }
    m_spacing = spacing;
}

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

void UIMenu::updateSelf(float /*dt*/, const UIPointerState& /*pointer*/,
                        const glm::vec4& /*resolvedBounds*/) {
    if (!m_enabled) {
        setState(UIState::Disabled);
    } else if (m_state == UIState::Disabled) {
        setState(UIState::Idle);
    }
}

void UIScreen::update(float dt, const UIPointerState& pointer) {
    if (!finite(canvasSize) || canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
        throw std::invalid_argument("UI screen canvas size must be finite and positive");
    }
    for (auto& root : roots) {
        if (root) {
            root->updateTree(dt, pointer, canvasSize);
        }
    }
}

std::vector<UIRenderCommand> UIScreen::collectRenderCommands() const {
    if (!finite(canvasSize) || canvasSize.x <= 0.0f || canvasSize.y <= 0.0f) {
        throw std::invalid_argument("UI screen canvas size must be finite and positive");
    }
    std::vector<UIRenderCommand> commands;
    for (const auto& root : roots) {
        if (root) {
            root->collectRenderTree(commands, canvasSize);
        }
    }
    return commands;
}

} // namespace UI
