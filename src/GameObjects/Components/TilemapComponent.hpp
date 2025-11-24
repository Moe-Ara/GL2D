//
// Created by Codex on 22/11/2025.
//

#ifndef GL2D_TILEMAPCOMPONENT_HPP
#define GL2D_TILEMAPCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include <glm/vec2.hpp>
#include <vector>
#include <memory>
#include <string>

struct TilemapData {
    int width{0};
    int height{0};
    glm::vec2 tileSize{1.0f, 1.0f};
    std::string tilesetId;
    std::vector<int> tiles; // row-major indices into a tileset atlas
};

class TilemapComponent : public IComponent {
public:
    explicit TilemapComponent(std::shared_ptr<TilemapData> data = nullptr, int zIndex = 0, bool collision = false);
    ~TilemapComponent() override = default;

    TilemapComponent(const TilemapComponent&) = delete;
    TilemapComponent& operator=(const TilemapComponent&) = delete;
    TilemapComponent(TilemapComponent&&) = delete;
    TilemapComponent& operator=(TilemapComponent&&) = delete;

    void setData(std::shared_ptr<TilemapData> data) { m_data = std::move(data); }
    std::shared_ptr<TilemapData> data() const { return m_data; }
    int zIndex() const { return m_zIndex; }
    bool collisionEnabled() const { return m_collision; }

private:
    std::shared_ptr<TilemapData> m_data;
    int m_zIndex{0};
    bool m_collision{false};
};

#endif //GL2D_TILEMAPCOMPONENT_HPP
