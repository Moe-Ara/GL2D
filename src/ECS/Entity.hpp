#pragma once

#include <cstdint>
#include <functional>
#include <limits>

namespace ECS {

class Entity {
public:
    using Index = std::uint32_t;
    using Generation = std::uint32_t;

    constexpr Entity() noexcept = default;
    constexpr Entity(Index index, Generation generation) noexcept
        : m_index(index), m_generation(generation) {}

    [[nodiscard]] constexpr Index index() const noexcept { return m_index; }
    [[nodiscard]] constexpr Generation generation() const noexcept { return m_generation; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return m_index != invalidIndex() && m_generation != 0;
    }

    [[nodiscard]] static constexpr Index invalidIndex() noexcept {
        return std::numeric_limits<Index>::max();
    }

    constexpr bool operator==(const Entity&) const noexcept = default;

private:
    Index m_index{invalidIndex()};
    Generation m_generation{0};
};

} // namespace ECS

template<>
struct std::hash<ECS::Entity> {
    std::size_t operator()(ECS::Entity entity) const noexcept {
        const std::uint64_t value = (static_cast<std::uint64_t>(entity.generation()) << 32U) |
                                    entity.index();
        return std::hash<std::uint64_t>{}(value);
    }
};
