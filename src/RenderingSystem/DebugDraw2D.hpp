#pragma once

#include <span>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace Rendering {
class Renderer;

// Small world-space diagnostic primitives submitted through Renderer. These
// functions own no GL state and are safe in a core-profile context.
namespace DebugDraw2D {

void line(Renderer& renderer,
          const glm::vec2& from,
          const glm::vec2& to,
          float thickness,
          const glm::vec4& color,
          int zOrder = 10'000);

void point(Renderer& renderer,
           const glm::vec2& center,
           float size,
           const glm::vec4& color,
           int zOrder = 10'000);

void rectangle(Renderer& renderer,
               const glm::vec2& min,
               const glm::vec2& max,
               float thickness,
               const glm::vec4& color,
               int zOrder = 10'000);

void polyline(Renderer& renderer,
              std::span<const glm::vec2> points,
              bool closed,
              float thickness,
              const glm::vec4& color,
              int zOrder = 10'000);

void circle(Renderer& renderer,
            const glm::vec2& center,
            float radius,
            float thickness,
            const glm::vec4& color,
            int segments = 32,
            int zOrder = 10'000);

void arrow(Renderer& renderer,
           const glm::vec2& from,
           const glm::vec2& to,
           float thickness,
           const glm::vec4& color,
           int zOrder = 10'000);

} // namespace DebugDraw2D
} // namespace Rendering
