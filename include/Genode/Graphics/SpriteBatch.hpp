////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Laurent Gomila (laurent@sfml-dev.org)
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

#pragma once

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include <Genode/Entities/Renderable.hpp>
#include <Genode/Graphics/RenderStates.hpp>
#include <Genode/Graphics/VertexPool.hpp>
#include <Genode/SceneGraph/Node.hpp>
#include <Genode/SceneGraph/RenderableContainer.hpp>
#include <Genode/SceneGraph/UpdatableContainer.hpp>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <vector>

namespace Gx
{

    ////////////////////////////////////////////////////////////
    /// @brief Batching utility for drawables, meant for increasing
    ///        rendering performance
    ///
    ////////////////////////////////////////////////////////////
    class SpriteBatch : public virtual Node, public virtual RenderableContainer, public virtual UpdatableContainer, public RenderSurface
    {
    public:
        enum class Mode
        {
            ////////////////////////////////////////////////////////////
            /// @brief Creates a batch for each contiguous groups of drawables
            ///        that use the same texture.
            ///
            ////////////////////////////////////////////////////////////
            Deferred,

            ////////////////////////////////////////////////////////////
            /// @brief Organizes objects into a single layer, then reorders
            ///        them by texture before drawing.
            ///
            ////////////////////////////////////////////////////////////
            TextureSort,

            ////////////////////////////////////////////////////////////
            /// @brief Organizes objects into multiple layers. Groups objects
            ///        into layers based on level, then reorders objects in
            ///        each layer by texture before drawing the layers from
            ///        closest to furthest.
            ///
            ////////////////////////////////////////////////////////////
            LayerSort,
        };

        enum class Usage
        {
            ////////////////////////////////////////////////////////////
            /// @brief Rebuilds the batch from scratch on every flush.
            ///
            ////////////////////////////////////////////////////////////
            Stream,

            ////////////////////////////////////////////////////////////
            /// @brief Locks the batch in on flush and diffs subsequent
            ///        submissions against it, rewriting only vertices
            ///        that changed and re-sorting only when the batch
            ///        structure changes.
            ///
            ////////////////////////////////////////////////////////////
            Dynamic,
        };

        SpriteBatch() = default;
        explicit SpriteBatch(Mode batchMode);
        SpriteBatch(Mode batchMode, Usage batchUsage);

        SpriteBatch(const SpriteBatch& other);
        SpriteBatch& operator=(const SpriteBatch& other);

        ////////////////////////////////////////////////////////////
        /// @brief Sets the batch mode to use
        ///
        /// @param batchMode The new batch mode to use
        ///
        ////////////////////////////////////////////////////////////
        void SetBatchMode(Mode batchMode);

        ////////////////////////////////////////////////////////////
        /// @brief Sets the batch usage to use
        ///
        /// @param batchUsage The new batch usage to use
        ///
        ////////////////////////////////////////////////////////////
        void SetBatchUsage(Usage batchUsage);

        ////////////////////////////////////////////////////////////
        /// @brief Gets the current batch usage
        ///
        /// @return The current batch usage
        ///
        ////////////////////////////////////////////////////////////
        [[nodiscard]] Usage GetBatchUsage() const;

        ////////////////////////////////////////////////////////////
        /// @brief Gets the vertex pool that holds the vertices to render
        ///
        /// @return Reference to the vertex pool of the batch
        ///
        ////////////////////////////////////////////////////////////
        [[nodiscard]] VertexPool& GetVertexPool();

        ////////////////////////////////////////////////////////////
        /// @brief Batches an array of vertices
        ///
        /// @param vertices  The array of vertices to be batched
        /// @param count     How many vertices to batch
        /// @param type      The primitive formed by the vertices.
        ///                  Only triangle primitives are supported
        /// @param texture   The texture to use for rendering
        /// @param transform The transform to apply on the vertices
        /// @param layer     The layer at which the drawable will be
        ///                  renderered. This value is used only when
        ///                  BatchMode::LayerSort mode is used.
        ///
        ////////////////////////////////////////////////////////////
        void Batch(const sf::Vertex*    vertices,
                   std::size_t          count,
                   sf::PrimitiveType    type,
                   const sf::Texture*   texture,
                   const sf::Transform& transform = sf::Transform::Identity,
                   float                layer     = 0.f);

        void Batch(const sf::VertexArray& vertices,
                   const sf::Texture*     texture,
                   const sf::Transform&   transform = sf::Transform::Identity,
                   float                  layer     = 0.f);

        void Update(const sf::Time& delta) override;

        ////////////////////////////////////////////////////////////
        /// @brief Renders the batch to the render target
        ///
        /// @param surface The RenderTarget to draw to
        /// @param states The RenderStates to use. The texture is ignored
        ///
        ////////////////////////////////////////////////////////////
        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

        void Clear(sf::Color clearColor) override;
        void Clear(sf::Color clearColor, sf::StencilValue stencilValue) override;

        void Render(const Renderable& renderable, const RenderStates& states) override;
        void Render(const sf::Vertex*   vertices,
                    std::size_t         vertexCount,
                    sf::PrimitiveType   type,
                    const RenderStates& states
        ) override;

        void Render(const sf::VertexBuffer& vertexBuffer, const RenderStates& states) override;
        void Render(const sf::VertexBuffer& vertexBuffer,
                    std::size_t             firstVertex,
                    std::size_t             vertexCount,
                    const RenderStates&     states
        ) override;

        [[nodiscard]] const sf::View& GetDefaultView() const override;
        [[nodiscard]] const sf::View& GetView() const override;
        void SetView(const sf::View& view) override;

        ////////////////////////////////////////////////////////////
        /// @brief Clears the batch, removing all drawables that were
        ///        added
        ///
        ////////////////////////////////////////////////////////////
        void ClearBatch();

    private:
        ////////////////////////////////////////////////////////////
        /// @brief Pushes a triangle into internal storage
        ///
        /// @param a         First vertex
        /// @param b         Second vertex
        /// @param c         Third vertex
        /// @param transform Transform to apply before inserting
        /// @param texture   Texture to apply to triangle
        /// @param level     The level to use for level sorting
        ///
        ////////////////////////////////////////////////////////////
        void PushTriangle(const sf::Vertex&    a,
                          const sf::Vertex&    b,
                          const sf::Vertex&    c,
                          const sf::Transform& transform,
                          const sf::Texture*   texture,
                          float                level);

        [[nodiscard]] bool IsStructureChanged() const;

        void RebuildBatch() const;
        void RewriteVertices() const;
        void Flush();

        ////////////////////////////////////////////////////////////
        /// @brief Holds information about a batched triangle
        ///
        ////////////////////////////////////////////////////////////
        struct TriangleInfo
        {
            const sf::Texture* texture{}; //!< The triangle's texture
            float              level{};   //!< The level of the triangle

            TriangleInfo() = default;
            TriangleInfo(const sf::Texture* theTexture, float theLevel) : texture(theTexture), level(theLevel) {}
        };

        ////////////////////////////////////////////////////////////
        /// @brief Holds information for rendering a batch
        ///
        ////////////////////////////////////////////////////////////
        struct BatchInfo
        {
            const sf::Texture* texture{};     //!< The texture used to render the batch
            std::size_t        vertexCount{}; //!< The number of contiguous vertices to render

            BatchInfo() = default;
            BatchInfo(const sf::Texture* theTexture, std::size_t theVertexCount) :
            texture(theTexture),
            vertexCount(theVertexCount)
            {
            }
        };

        ////////////////////////////////////////////////////////////
        // Member data
        ////////////////////////////////////////////////////////////
        // Batched Triangles
        std::vector<TriangleInfo> m_triangles;        //!< Info about batched triangles
        std::vector<sf::Vertex>   m_unsortedVertices; //!< Vertices currently batched
        std::vector<TriangleInfo> m_flushedTriangles; //!< Info about triangles locked in at the last flush

        mutable std::vector<BatchInfo>    m_batches; //!< Prepared batch information, ready for rendering
        mutable std::vector<std::size_t>  m_order;   //!< Triangle permutation of the last rebuild
        mutable std::optional<VertexSpan> m_span;    //!< Prepared vertices, ready for rendering

        // Vertex Storage
        mutable VertexPool m_pool; //!< The pool that holds the prepared vertices

        // Batch Settings
        Mode                         m_batchMode{Mode::Deferred};  //!< The current batch strategy
        Usage                        m_batchUsage{Usage::Dynamic};  //!< The current batch usage
        std::optional<sf::BlendMode> m_blendMode{};                //!< The blending mode for rendering
        mutable bool                 m_rebuildRequired{true};      //!< If true, batch must be rebuilt before rendering
    };

} // namespace Gx

////////////////////////////////////////////////////////////
/// @class Gx::SpriteBatch
/// @ingroup graphics
///
/// sf::SpriteBatch is a batching utility for drawables. It
/// increases rendering performance by reordering and combining
/// drawables, such that they are rendered using fewer draw calls
/// and state switches.
///
/// sf::SpriteBatch supports multiple batching strategies. Each
/// comes with its benefits and drawbacks, so the optimal choice
/// depends on your specific needs.
///
/// @c sf::BatchMode::Deferred is best used when rendering contiguous
/// groups of drawables that use the same texture. This mode creates
/// batches for each such group, improving performance when many
/// drawables with few vertices are drawn (such as hundreds to
/// thousands of sf::Sprite drawables).
/// The render order of drawables is the same as the order they
/// were batched in. This is similar how rendering works with
/// sf::RenderTarget.
///
/// @c sf::BatchMode::TextureSort is best used when drawing objects
/// that are positioned on a "layer", and do not overlap each other.
/// This mode assumes that the draw order is not important within
/// a layer, and sorts all objects by texture before batching them.
/// This provides great performance improvements, especially when
/// rendering objects using different textures (since it will reorder
/// them back in contiguous groups).
/// Care should be taken when drawing overlapping objects, since
/// it is not guaranteed that they will be ordered correctly.
///
/// @c sf::BatchMode::LayerSort is best used when drawing objects
/// that are positioned on multiple "layers" based on level, and objects
/// within a layer do not overlap each other.
/// Within each layer, objects are sorted by texture before drawing,
/// similar to @c TextureSort. However, objects in a layer with a
/// lower "level" value will be drawn behind objects with a higher
/// "level" value.
/// You can also use this mode to render objects in an order that is
/// different from the order they were batched in, since objects
/// will be reordered by level before rendering.
///
/// An example of how to use sf::SpriteBatch can be found in the
/// examples/sprite_batch project.
///
////////////////////////////////////////////////////////////
