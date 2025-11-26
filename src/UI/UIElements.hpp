//
// Created by Mohamad on 26/11/2025.
//

#ifndef GL2D_UIELEMENTS_HPP
#define GL2D_UIELEMENTS_HPP

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "GameObjects/Texture.hpp"

namespace UI {

enum class UIState {
    Idle,
    Hovered,
    Pressed,
    Disabled
};

enum class ButtonMode {
    Trigger,
    Hold,
    Click,
};

struct UIPointerState {
    glm::vec2 position{0.0f};
    bool down{false};
    bool pressed{false};
    bool released{false};
};

struct UITransform {
    glm::vec2 position{0.0f};
    glm::vec2 size{0.0f};
    glm::vec2 anchor{0.0f};
    glm::vec2 pivot{0.0f};

    [[nodiscard]] glm::vec2 worldPosition(const glm::vec2& canvasSize) const;
    [[nodiscard]] glm::vec4 bounds(const glm::vec2& canvasSize) const;
    [[nodiscard]] bool contains(const glm::vec2& point, const glm::vec2& canvasSize) const;
};

struct UIEffect {
    glm::vec4 normal{1.0f};
    glm::vec4 hover{1.0f};
    glm::vec4 pressed{1.0f};
    glm::vec4 disabled{0.45f, 0.45f, 0.45f, 0.75f};
    float lerpSpeed{12.0f};
    glm::vec4 current{1.0f};

    void reset();
    void update(UIState state, float dt);
};

struct UIRenderCommand {
    glm::vec4 rect{}; // x0, y0, x1, y1 in screen space
    glm::vec4 color{1.0f};
    GameObjects::Texture* texture{nullptr}; // optional, non-owning
    float zIndex{0.0f};
};

class UIElement {
public:
    UIElement(std::string id,
              UITransform transform = {},
              UIEffect effect = {},
              GameObjects::Texture* texture = nullptr,
              float zIndex = 1000.0f);
    virtual ~UIElement() = default;

    UIElement(const UIElement& other) = delete;
    UIElement& operator=(const UIElement& other) = delete;
    UIElement(UIElement&& other) = delete;
    UIElement& operator=(UIElement&& other) = delete;

    const std::string& id() const { return m_id; }
    UIState state() const { return m_state; }
    bool isVisible() const { return m_visible; }
    bool isEnabled() const { return m_enabled; }
    float zIndex() const { return m_zIndex; }
    void setZIndex(float z) { m_zIndex = z; }

    UITransform& transform() { return m_transform; }
    const UITransform& transform() const { return m_transform; }
    UIEffect& effect() { return m_effect; }
    const UIEffect& effect() const { return m_effect; }

    void setVisible(bool visible) { m_visible = visible; }
    void setEnabled(bool enabled);
    void setTexture(GameObjects::Texture* texture) { m_texture = texture; }

    void addChild(const std::shared_ptr<UIElement>& child);
    const std::vector<std::shared_ptr<UIElement>>& children() const { return m_children; }
    UIElement* parent() const { return m_parent; }

    [[nodiscard]] bool contains(const glm::vec2& point, const glm::vec2& canvasSize) const;
    [[nodiscard]] glm::vec4 bounds(const glm::vec2& canvasSize) const;

    void updateTree(float dt, const UIPointerState& pointer, const glm::vec2& canvasSize);
    void collectRenderTree(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const;

protected:
    virtual void updateSelf(float dt, const UIPointerState& pointer, const glm::vec2& canvasSize);
    virtual void buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const;

    void applyEffect(float dt);
    void setState(UIState newState) { m_state = newState; }

    std::string m_id;
    UITransform m_transform{};
    UIEffect m_effect{};
    GameObjects::Texture* m_texture{nullptr};
    UIState m_state{UIState::Idle};
    bool m_visible{true};
    bool m_enabled{true};
    float m_zIndex{1000.0f};

private:
    UIElement* m_parent{nullptr};
    std::vector<std::shared_ptr<UIElement>> m_children;
};

template<typename... Args>
class Button : public UIElement {
public:
    using Callback = std::function<void(Args...)>;

    Button(std::string id,
           UITransform transform = {},
           Callback cb = nullptr,
           ButtonMode mode = ButtonMode::Click,
           GameObjects::Texture* texture = nullptr,
           UIEffect effect = {})
        : UIElement(std::move(id), transform, effect, texture),
          m_callback(std::move(cb)),
          m_mode(mode) {}

    void setCallback(Callback cb) { m_callback = std::move(cb); }
    void setMode(ButtonMode mode) { m_mode = mode; }
    void setDefaultArgs(Args... args) { m_boundArgs = std::make_tuple(std::forward<Args>(args)...); }

    void updateSelf(float dt, const UIPointerState& pointer, const glm::vec2& canvasSize) override {
        if (!m_enabled) {
            setState(UIState::Disabled);
            return;
        }

        const bool hovering = contains(pointer.position, canvasSize);
        const bool pressedInside = hovering && pointer.pressed;
        const bool releasedInside = hovering && pointer.released;

        if (hovering) {
            setState(pointer.down ? UIState::Pressed : UIState::Hovered);
        } else {
            setState(UIState::Idle);
        }

        bool fire = false;
        switch (m_mode) {
            case ButtonMode::Trigger: fire = pressedInside; break;
            case ButtonMode::Hold: fire = hovering && pointer.down; break;
            case ButtonMode::Click: fire = releasedInside; break;
        }

        if (fire) {
            invoke();
        }
    }

private:
    void invoke() {
        if (!m_callback) return;
        if constexpr (sizeof...(Args) == 0) {
            m_callback();
        } else if (m_boundArgs) {
            std::apply(m_callback, *m_boundArgs);
        }
    }

    Callback m_callback{};
    ButtonMode m_mode{ButtonMode::Click};
    std::optional<std::tuple<Args...>> m_boundArgs{};
};

class UICheckbox : public UIElement {
public:
    explicit UICheckbox(std::string id,
                        UITransform transform = {},
                        UIEffect effect = {},
                        GameObjects::Texture* texture = nullptr);

    std::function<void(bool)> onToggle;
    bool checked{false};

protected:
    void updateSelf(float dt, const UIPointerState& pointer, const glm::vec2& canvasSize) override;
    void buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const override;

private:
    float m_checkInset{4.0f};
    glm::vec4 m_checkColor{0.15f, 0.85f, 0.35f, 1.0f};
};

class UIBar : public UIElement {
public:
    explicit UIBar(std::string id,
                   UITransform transform = {},
                   UIEffect effect = {},
                   GameObjects::Texture* texture = nullptr);

    void setValue(float v);
    float value() const { return m_value; }

    glm::vec4 background{0.12f, 0.12f, 0.12f, 0.8f};
    glm::vec4 fill{0.25f, 0.65f, 1.0f, 0.95f};

protected:
    void buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const override;

private:
    float m_value{0.0f};
};

class UIIcon : public UIElement {
public:
    explicit UIIcon(std::string id,
                    UITransform transform = {},
                    GameObjects::Texture* texture = nullptr,
                    UIEffect effect = {});

    glm::vec4 tint{1.0f};

protected:
    void buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const override;
};

class UIItem : public UIElement {
public:
    explicit UIItem(std::string id,
                    UITransform transform = {},
                    UIEffect effect = {},
                    GameObjects::Texture* texture = nullptr);

    std::string label;
    bool selectable{true};
    bool selected{false};
    std::function<void()> onActivate;

protected:
    void updateSelf(float dt, const UIPointerState& pointer, const glm::vec2& canvasSize) override;
    void buildRenderCommands(std::vector<UIRenderCommand>& out, const glm::vec2& canvasSize) const override;
};

class UIMenu : public UIElement {
public:
    enum class LayoutDirection { Vertical, Horizontal };

    explicit UIMenu(std::string id,
                    UITransform transform = {},
                    UIEffect effect = {},
                    LayoutDirection direction = LayoutDirection::Vertical,
                    float spacing = 6.0f);

    void relayout();
    void setSpacing(float spacing) { m_spacing = spacing; }
    void setDirection(LayoutDirection dir) { m_direction = dir; }

protected:
    void updateSelf(float dt, const UIPointerState& pointer, const glm::vec2& canvasSize) override;

private:
    LayoutDirection m_direction;
    float m_spacing;
};

struct UIScreen {
    glm::vec2 canvasSize{1280.0f, 720.0f};
    glm::vec4 clearColor{0.0f};
    std::vector<std::shared_ptr<UIElement>> roots;

    void update(float dt, const UIPointerState& pointer);
    [[nodiscard]] std::vector<UIRenderCommand> collectRenderCommands() const;
};

} // namespace UI

#endif //GL2D_UIELEMENTS_HPP
