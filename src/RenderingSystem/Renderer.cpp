

//
// Created by Mohamad on 21/11/2025.
//

#include "Renderer.hpp"
#include "GameObjects/Sprite.hpp"
#include "GameObjects/Texture.hpp"
#include "Graphics/Shader.hpp"
#include "RenderingSystem/ColorRenderTarget.hpp"
#include "RenderingSystem/LightingPass.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include "RenderingSystem/PostProcessPipeline.hpp"
#include "RenderingSystem/RenderTarget.hpp"
#include "RenderingSystem/TilemapRenderer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace Rendering {
namespace {
inline Vertex makeVertex(const glm::vec2 &pos, const glm::vec4 &color,
                         const glm::vec2 &uv) {
  Vertex v{};
  v.position = pos;
  v.color = color;
  v.uv = uv;
  return v;
}

bool finite(const glm::vec4& value) {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z) && std::isfinite(value.w);
}

bool finite(const glm::vec2& value) {
  return std::isfinite(value.x) && std::isfinite(value.y);
}

GLuint createSolidTexture(const std::array<unsigned char, 4>& pixel,
                          const char* purpose) {
  GLint previousActiveTexture = 0;
  GLint previousTexture = 0;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

  GLuint texture = 0;
  glGenTextures(1, &texture);
  if (!texture) {
    throw std::runtime_error(std::string("OpenGL failed to allocate ") + purpose);
  }
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixel.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
  glActiveTexture(static_cast<GLenum>(previousActiveTexture));
  return texture;
}

bool finite(const glm::mat4& value) {
  for (int column = 0; column < 4; ++column) {
    for (int row = 0; row < 4; ++row) {
      if (!std::isfinite(value[column][row])) return false;
    }
  }
  return true;
}
} // namespace

Renderer::Renderer(const std::string &vsPath, const std::string &fsPath)
    : m_shader(std::make_shared<Graphics::Shader>(vsPath, fsPath)) {
  try {
    createBuffers();
    createDefaultTexture();
    createDefaultNormalTexture();
  } catch (...) {
    destroyBuffers();
    throw;
  }
}

Renderer::~Renderer() { destroyBuffers(); }

void Renderer::createBuffers() {
  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ibo);
  if (!m_vao || !m_vbo || !m_ibo) {
    throw std::runtime_error("OpenGL failed to allocate renderer buffers");
  }

  GLint previousVertexArray = 0;
  GLint previousArrayBuffer = 0;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, color));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, uv));

  glBindVertexArray(static_cast<GLuint>(previousVertexArray));
  glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
}

void Renderer::destroyBuffers() {
  if (m_defaultTexture) {
    glDeleteTextures(1, &m_defaultTexture);
    m_defaultTexture = 0;
  }
  if (m_defaultNormal) {
    glDeleteTextures(1, &m_defaultNormal);
    m_defaultNormal = 0;
  }
  if (m_vbo) {
    glDeleteBuffers(1, &m_vbo);
    m_vbo = 0;
  }
  if (m_ibo) {
    glDeleteBuffers(1, &m_ibo);
    m_ibo = 0;
  }
  if (m_vao) {
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
  }
}

void Renderer::beginFrame(const glm::mat4 &viewProj,
                          const glm::vec4 &clearColor,
                          bool clearBuffer) {
  if (m_frameActive) {
    throw std::logic_error("Renderer::beginFrame called before the active frame ended");
  }
  if (!finite(viewProj) || !finite(clearColor)) {
    throw std::invalid_argument("Renderer frame matrices and colors must be finite");
  }
  m_viewProj = viewProj;
  m_quads.clear();
  m_frameActive = true;
  if (clearBuffer) {
      glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
      glClear(GL_COLOR_BUFFER_BIT);
  }
}

void Renderer::submitSprite(const GameObjects::Sprite &sprite,
                            const glm::mat4 &model,
                            int layer,
                            int zOrder) {
  SpriteDrawData drawData{};
  drawData.color = sprite.getColor();
  drawData.uvRect = sprite.getUVCoords();
  drawData.flipX = sprite.isFlipX();
  submitSprite(sprite, model, layer, zOrder, drawData);
}

void Renderer::submitSprite(const GameObjects::Sprite &sprite,
                            const glm::mat4 &model,
                            int layer,
                            int zOrder,
                            const SpriteDrawData &drawData) {
  if (!m_frameActive) {
    throw std::logic_error("Renderer::submitSprite requires an active frame");
  }
  const glm::vec2 size = sprite.getSize();
  if (!finite(model) || !finite(drawData.color) || !finite(drawData.uvRect) ||
      !finite(size) || size.x < 0.0f || size.y < 0.0f) {
    throw std::invalid_argument(
        "Renderer sprite transform, size, color, and UVs must be finite; size cannot be negative");
  }
  Quad quad{};
  quad.textureId = drawData.textureOverride
                       ? drawData.textureOverride->getID()
                       : sprite.hasTexture() && sprite.getTexture()
                       ? sprite.getTexture()->getID()
                       : m_defaultTexture;
  quad.normalTextureId = drawData.normalTextureOverride
                         ? drawData.normalTextureOverride->getID()
                         : sprite.hasNormalTexture() && sprite.getNormalTexture()
                         ? sprite.getNormalTexture()->getID()
                         : m_defaultNormal;
  quad.layer = layer;
  quad.zIndex = zOrder;

  const auto &color = drawData.color;
  const auto &uv = drawData.uvRect;
  const std::array<glm::vec2, 4> localPositions = {
      glm::vec2(0.0f, size.y), glm::vec2(size.x, size.y),
      glm::vec2(size.x, 0.0f), glm::vec2(0.0f, 0.0f)};
  const bool flipX = drawData.flipX;
  const float u0 = flipX ? uv.z : uv.x;
  const float u1 = flipX ? uv.x : uv.z;
  const std::array<glm::vec2, 4> uvCoords = {
      glm::vec2(u0, uv.w), glm::vec2(u1, uv.w), glm::vec2(u1, uv.y),
      glm::vec2(u0, uv.y)};

  for (size_t i = 0; i < localPositions.size(); ++i) {
    glm::vec4 world = model * glm::vec4(localPositions[i], 0.0f, 1.0f);
    const glm::vec4 tintedColor = color * m_globalTint;
    quad.verts[i] = makeVertex(glm::vec2(world.x, world.y), tintedColor,
                               uvCoords[i]);
  }

  m_quads.push_back(quad);
}

void Renderer::flush() {
  if (m_quads.empty()) {
    return;
  }

  std::stable_sort(m_quads.begin(), m_quads.end(),
                   [](const Quad &a, const Quad &b) {
                     if (a.layer != b.layer) return a.layer < b.layer;
                     return a.zIndex < b.zIndex;
                   });

  m_shader->enable();
  m_shader->setUniformMat4("projection", m_viewProj);
  m_shader->setUniformMat4("transform", glm::mat4(1.0f));
  m_shader->setUniformInt1("spriteTexture", 0);
  m_shader->setUniformInt1("normalTexture", 1);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindVertexArray(m_vao);

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  constexpr std::size_t maxQuadsPerBatch = std::min(
      static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max() / 4U),
      static_cast<std::size_t>(std::numeric_limits<GLsizei>::max() / 6));
  size_t quadIndex = 0;
  while (quadIndex < m_quads.size()) {
    const GLuint currentTexture = m_quads[quadIndex].textureId;
    const GLuint currentNormal = m_quads[quadIndex].normalTextureId;
    vertices.clear();
    indices.clear();

    for (; quadIndex < m_quads.size() &&
           m_quads[quadIndex].textureId == currentTexture &&
           m_quads[quadIndex].normalTextureId == currentNormal &&
           vertices.size() / 4U < maxQuadsPerBatch;
         ++quadIndex) {
      const auto &quad = m_quads[quadIndex];
      const auto base = static_cast<uint32_t>(vertices.size());
      vertices.insert(vertices.end(), std::begin(quad.verts),
                      std::end(quad.verts));
      indices.insert(indices.end(),
                     {base, static_cast<uint32_t>(base + 1),
                      static_cast<uint32_t>(base + 2),
                      static_cast<uint32_t>(base + 2),
                      static_cast<uint32_t>(base + 3), base});
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() *
                                                          sizeof(Vertex)),
                 vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_DYNAMIC_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, currentNormal);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                   GL_UNSIGNED_INT, nullptr);
  }

  glBindVertexArray(0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  m_quads.clear();
}

void Renderer::endFrame() {
  if (!m_frameActive) {
    throw std::logic_error("Renderer::endFrame called without an active frame");
  }
  try {
    flush();
  } catch (...) {
    m_quads.clear();
    m_frameActive = false;
    throw;
  }
  m_frameActive = false;
}

void Renderer::createDefaultTexture() {
  m_defaultTexture = createSolidTexture({255, 255, 255, 255},
                                        "renderer default texture");
}

void Renderer::createDefaultNormalTexture() {
  m_defaultNormal = createSolidTexture({128, 128, 255, 255},
                                       "renderer default normal texture");
}

void Renderer::applyFeeling(const FeelingsSystem::FeelingSnapshot &snapshot) {
    // Feelings no longer tint the albedo; keep tint neutral.
    (void)snapshot;
    m_globalTint = glm::vec4(1.0f);
}

RenderTarget& Renderer::sceneTarget() {
  if (!m_sceneTarget) m_sceneTarget = std::make_unique<RenderTarget>();
  return *m_sceneTarget;
}

ColorRenderTarget& Renderer::lightingTarget() {
  if (!m_lightingTarget) {
    m_lightingTarget = std::make_unique<ColorRenderTarget>();
  }
  return *m_lightingTarget;
}

PostProcessPipeline& Renderer::postProcessor() {
  if (!m_postProcessor) {
    m_postProcessor = std::make_unique<PostProcessPipeline>();
  }
  return *m_postProcessor;
}

ParticleRenderer& Renderer::particleRenderer() {
  if (!m_particleRenderer) {
    m_particleRenderer = std::make_unique<ParticleRenderer>();
  }
  return *m_particleRenderer;
}

TilemapRenderer& Renderer::tilemapRenderer() {
  if (!m_tilemapRenderer) {
    m_tilemapRenderer = std::make_unique<TilemapRenderer>();
  }
  return *m_tilemapRenderer;
}

LightingPass& Renderer::lightingPass() {
  if (!m_lightingPass) m_lightingPass = std::make_unique<LightingPass>();
  return *m_lightingPass;
}
} // namespace Rendering
