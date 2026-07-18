#pragma once

#include <Genode/Entities/Poolable.hpp>

#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <vector>
#include <optional>
#include <iterator>
#include <Genode/Entities/Renderable.hpp>

namespace Gx
{
    class VertexPool;
    class VertexSpan final : public Renderable
    {
    public:
        using value_type             = sf::Vertex;
        using size_type              = std::size_t;
        using difference_type        = std::ptrdiff_t;
        using reference              = value_type&;
        using const_reference        = const value_type&;
        using pointer                = value_type*;
        using const_pointer          = const value_type*;
        using iterator               = std::vector<sf::Vertex>::iterator;
        using const_iterator         = std::vector<sf::Vertex>::const_iterator;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        VertexSpan(VertexSpan&& other) noexcept;
        ~VertexSpan() override;

        [[nodiscard]] reference operator[](size_type idx);
        [[nodiscard]] const_reference operator[](size_type idx) const;
        [[nodiscard]] reference front();
        [[nodiscard]] const_reference front() const;
        [[nodiscard]] reference back();
        [[nodiscard]] const_reference back() const;
        [[nodiscard]] pointer data();
        [[nodiscard]] const_pointer data() const;

        [[nodiscard]] iterator begin() noexcept;
        [[nodiscard]] const_iterator begin() const noexcept;
        [[nodiscard]] const_iterator cbegin() const noexcept;
        [[nodiscard]] iterator end() noexcept;
        [[nodiscard]] const_iterator end() const noexcept;
        [[nodiscard]] const_iterator cend() const noexcept;
        [[nodiscard]] reverse_iterator rbegin() noexcept;
        [[nodiscard]] const_reverse_iterator rbegin() const noexcept;
        [[nodiscard]] const_reverse_iterator crbegin() const noexcept;
        [[nodiscard]] reverse_iterator rend() noexcept;
        [[nodiscard]] const_reverse_iterator rend() const noexcept;
        [[nodiscard]] const_reverse_iterator crend() const noexcept;

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] size_type size() const noexcept;
        [[nodiscard]] size_type offset() const noexcept;

        [[nodiscard]] const std::vector<sf::Vertex>& container() const noexcept;

        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

        VertexSpan& operator=(VertexSpan&& other) noexcept;

        [[nodiscard]] bool operator==(const VertexSpan& other) const noexcept;
        [[nodiscard]] bool operator!=(const VertexSpan& other) const noexcept;

    private:
        friend class VertexPool;

        VertexSpan(VertexPool& pool, size_type offset, size_type size, bool scoped = false);

        VertexPool* m_pool = nullptr;
        size_type   m_offset = {};
        size_type   m_size   = {};
        bool        m_scoped = false;
    };

    class VertexPool : public Poolable<VertexSpan>, public Renderable
    {
    public:
        VertexPool() = default;
        explicit VertexPool(sf::PrimitiveType primitiveType);
        explicit VertexPool(std::size_t capacity);
        explicit VertexPool(sf::PrimitiveType primitiveType, std::size_t capacity);

        [[nodiscard]] VertexSpan Rent(std::size_t size) override;
        [[nodiscard]] VertexSpan Rent(std::size_t size, bool scoped);
        void Return(VertexSpan& span) override;

        [[nodiscard]] VertexSpan Transfer(const std::vector<sf::Vertex>& vertices);
        [[nodiscard]] VertexSpan Transfer(const sf::VertexArray& vertices);

        void Reset();
        void Clear();

        [[nodiscard]] std::size_t GetCapacity() const noexcept;
        [[nodiscard]] std::size_t GetSize() const noexcept;
        [[nodiscard]] bool IsEmpty() const noexcept;

        [[nodiscard]] const std::vector<sf::Vertex>& GetVertices() const noexcept;
        [[nodiscard]] const sf::Vertex* GetData() const noexcept;
        [[nodiscard]] sf::Vertex* GetData() noexcept;

        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

    private:
        friend class VertexSpan;

        struct Segment
        {
            std::size_t m_offset = {};
            std::size_t m_size   = {};
            bool m_inUse         = {};

            Segment(std::size_t offset, std::size_t size, bool inUse = false);
        };

        [[nodiscard]] std::optional<std::size_t> Scan(std::size_t size) const;
        void Defragment();

        sf::PrimitiveType m_primitive      = sf::PrimitiveType::Triangles;
        std::vector<sf::Vertex> m_vertices = {};
        std::vector<Segment> m_segments    = {};
    };
}
