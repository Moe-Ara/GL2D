#include "RenderingSystem/TilemapRenderer.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Engine/Scene.hpp"
#include "GameObjects/Components/TilemapComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Vertex.hpp"
#include "Graphics/Shader.hpp"
#include "LevelBuildingSystem/Tileset.hpp"
#include "Managers/TilesetManager.hpp"

namespace Rendering {

struct TilemapRenderer::Impl {
    struct TilemapCache {
        GLuint vao{0};
        GLuint vbo{0};
        GLuint ibo{0};
        GLsizei indexCount{0};
        std::weak_ptr<TilemapData> data;
        std::weak_ptr<Tileset> tileset;
        glm::ivec2 dimensions{0, 0};
        glm::vec2 tileSize{0.0f};
    };

    ~Impl() {
        for (auto& [component, cache] : caches) {
            (void)component;
            destroyCache(cache);
        }
        if (defaultNormal) glDeleteTextures(1, &defaultNormal);
    }

    static void destroyCache(TilemapCache& cache) noexcept {
        if (cache.vbo) glDeleteBuffers(1, &cache.vbo);
        if (cache.ibo) glDeleteBuffers(1, &cache.ibo);
        if (cache.vao) glDeleteVertexArrays(1, &cache.vao);
        cache.vbo = 0;
        cache.ibo = 0;
        cache.vao = 0;
        cache.indexCount = 0;
    }

    static void ensureBuffers(TilemapCache& cache) {
        if (cache.vao) return;
        glGenVertexArrays(1, &cache.vao);
        glGenBuffers(1, &cache.vbo);
        glGenBuffers(1, &cache.ibo);
        if (!cache.vao || !cache.vbo || !cache.ibo) {
            destroyCache(cache);
            throw std::runtime_error("OpenGL failed to allocate tilemap buffers");
        }

        GLint previousVertexArray = 0;
        GLint previousArrayBuffer = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
        glBindVertexArray(cache.vao);
        glBindBuffer(GL_ARRAY_BUFFER, cache.vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cache.ibo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsetof(Vertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsetof(Vertex, uv)));
        glBindVertexArray(static_cast<GLuint>(previousVertexArray));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    }

    static bool needsRebuild(const TilemapCache& cache,
                             const std::shared_ptr<TilemapData>& data,
                             const std::shared_ptr<Tileset>& tileset) {
        return !data || !tileset || cache.data.lock() != data ||
               cache.tileset.lock() != tileset ||
               cache.dimensions != glm::ivec2(data->width, data->height) ||
               cache.tileSize != data->tileSize;
    }

    static void buildMesh(TilemapCache& cache,
                          const std::shared_ptr<TilemapData>& data,
                          const std::shared_ptr<Tileset>& tileset) {
        if (!data || !tileset || data->width <= 0 || data->height <= 0 ||
            !std::isfinite(data->tileSize.x) || !std::isfinite(data->tileSize.y) ||
            data->tileSize.x <= 0.0f || data->tileSize.y <= 0.0f) {
            throw std::invalid_argument("Tilemap dimensions and tile size must be positive and finite");
        }
        const std::size_t cellCount = static_cast<std::size_t>(data->width) *
                                      static_cast<std::size_t>(data->height);
        if (cellCount > std::numeric_limits<std::uint32_t>::max() / 4U ||
            cellCount > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) / 6U) {
            throw std::length_error("Tilemap mesh exceeds OpenGL index limits");
        }

        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        vertices.reserve(cellCount * 4U);
        indices.reserve(cellCount * 6U);

        for (int y = 0; y < data->height; ++y) {
            for (int x = 0; x < data->width; ++x) {
                const std::size_t cell = static_cast<std::size_t>(y) *
                                         static_cast<std::size_t>(data->width) +
                                         static_cast<std::size_t>(x);
                if (cell >= data->tiles.size()) continue;
                const int tileIndex = data->tiles[cell];
                if (tileIndex < 0 ||
                    static_cast<std::size_t>(tileIndex) >= tileset->uvs.size()) {
                    continue;
                }
                const glm::vec4 uv = tileset->getUV(tileIndex);
                const glm::vec2 base{
                    static_cast<float>(x) * data->tileSize.x,
                    static_cast<float>(y) * data->tileSize.y};
                const std::uint32_t first = static_cast<std::uint32_t>(vertices.size());
                const glm::vec4 white{1.0f};
                vertices.push_back({{base.x, base.y + data->tileSize.y}, white,
                                    {uv.x, uv.w}});
                vertices.push_back({{base.x + data->tileSize.x,
                                     base.y + data->tileSize.y}, white, {uv.z, uv.w}});
                vertices.push_back({{base.x + data->tileSize.x, base.y}, white,
                                    {uv.z, uv.y}});
                vertices.push_back({base, white, {uv.x, uv.y}});
                indices.insert(indices.end(),
                    {first, first + 1U, first + 2U,
                     first + 2U, first + 3U, first});
            }
        }

        ensureBuffers(cache);
        GLint previousVertexArray = 0;
        GLint previousArrayBuffer = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
        glBindVertexArray(cache.vao);
        glBindBuffer(GL_ARRAY_BUFFER, cache.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                     vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cache.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t)),
                     indices.data(), GL_STATIC_DRAW);
        glBindVertexArray(static_cast<GLuint>(previousVertexArray));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));

        cache.data = data;
        cache.tileset = tileset;
        cache.dimensions = {data->width, data->height};
        cache.tileSize = data->tileSize;
        cache.indexCount = static_cast<GLsizei>(indices.size());
    }

    void ensureSharedResources() {
        if (!shader) {
            shader = std::make_shared<Graphics::Shader>(
                "Shaders/vertex.vert", "Shaders/fragment.frag");
        }
        if (defaultNormal) return;

        GLint previousActiveTexture = 0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0);
        GLint previousTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
        glGenTextures(1, &defaultNormal);
        if (!defaultNormal) {
            glActiveTexture(static_cast<GLenum>(previousActiveTexture));
            throw std::runtime_error("OpenGL failed to allocate tilemap default normal texture");
        }
        glBindTexture(GL_TEXTURE_2D, defaultNormal);
        constexpr unsigned char flat[4] = {128, 128, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, flat);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    }

    std::unordered_map<const TilemapComponent*, TilemapCache> caches;
    std::shared_ptr<Graphics::Shader> shader;
    GLuint defaultNormal{0};
};

TilemapRenderer::TilemapRenderer() : m_impl(std::make_unique<Impl>()) {}
TilemapRenderer::~TilemapRenderer() = default;

void TilemapRenderer::render(Scene& scene, const glm::mat4& viewProjection) {
    std::unordered_set<const TilemapComponent*> liveComponents;
    for (auto& entity : scene.getEntities()) {
        if (!entity) continue;
        auto* tilemap = entity->getComponent<TilemapComponent>();
        auto* transform = entity->getComponent<TransformComponent>();
        if (!tilemap || !transform) continue;
        liveComponents.insert(tilemap);

        const auto data = tilemap->data();
        if (!data || data->tiles.empty() || data->width <= 0 || data->height <= 0) {
            continue;
        }
        const auto tileset = TilesetManager::get(data->tilesetId);
        if (!tileset || !tileset->texture) continue;

        m_impl->ensureSharedResources();
        auto& cache = m_impl->caches[tilemap];
        if (Impl::needsRebuild(cache, data, tileset)) {
            Impl::buildMesh(cache, data, tileset);
        }
        if (!cache.vao || cache.indexCount == 0) continue;

        m_impl->shader->enable();
        m_impl->shader->setUniformMat4("projection", viewProjection);
        m_impl->shader->setUniformMat4("transform", transform->modelMatrix());
        m_impl->shader->setUniformInt1("spriteTexture", 0);
        m_impl->shader->setUniformInt1("normalTexture", 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tileset->texture->getID());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_impl->defaultNormal);
        glBindVertexArray(cache.vao);
        glDrawElements(GL_TRIANGLES, cache.indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    for (auto it = m_impl->caches.begin(); it != m_impl->caches.end();) {
        if (liveComponents.contains(it->first)) {
            ++it;
        } else {
            Impl::destroyCache(it->second);
            it = m_impl->caches.erase(it);
        }
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Rendering
