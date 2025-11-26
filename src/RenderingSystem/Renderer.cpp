

//
// Created by Mohamad on 21/11/2025.
//

#include "Renderer.hpp"
#include "GameObjects/Sprite.hpp"
#include "GameObjects/Texture.hpp"
#include "Graphics/Shader.hpp"
#include <algorithm>
#include <array>
#include <cstddef>

namespace Rendering {
namespace {
inline Vertex makeVertex(const glm::vec2 &pos, const glm::vec3 &color,
                         const glm::vec2 &uv) {
  Vertex v{};
  v.position = pos;
  v.color = color;
  v.uv = uv;
  return v;
}
} // namespace

Renderer::Renderer(const std::string &vsPath, const std::string &fsPath)
    : m_shader(std::make_shared<Graphics::Shader>(vsPath, fsPath)) {
  createBuffers();
  createDefaultTexture();
  createDefaultNormalTexture();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Renderer::~Renderer() { destroyBuffers(); }

void Renderer::createBuffers() {
  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ibo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, color));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, uv));

  glBindVertexArray(0);
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
  m_viewProj = viewProj;
  m_quads.clear();
  if (clearBuffer) {
      glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
      glClear(GL_COLOR_BUFFER_BIT);
  }
}

void Renderer::submitSprite(const GameObjects::Sprite &sprite,
                            const glm::mat4 &model, int zOrder) {
  Quad quad{};
  quad.textureId = sprite.hasTexture() && sprite.getTexture()
                       ? sprite.getTexture()->getID()
                       : m_defaultTexture;
  quad.normalTextureId = sprite.hasNormalTexture() && sprite.getNormalTexture()
                         ? sprite.getNormalTexture()->getID()
                         : m_defaultNormal;
  quad.zIndex = zOrder;

  const auto &color = sprite.getColor();
  const auto &uv = sprite.getUVCoords();
  const auto &size = sprite.getSize();

  const std::array<glm::vec2, 4> localPositions = {
      glm::vec2(0.0f, size.y), glm::vec2(size.x, size.y),
      glm::vec2(size.x, 0.0f), glm::vec2(0.0f, 0.0f)};
  const std::array<glm::vec2, 4> uvCoords = {
      glm::vec2(uv.x, uv.w), glm::vec2(uv.z, uv.w), glm::vec2(uv.z, uv.y),
      glm::vec2(uv.x, uv.y)};

  for (size_t i = 0; i < localPositions.size(); ++i) {
    glm::vec4 world = model * glm::vec4(localPositions[i], 0.0f, 1.0f);
    glm::vec3 tintedColor = glm::vec3(color) * glm::vec3(m_globalTint);
    quad.verts[i] = makeVertex(glm::vec2(world.x, world.y), tintedColor, uvCoords[i]);
  }

  m_quads.push_back(quad);
}

void Renderer::flush() {
  if (m_quads.empty()) {
    return;
  }

  std::stable_sort(m_quads.begin(), m_quads.end(),
                   [](const Quad &a, const Quad &b) {
                     if (a.zIndex == b.zIndex) {
                       if (a.textureId == b.textureId) {
                         return a.normalTextureId < b.normalTextureId;
                       }
                       return a.textureId < b.textureId;
                     }
                     return a.zIndex < b.zIndex;
                   });

  m_shader->enable();
  m_shader->setUniformMat4("projection", m_viewProj);
  m_shader->setUniformMat4("transform", glm::mat4(1.0f));
  m_shader->setUniformInt1("spriteTexture", 0);
  m_shader->setUniformInt1("normalTexture", 1);

  glBindVertexArray(m_vao);

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  size_t quadIndex = 0;
  while (quadIndex < m_quads.size()) {
    const GLuint currentTexture = m_quads[quadIndex].textureId;
    const GLuint currentNormal = m_quads[quadIndex].normalTextureId;
    vertices.clear();
    indices.clear();

    for (; quadIndex < m_quads.size() &&
           m_quads[quadIndex].textureId == currentTexture &&
           m_quads[quadIndex].normalTextureId == currentNormal;
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
  glBindTexture(GL_TEXTURE_2D, 0);
  m_quads.clear();
}

void Renderer::endFrame() { flush(); }

void Renderer::createDefaultTexture() {
  glGenTextures(1, &m_defaultTexture);
  glBindTexture(GL_TEXTURE_2D, m_defaultTexture);
  const unsigned char white[4] = {255, 255, 255, 255};
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::createDefaultNormalTexture() {
  glGenTextures(1, &m_defaultNormal);
  glBindTexture(GL_TEXTURE_2D, m_defaultNormal);
  const unsigned char flat[4] = {128, 128, 255, 255}; // (0,0,1)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, flat);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::applyFeeling(const FeelingsSystem::FeelingSnapshot &snapshot) {
    // Feelings no longer tint the albedo; keep tint neutral.
    (void)snapshot;
    m_globalTint = glm::vec4(1.0f);
}
} // namespace Rendering
