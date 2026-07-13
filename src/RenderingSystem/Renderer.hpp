//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_RENDERER_HPP
#define GL2D_RENDERER_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <vector>
#include <GL/glew.h>
#include "GameObjects/Vertex.hpp"
#include "Graphics/Shader.hpp"
#include "GameObjects/Sprite.hpp"
#include "FeelingsSystem/FeelingSnapshot.hpp"
#include "RenderingSystem/RenderLayers.hpp"

class RenderSystem;

namespace Rendering {

class ColorRenderTarget;
class LightingPass;
class ParticleRenderer;
class PostProcessPipeline;
class RenderTarget;
class TilemapRenderer;

struct SpriteDrawData {
  glm::vec4 color{1.0f};
  glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};
  bool flipX{false};
  const GameObjects::Texture* textureOverride{nullptr};
  const GameObjects::Texture* normalTextureOverride{nullptr};
};

class Renderer {
public:
  explicit Renderer(const std::string &vsPath = "Shaders/vertex.vert",
           const std::string &fsPath = "Shaders/fragment.frag");
  ~Renderer();

  Renderer(const Renderer &other) = delete;
  Renderer &operator=(const Renderer &other) = delete;
  Renderer(Renderer &&other) = delete;
  Renderer &operator=(Renderer &&other) = delete;

  void beginFrame(const glm::mat4 &viewProj,
                  const glm::vec4 &clearColor = {0.f, 0.f, 0.f, 1.f},
                  bool clearBuffer = true);
  void submitSprite(const GameObjects::Sprite &sprite, const glm::mat4 &model,
                    int layer = static_cast<int>(RenderLayer::Gameplay),
                    int zOrder = 0);
  void submitSprite(const GameObjects::Sprite &sprite, const glm::mat4 &model,
                    int layer, int zOrder, const SpriteDrawData& drawData);
  void endFrame();
  void applyFeeling(const FeelingsSystem::FeelingSnapshot& snapshot);

private:
  friend class ::RenderSystem;

  struct Quad {
    GLuint textureId{0};
    GLuint normalTextureId{0};
    int layer{static_cast<int>(RenderLayer::Gameplay)};
    int zIndex{0};
    Vertex verts[4];
  };
  void createDefaultTexture();
  void createDefaultNormalTexture();

  void createBuffers();
  void destroyBuffers();
  void flush();

  RenderTarget& sceneTarget();
  ColorRenderTarget& lightingTarget();
  PostProcessPipeline& postProcessor();
  ParticleRenderer& particleRenderer();
  TilemapRenderer& tilemapRenderer();
  LightingPass& lightingPass();

  std::shared_ptr<Graphics::Shader> m_shader;
  GLuint m_vao{}, m_vbo{}, m_ibo{};
  GLuint m_defaultTexture{0};
  GLuint m_defaultNormal{0};
  glm::mat4 m_viewProj{1.0f};
  std::vector<Quad> m_quads;
  glm::vec4 m_globalTint{1.0f, 1.0f, 1.0f, 1.0f};
  bool m_frameActive{false};
  std::unique_ptr<RenderTarget> m_sceneTarget;
  std::unique_ptr<ColorRenderTarget> m_lightingTarget;
  std::unique_ptr<PostProcessPipeline> m_postProcessor;
  std::unique_ptr<ParticleRenderer> m_particleRenderer;
  std::unique_ptr<TilemapRenderer> m_tilemapRenderer;
  std::unique_ptr<LightingPass> m_lightingPass;
};

} // namespace Rendering

#endif // GL2D_RENDERER_HPP
