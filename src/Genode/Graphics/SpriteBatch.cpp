////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2022 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include <Genode/Graphics/SpriteBatch.hpp>
#include <Genode/System/Exception.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <algorithm>
#include <cstring>
#include <numeric>

namespace Gx
{
    SpriteBatch::SpriteBatch(const Mode batchMode) :
        m_batchMode(batchMode)
    {
    }

    ////////////////////////////////////////////////////////////
    SpriteBatch::SpriteBatch(const Mode batchMode, const Usage batchUsage) :
        m_batchMode(batchMode),
        m_batchUsage(batchUsage)
    {
    }

    ////////////////////////////////////////////////////////////
    SpriteBatch::SpriteBatch(const SpriteBatch& other) :
        Node(other),
        RenderableContainer(other),
        UpdatableContainer(other),
        m_triangles(other.m_triangles),
        m_unsortedVertices(other.m_unsortedVertices),
        m_flushedTriangles(other.m_flushedTriangles),
        m_batchMode(other.m_batchMode),
        m_batchUsage(other.m_batchUsage),
        m_blendMode(other.m_blendMode)
    {
    }

    ////////////////////////////////////////////////////////////
    SpriteBatch& SpriteBatch::operator=(const SpriteBatch& other)
    {
        if (this == &other)
            return *this;

        Node::operator=(other);
        RenderableContainer::operator=(other);

        if (m_span)
        {
            m_pool.Return(*m_span);
            m_span.reset();
        }

        m_triangles        = other.m_triangles;
        m_unsortedVertices = other.m_unsortedVertices;
        m_flushedTriangles  = other.m_flushedTriangles;
        m_batchMode        = other.m_batchMode;
        m_batchUsage       = other.m_batchUsage;
        m_blendMode        = other.m_blendMode;
        m_rebuildRequired  = true;
        m_batches.clear();
        m_order.clear();

        return *this;
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::SetBatchMode(const Mode batchMode)
    {
        m_batchMode       = batchMode;
        m_rebuildRequired = true;
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::SetBatchUsage(const Usage batchUsage)
    {
        m_batchUsage      = batchUsage;
        m_rebuildRequired = true;
    }

    ////////////////////////////////////////////////////////////
    SpriteBatch::Usage SpriteBatch::GetBatchUsage() const
    {
        return m_batchUsage;
    }

    ////////////////////////////////////////////////////////////
    VertexPool& SpriteBatch::GetVertexPool()
    {
        return m_pool;
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::Batch(const sf::VertexArray& vertices, const sf::Texture* texture, const sf::Transform& transform, const float layer)
    {
        Batch(&vertices[0], vertices.getVertexCount(), vertices.getPrimitiveType(), texture, transform, layer);
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::Batch(const sf::Vertex*       vertices,
                            const std::size_t       count,
                            const sf::PrimitiveType type,
                            const sf::Texture*      texture,
                            const sf::Transform&    transform,
                            const float             layer)
    {
        if (type != sf::PrimitiveType::TriangleFan && type != sf::PrimitiveType::TriangleStrip && type != sf::PrimitiveType::Triangles)
           throw Exception("SpriteBatch supports only triangle-based primitive types");

        // Anything to batch?
        if (count == 0)
            return;

        // Split into triangles
        switch (type)
        {
            case sf::PrimitiveType::TriangleStrip:
                for (std::size_t i = 2; i < count; i++)
                    PushTriangle(vertices[i - 2], vertices[i - 1], vertices[i], transform, texture, layer);

                break;
            case sf::PrimitiveType::TriangleFan:
                for (std::size_t i = 2; i < count; i++)
                    PushTriangle(vertices[0], vertices[i - 1], vertices[i], transform, texture, layer);

                break;
            case sf::PrimitiveType::Triangles:
                for (std::size_t i = 2; i < count; i += 3)
                    PushTriangle(vertices[i - 2], vertices[i - 1], vertices[i], transform, texture, layer);

                break;
            default:
                throw Exception("A non-triangle primitive type was encountered when decomposing into triangles");
        }
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::Update(const sf::Time& delta)
    {
        UpdatableContainer::Update(delta);
    }

    ////////////////////////////////////////////////////////////
    RenderStates SpriteBatch::Render(RenderSurface& surface, RenderStates states) const
    {
        if (!IsVisible())
            return states;

        states.transform *= GetTransform();
        states.blendMode  = m_blendMode.value_or(states.blendMode);

        auto& batcher = const_cast<SpriteBatch&>(*this);
        auto localStates      = states;
        localStates.transform = sf::Transform::Identity;
        RenderableContainer::Render(batcher, localStates);

        if (m_batchUsage == Usage::Dynamic)
        {
            if (m_rebuildRequired || IsStructureChanged())
                RebuildBatch();
            else
                RewriteVertices();

            batcher.Flush();
        }
        else
        {
            RebuildBatch();
            batcher.m_triangles.clear();
            batcher.m_unsortedVertices.clear();
        }

        m_rebuildRequired = false;

        auto cstates = states;
        if (const sf::Vertex* vertices = m_span ? m_span->data() : nullptr)
        {
            std::size_t startVertex = 0;
            for (const auto& batch : m_batches)
            {
                cstates.texture = batch.texture;
                surface.Render(vertices + startVertex, batch.vertexCount, sf::PrimitiveType::Triangles, cstates);

                startVertex += batch.vertexCount;
            }
        }

        return states;
    }

    void SpriteBatch::Clear(const sf::Color)
    {
        ClearBatch();
    }

    void SpriteBatch::Clear(const sf::Color, sf::StencilValue)
    {
        ClearBatch();
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::Render(const Renderable& renderable, const RenderStates& states)
    {
        renderable.Render(*this, states);
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::Render(const sf::Vertex* vertices, const std::size_t vertexCount, const sf::PrimitiveType type, const RenderStates& states)
    {
        if (m_blendMode.has_value() && m_blendMode != states.blendMode)
            throw NotSupportedException("Multiple blending mode usage within single batch is not supported");

        m_blendMode = states.blendMode;
        Batch(vertices, vertexCount, type, states.texture, states.transform, states.Layer);
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::Render(const sf::VertexBuffer& vertexBuffer, const RenderStates& states)
    {
        throw NotSupportedException("Vertex Buffer is already live in GPU Memory");
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::Render(const sf::VertexBuffer& vertexBuffer, const std::size_t firstVertex, const std::size_t vertexCount, const RenderStates& states)
    {
        throw NotSupportedException("Vertex Buffer is already live in GPU Memory");
    }

    const sf::View& SpriteBatch::GetDefaultView() const
    {
        throw NotSupportedException("SpriteBatch does not have a view");
    }

    const sf::View& SpriteBatch::GetView() const
    {
        throw NotSupportedException("SpriteBatch does not have a view");
    }

    void SpriteBatch::SetView(const sf::View& view)
    {
        throw NotSupportedException("SpriteBatch does not have a view");
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::PushTriangle(const sf::Vertex&    a,
                                   const sf::Vertex&    b,
                                   const sf::Vertex&    c,
                                   const sf::Transform& transform,
                                   const sf::Texture*   texture,
                                   float                level)
    {
        m_triangles.emplace_back(texture, level);

        m_unsortedVertices.push_back({transform * a.position, a.color, a.texCoords});
        m_unsortedVertices.push_back({transform * b.position, b.color, b.texCoords});
        m_unsortedVertices.push_back({transform * c.position, c.color, c.texCoords});
    }

    ////////////////////////////////////////////////////////////
    bool SpriteBatch::IsStructureChanged() const
    {
        if (m_triangles.size() != m_flushedTriangles.size())
            return true;

        return !std::equal(m_triangles.begin(), m_triangles.end(), m_flushedTriangles.begin(),
                           [](const TriangleInfo& a, const TriangleInfo& b)
                           { return a.texture == b.texture && a.level == b.level; });
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::RebuildBatch() const
    {
        m_batches.clear();
        m_order.resize(m_triangles.size());
        std::iota(m_order.begin(), m_order.end(), 0);

        if (m_batchMode == Mode::TextureSort)
        {
            const auto comp = [this](const std::size_t a, const std::size_t b)
            { return m_triangles[a].texture < m_triangles[b].texture; };

            std::stable_sort(m_order.begin(), m_order.end(), comp);
        }
        else if (m_batchMode == Mode::LayerSort)
        {
            const auto comp = [this](const std::size_t a, const std::size_t b)
            {
                if (m_triangles[a].level != m_triangles[b].level)
                    return m_triangles[a].level < m_triangles[b].level;
                else
                    return m_triangles[a].texture < m_triangles[b].texture;
            };

            std::stable_sort(m_order.begin(), m_order.end(), comp);
        }

        if (!m_span || m_span->size() < m_unsortedVertices.size())
        {
            if (m_span)
                m_pool.Return(*m_span);

            m_span.emplace(m_pool.Rent(m_unsortedVertices.size()));
        }

        for (std::size_t i = 0; i < m_order.size(); i++)
        {
            const std::size_t newPos = i * 3;
            const std::size_t oldPos = m_order[i] * 3;
            (*m_span)[newPos]     = m_unsortedVertices[oldPos];
            (*m_span)[newPos + 1] = m_unsortedVertices[oldPos + 1];
            (*m_span)[newPos + 2] = m_unsortedVertices[oldPos + 2];
        }

        std::size_t startIndex = 0;
        const sf::Texture* lastTexture = m_order.empty() ? nullptr : m_triangles[m_order[0]].texture;

        for (std::size_t i = 1; i < m_order.size(); i++)
        {
            if (const sf::Texture* nextTexture = m_triangles[m_order[i]].texture; nextTexture != lastTexture)
            {
                m_batches.emplace_back(lastTexture, (i - startIndex) * 3);
                lastTexture = nextTexture;
                startIndex  = i;
            }
        }

        if (startIndex != m_order.size())
            m_batches.emplace_back(lastTexture, (m_order.size() - startIndex) * 3);
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::RewriteVertices() const
    {
        for (std::size_t i = 0; i < m_order.size(); i++)
        {
            const std::size_t oldPos = m_order[i] * 3;
            const std::size_t newPos = i * 3;
            if (std::memcmp(&m_unsortedVertices[oldPos], m_span->data() + newPos, sizeof(sf::Vertex) * 3) == 0)
                continue;

            (*m_span)[newPos]     = m_unsortedVertices[oldPos];
            (*m_span)[newPos + 1] = m_unsortedVertices[oldPos + 1];
            (*m_span)[newPos + 2] = m_unsortedVertices[oldPos + 2];
        }
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::Flush()
    {
        std::swap(m_flushedTriangles, m_triangles);

        m_triangles.clear();
        m_unsortedVertices.clear();
    }

    ////////////////////////////////////////////////////////////
    void SpriteBatch::ClearBatch()
    {
        m_unsortedVertices.clear();
        m_triangles.clear();
        m_flushedTriangles.clear();
        m_order.clear();
        m_batches.clear();

        m_rebuildRequired = true;
    }

} // namespace Gx
