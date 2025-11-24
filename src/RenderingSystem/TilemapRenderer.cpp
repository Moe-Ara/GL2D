//
// TilemapRenderer.cpp
//

#include "TilemapRenderer.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <unordered_map>

#include "Engine/Scene.hpp"
#include "GameObjects/Components/TilemapComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "Graphics/Shader.hpp"
#include "Managers/TilesetManager.hpp"
#include "Managers/Tileset.hpp"
#include "GameObjects/Vertex.hpp"

namespace Rendering {
namespace {
    struct TilemapCache {
        GLuint vao{0};
        GLuint vbo{0};
        GLuint ibo{0};
        GLsizei indexCount{0};
        std::weak_ptr<TilemapData> data;
        std::weak_ptr<Tileset> tileset;
        glm::ivec2 dims{0,0};
        glm::vec2 tileSize{0.0f};
    };

    std::unordered_map<const TilemapComponent*, TilemapCache>& cacheMap() {
        static std::unordered_map<const TilemapComponent*, TilemapCache> s_cache;
        return s_cache;
    }

    void destroyCache(TilemapCache& cache) {
        if (cache.vbo) { glDeleteBuffers(1, &cache.vbo); cache.vbo = 0; }
        if (cache.ibo) { glDeleteBuffers(1, &cache.ibo); cache.ibo = 0; }
        if (cache.vao) { glDeleteVertexArrays(1, &cache.vao); cache.vao = 0; }
        cache.indexCount = 0;
    }

    void ensureBuffers(TilemapCache& cache) {
        if (!cache.vao) {
            glGenVertexArrays(1, &cache.vao);
            glGenBuffers(1, &cache.vbo);
            glGenBuffers(1, &cache.ibo);
            glBindVertexArray(cache.vao);
            glBindBuffer(GL_ARRAY_BUFFER, cache.vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cache.ibo);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
            glBindVertexArray(0);
        }
    }

    bool needsRebuild(const TilemapCache& cache,
                      const std::shared_ptr<TilemapData>& data,
                      const std::shared_ptr<Tileset>& tileset) {
        if (!data || !tileset) return true;
        if (cache.data.expired() || cache.tileset.expired()) return true;
        if (cache.data.lock() != data) return true;
        if (cache.tileset.lock() != tileset) return true;
        if (cache.dims.x != data->width || cache.dims.y != data->height) return true;
        if (cache.tileSize != data->tileSize) return true;
        return false;
    }

    void buildMesh(TilemapCache& cache,
                   const TilemapComponent& comp,
                   const TransformComponent& transformComp,
                   const std::shared_ptr<TilemapData>& data,
                   const std::shared_ptr<Tileset>& tileset) {
        cache.dims = {data->width, data->height};
        cache.tileSize = data->tileSize;
        cache.data = data;
        cache.tileset = tileset;

        std::vector<Vertex> verts;
        std::vector<uint32_t> indices;
        verts.reserve(static_cast<size_t>(data->width * data->height) * 4);
        indices.reserve(static_cast<size_t>(data->width * data->height) * 6);

        const glm::vec2 origin = transformComp.getTransform().Position;
        const glm::vec2 tileSize = data->tileSize;

        for (int y = 0; y < data->height; ++y) {
            for (int x = 0; x < data->width; ++x) {
                const int idx = y * data->width + x;
                if (idx < 0 || idx >= static_cast<int>(data->tiles.size())) continue;
                const int tileIndex = data->tiles[static_cast<size_t>(idx)];
                if (tileIndex < 0 || tileIndex >= static_cast<int>(tileset->uvs.size())) continue;
                const glm::vec4 uv = tileset->getUV(tileIndex);

                const glm::vec2 base = origin + glm::vec2{x * tileSize.x, y * tileSize.y};
                const uint32_t baseIndex = static_cast<uint32_t>(verts.size());

                verts.push_back(Vertex{glm::vec2(base.x, base.y + tileSize.y), glm::vec3(1.0f), glm::vec2(uv.x, uv.w)});
                verts.push_back(Vertex{glm::vec2(base.x + tileSize.x, base.y + tileSize.y), glm::vec3(1.0f), glm::vec2(uv.z, uv.w)});
                verts.push_back(Vertex{glm::vec2(base.x + tileSize.x, base.y), glm::vec3(1.0f), glm::vec2(uv.z, uv.y)});
                verts.push_back(Vertex{glm::vec2(base.x, base.y), glm::vec3(1.0f), glm::vec2(uv.x, uv.y)});

                indices.insert(indices.end(), {
                    baseIndex, baseIndex + 1, baseIndex + 2,
                    baseIndex + 2, baseIndex + 3, baseIndex
                });
            }
        }

        cache.indexCount = static_cast<GLsizei>(indices.size());
        ensureBuffers(cache);

        glBindBuffer(GL_ARRAY_BUFFER, cache.vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cache.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
        glBindVertexArray(0);
    }
} // namespace

void TilemapRenderer::render(Scene &scene, Camera &camera, const glm::mat4 &viewProj) {
    static std::shared_ptr<Graphics::Shader> shader = std::make_shared<Graphics::Shader>("Shaders/vertex.vert", "Shaders/fragment.frag");

    for (auto &entityPtr : scene.getEntities()) {
        if (!entityPtr) continue;
        auto *tilemapComp = entityPtr->getComponent<TilemapComponent>();
        auto *transformComp = entityPtr->getComponent<TransformComponent>();
        if (!tilemapComp || !transformComp) continue;
        const auto data = tilemapComp->data();
        if (!data) continue;
        auto tileset = TilesetManager::get(data->tilesetId);
        if (!tileset || !tileset->texture) continue;
        if (data->tiles.empty() || data->width <= 0 || data->height <= 0) continue;

        auto &cache = cacheMap()[tilemapComp];
        if (needsRebuild(cache, data, tileset)) {
            buildMesh(cache, *tilemapComp, *transformComp, data, tileset);
        }
        if (cache.indexCount == 0 || !cache.vao) continue;

        shader->enable();
        shader->setUniformMat4("projection", viewProj);
        shader->setUniformMat4("transform", glm::mat4(1.0f));
        shader->setUniformInt1("spriteTexture", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tileset->texture->getID());

        glBindVertexArray(cache.vao);
        glDrawElements(GL_TRIANGLES, cache.indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Rendering
