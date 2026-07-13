#include "DebugDraw2D.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat4x4.hpp>

#include "GameObjects/Sprite.hpp"
#include "RenderingSystem/RenderLayers.hpp"
#include "RenderingSystem/Renderer.hpp"

namespace Rendering::DebugDraw2D {
namespace {
constexpr float kMinimumLength = 1e-5f;

bool finite(const glm::vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool finite(const glm::vec4& value) {
    return std::isfinite(value.r) && std::isfinite(value.g) &&
           std::isfinite(value.b) && std::isfinite(value.a);
}

bool validStyle(float thickness, const glm::vec4& color) {
    return std::isfinite(thickness) && thickness > 0.0f && finite(color);
}
}

void line(Renderer& renderer,
          const glm::vec2& from,
          const glm::vec2& to,
          float thickness,
          const glm::vec4& color,
          int zOrder) {
    if (!finite(from) || !finite(to) || !validStyle(thickness, color)) return;
    const glm::vec2 delta = to - from;
    const float length = glm::length(delta);
    if (!std::isfinite(length) || length <= kMinimumLength) return;

    GameObjects::Sprite sprite(glm::vec2{0.0f}, {length, thickness}, glm::vec3(color));
    sprite.setColor(color);
    glm::mat4 model{1.0f};
    model = glm::translate(model, glm::vec3(from, 0.0f));
    model = glm::rotate(model, std::atan2(delta.y, delta.x), {0.0f, 0.0f, 1.0f});
    model = glm::translate(model, {0.0f, -thickness * 0.5f, 0.0f});
    renderer.submitSprite(sprite, model, static_cast<int>(RenderLayer::UI), zOrder);
}

void point(Renderer& renderer,
           const glm::vec2& center,
           float size,
           const glm::vec4& color,
           int zOrder) {
    if (!finite(center) || !validStyle(size, color)) return;
    GameObjects::Sprite sprite(glm::vec2{0.0f}, {size, size}, glm::vec3(color));
    sprite.setColor(color);
    glm::mat4 model{1.0f};
    model = glm::translate(model, glm::vec3(center - glm::vec2{size * 0.5f}, 0.0f));
    renderer.submitSprite(sprite, model, static_cast<int>(RenderLayer::UI), zOrder);
}

void rectangle(Renderer& renderer,
               const glm::vec2& min,
               const glm::vec2& max,
               float thickness,
               const glm::vec4& color,
               int zOrder) {
    if (!finite(min) || !finite(max)) return;
    const glm::vec2 low = glm::min(min, max);
    const glm::vec2 high = glm::max(min, max);
    line(renderer, {low.x, low.y}, {high.x, low.y}, thickness, color, zOrder);
    line(renderer, {high.x, low.y}, {high.x, high.y}, thickness, color, zOrder);
    line(renderer, {high.x, high.y}, {low.x, high.y}, thickness, color, zOrder);
    line(renderer, {low.x, high.y}, {low.x, low.y}, thickness, color, zOrder);
}

void polyline(Renderer& renderer,
              std::span<const glm::vec2> points,
              bool closed,
              float thickness,
              const glm::vec4& color,
              int zOrder) {
    if (points.size() < 2 || !validStyle(thickness, color)) return;
    for (std::size_t i = 1; i < points.size(); ++i) {
        line(renderer, points[i - 1], points[i], thickness, color, zOrder);
    }
    if (closed && points.size() > 2) {
        line(renderer, points.back(), points.front(), thickness, color, zOrder);
    }
}

void circle(Renderer& renderer,
            const glm::vec2& center,
            float radius,
            float thickness,
            const glm::vec4& color,
            int segments,
            int zOrder) {
    if (!finite(center) || !std::isfinite(radius) || radius <= 0.0f ||
        !validStyle(thickness, color)) return;
    segments = std::clamp(segments, 3, 256);
    std::vector<glm::vec2> points;
    points.reserve(static_cast<std::size_t>(segments));
    for (int i = 0; i < segments; ++i) {
        const float angle = glm::two_pi<float>() * static_cast<float>(i) /
                            static_cast<float>(segments);
        points.push_back(center + glm::vec2{std::cos(angle), std::sin(angle)} * radius);
    }
    polyline(renderer, points, true, thickness, color, zOrder);
}

void arrow(Renderer& renderer,
           const glm::vec2& from,
           const glm::vec2& to,
           float thickness,
           const glm::vec4& color,
           int zOrder) {
    if (!finite(from) || !finite(to) || !validStyle(thickness, color)) return;
    const glm::vec2 delta = to - from;
    const float length = glm::length(delta);
    if (!std::isfinite(length) || length <= kMinimumLength) return;
    const glm::vec2 direction = delta / length;
    const glm::vec2 perpendicular{-direction.y, direction.x};
    const float headLength = std::min(length * 0.25f, 18.0f);
    line(renderer, from, to, thickness, color, zOrder);
    line(renderer, to,
         to - direction * headLength + perpendicular * (headLength * 0.4f),
         thickness, color, zOrder);
    line(renderer, to,
         to - direction * headLength - perpendicular * (headLength * 0.4f),
         thickness, color, zOrder);
}

} // namespace Rendering::DebugDraw2D
