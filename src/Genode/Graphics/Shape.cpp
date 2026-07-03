////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2026 Laurent Gomila (laurent@sfml-dev.org)
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
#include <Genode/Graphics/Shape.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <algorithm>

#include <cassert>
#include <cmath>
#include <cstddef>

namespace
{
    // Compute the direction of a segment
    sf::Vector2f ComputeDirection(const sf::Vector2f p1, const sf::Vector2f p2)
    {
        sf::Vector2f direction = p2 - p1;
        const float  length    = direction.length();
        if (length != 0.f)
            direction /= length;
        return direction;
    }
}


namespace Gx
{
    ////////////////////////////////////////////////////////////
    Shape::Shape() :
        Shape(sf::PrimitiveType::TriangleFan)
    {
    }


    ////////////////////////////////////////////////////////////
    Shape::Shape(const sf::PrimitiveType primitiveType) :
        m_vertices(primitiveType)
    {
    }


    ////////////////////////////////////////////////////////////
    void Shape::SetTexture(const sf::Texture& texture, const bool resetRect)
    {
        // Recompute the texture area if requested, or if there was no texture & rect before
        if (resetRect || (!m_texture && (m_textureRect == sf::IntRect())))
            SetTexCoords(sf::IntRect({0, 0}, sf::Vector2i(texture.getSize())));

        // Assign the new texture
        m_texture = &texture;
    }


    ////////////////////////////////////////////////////////////
    const sf::Texture* Shape::GetTexture() const
    {
        return m_texture;
    }


    ////////////////////////////////////////////////////////////
    void Shape::SetTexCoords(const sf::IntRect& rect)
    {
        m_textureRect = rect;
        UpdateTexCoords();
    }


    ////////////////////////////////////////////////////////////
    const sf::IntRect& Shape::GetTexCoords() const
    {
        return m_textureRect;
    }


    ////////////////////////////////////////////////////////////
    void Shape::SetColor(const sf::Color& color)
    {
        m_fillColor = color;
        UpdateFillColors();
    }


    ////////////////////////////////////////////////////////////
    void Shape::SetColor(const unsigned int index, const sf::Color& color)
    {
        m_colorMap[index] = color;
    }


    ////////////////////////////////////////////////////////////
    const sf::Color& Shape::GetColor() const
    {
        return m_fillColor;
    }


    ////////////////////////////////////////////////////////////
    const sf::Color& Shape::GetColor(const unsigned int index) const
    {
        if (const auto it = m_colorMap.find(index); it != m_colorMap.end())
            return it->second;

        return m_fillColor;
    }


    ////////////////////////////////////////////////////////////
    void Shape::SetOutlineColor(const sf::Color& color)
    {
        m_outlineColor = color;
        UpdateOutlineColors();
    }


    ////////////////////////////////////////////////////////////
    const sf::Color& Shape::GetOutlineColor() const
    {
        return m_outlineColor;
    }


    ////////////////////////////////////////////////////////////
    void Shape::SetOutlineThickness(const float thickness)
    {
        m_outlineThickness = thickness;
        UpdateOutline();
    }


    ////////////////////////////////////////////////////////////
    float Shape::GetOutlineThickness() const
    {
        return m_outlineThickness;
    }


    ////////////////////////////////////////////////////////////
    void Shape::SetMiterLimit(const float miterLimit)
    {
        assert(miterLimit >= 1.f && "Shape::SetMiterLimit(float) cannot set miter limit to a value lower than 1");
        m_miterLimit = miterLimit;
        UpdateOutline();
    }


    ////////////////////////////////////////////////////////////
    float Shape::GetMiterLimit() const
    {
        return m_miterLimit;
    }


    ////////////////////////////////////////////////////////////
    sf::Vector2f Shape::GetGeometricCenter() const
    {
        const auto count = GetPointCount();

        switch (count)
        {
            case 0:
                assert(false && "Cannot calculate geometric center of shape with no points");
                return sf::Vector2f{};
            case 1:
                return GetPoint(0);
            case 2:
                return (GetPoint(0) + GetPoint(1)) / 2.f;
            default: // more than two points
                sf::Vector2f centroid;
                float        twiceArea = 0;

                auto previousPoint = GetPoint(count - 1);
                for (std::size_t i = 0; i < count; ++i)
                {
                    const auto  currentPoint = GetPoint(i);
                    const float product      = previousPoint.cross(currentPoint);
                    twiceArea += product;
                    centroid += (currentPoint + previousPoint) * product;

                    previousPoint = currentPoint;
                }

                if (twiceArea != 0.f)
                {
                    return centroid / 3.f / twiceArea;
                }

                // Fallback for no area - find the center of the bounding box
                auto minPoint = GetPoint(0);
                auto maxPoint = minPoint;
                for (std::size_t i = 1; i < count; ++i)
                {
                    const auto currentPoint = GetPoint(i);
                    minPoint.x              = std::min(minPoint.x, currentPoint.x);
                    maxPoint.x              = std::max(maxPoint.x, currentPoint.x);
                    minPoint.y              = std::min(minPoint.y, currentPoint.y);
                    maxPoint.y              = std::max(maxPoint.y, currentPoint.y);
                }
                return (maxPoint + minPoint) / 2.f;
        }
    }


    ////////////////////////////////////////////////////////////
    sf::FloatRect Shape::GetLocalBounds() const
    {
        return m_bounds;
    }


    ////////////////////////////////////////////////////////////
    sf::FloatRect Shape::GetGlobalBounds() const
    {
        auto parent    = GetParent();
        auto transform = sf::Transform::Identity;
        while (parent)
        {
            transform *= parent->GetTransform();
            parent = parent->GetParent();
        }

        transform *= GetTransform();
        return transform.transformRect(GetLocalBounds());
    }


    ////////////////////////////////////////////////////////////
    void Shape::Update()
    {
        // Get the total number of points of the shape
        const std::size_t count = GetPointCount();
        if (count < 3)
        {
            m_vertices.clear();
            m_outlineVertices.clear();
            return;
        }

        m_vertices.resize(count + 2); // + 2 for center and repeated first point

        // Position
        for (std::size_t i = 0; i < count; ++i)
            m_vertices[i + 1].position = GetPoint(i);
        m_vertices[count + 1].position = m_vertices[1].position;

        // Update the bounding rectangle
        m_vertices[0]  = m_vertices[1]; // so that the result of getBounds() is correct
        m_insideBounds = m_vertices.getBounds();

        // Compute the center and make it the first vertex
        m_vertices[0].position = m_insideBounds.getCenter();

        // Color
        UpdateFillColors();

        // Texture coordinates
        UpdateTexCoords();

        // Outline
        UpdateOutline();
    }


    ////////////////////////////////////////////////////////////
    RenderStates Shape::Render(RenderSurface& surface, RenderStates states) const
    {
        if (!IsVisible())
            return states;

        states.transform     *= GetTransform();
        states.coordinateType = sf::CoordinateType::Pixels;

        // Render the inside
        states.texture = m_texture;
        surface.Render(m_vertices, states);

        // Render the outline
        if (m_outlineThickness != 0)
        {
            states.texture = nullptr;
            surface.Render(m_outlineVertices, states);
        }

        return RenderableContainer::Render(surface, states);
    }


    ////////////////////////////////////////////////////////////
    void Shape::UpdateFillColors()
    {
        for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i)
            m_vertices[i].color = GetColor(i);
    }


    ////////////////////////////////////////////////////////////
    void Shape::UpdateTexCoords()
    {
        const sf::FloatRect convertedTextureRect(m_textureRect);

        // Make sure not to divide by zero when the points are aligned on a vertical or horizontal line
        const sf::Vector2f safeInsideSize(m_insideBounds.size.x > 0 ? m_insideBounds.size.x : 1.f,
                                          m_insideBounds.size.y > 0 ? m_insideBounds.size.y : 1.f);

        for (auto& vertex : m_vertices)
        {
            const sf::Vector2f ratio = (vertex.position - m_insideBounds.position).componentWiseDiv(safeInsideSize);
            vertex.texCoords         = convertedTextureRect.position + convertedTextureRect.size.componentWiseMul(ratio);
        }
    }


    ////////////////////////////////////////////////////////////
    void Shape::UpdateOutline()
    {
        // Return if there is no outline or no vertices
        if (m_outlineThickness == 0.f || m_vertices.getVertexCount() < 2)
        {
            m_outlineVertices.clear();
            m_bounds = m_insideBounds;
            return;
        }

        const std::size_t count = m_vertices.getVertexCount() - 2;
        m_outlineVertices.resize((count + 1) * 2); // We need at least that many vertices.
                                                   // We will add two more vertices each time we need a bevel.

        // Determine if points are defined clockwise or counterclockwise. This will impact normals computation.
        const bool flipNormals = [this, count]()
        {
            // p0 is either strictly inside the shape, or on an edge.
            const sf::Vector2f p0 = m_vertices[0].position;
            for (std::size_t i = 0; i < count; ++i)
            {
                const sf::Vector2f p1      = m_vertices[i + 1].position;
                const sf::Vector2f p2      = m_vertices[i + 2].position;
                const float        product = (p1 - p0).cross(p2 - p0);
                if (product == 0.f)
                {
                    // p0 is on the edge p1-p2. We cannot determine shape orientation yet, so continue.
                    continue;
                }
                return product > 0.f;
            }
            return true;
        }();

        std::size_t outlineIndex = 0;
        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t index = i + 1;

            // Get the two segments shared by the current point
            const sf::Vector2f p0 = (i == 0) ? m_vertices[count].position : m_vertices[index - 1].position;
            const sf::Vector2f p1 = m_vertices[index].position;
            const sf::Vector2f p2 = m_vertices[index + 1].position;

            // Compute their direction
            const sf::Vector2f d1 = ComputeDirection(p0, p1);
            const sf::Vector2f d2 = ComputeDirection(p1, p2);

            // Compute their normal pointing towards the outside of the shape
            const sf::Vector2f n1 = flipNormals ? -d1.perpendicular() : d1.perpendicular();
            const sf::Vector2f n2 = flipNormals ? -d2.perpendicular() : d2.perpendicular();

            // Decide whether to add a bevel or not
            const float twoCos2            = 1.f + n1.dot(n2);
            const float squaredLengthRatio = m_miterLimit * m_miterLimit * twoCos2 / 2.f;
            const bool  isConvexCorner     = d1.dot(n2) * m_outlineThickness >= 0.f;
            const bool  needsBevel         = twoCos2 == 0.f || (squaredLengthRatio < 1.f && isConvexCorner);

            if (needsBevel)
            {
                // Make room for two more vertices
                m_outlineVertices.resize(m_outlineVertices.getVertexCount() + 2);

                // Combine normals to get bevel edge's direction and normal vector pointing towards the outside of the shape
                const float        twoSin2   = 1.f - n1.dot(n2);
                const sf::Vector2f direction = (n2 - n1) / twoSin2; // Length is 1 / sin
                const sf::Vector2f extrusion = (flipNormals != (d1.dot(n2) >= 0.f) ? direction : -direction).perpendicular();

                // Compute bevel corner position in (direction, extrusion) coordinates
                const float sin = std::sqrt(twoSin2 / 2.f);
                const float u   = m_miterLimit * sin;
                const float v   = 1.f - std::sqrt(squaredLengthRatio);

                // Update the outline points
                m_outlineVertices[outlineIndex++].position = p1;
                m_outlineVertices[outlineIndex++].position = p1 + (u * extrusion - v * direction) * m_outlineThickness;
                m_outlineVertices[outlineIndex++].position = p1;
                m_outlineVertices[outlineIndex++].position = p1 + (u * extrusion + v * direction) * m_outlineThickness;
            }
            else
            {
                // Combine normals to get the extrusion direction
                const sf::Vector2f extrusion = (n1 + n2) / twoCos2;

                // Update the outline points
                m_outlineVertices[outlineIndex++].position = p1;
                m_outlineVertices[outlineIndex++].position = p1 + extrusion * m_outlineThickness;
            }
        }

        // Duplicate the first point at the end, to close the outline
        m_outlineVertices[outlineIndex++].position = m_outlineVertices[0].position;
        m_outlineVertices[outlineIndex++].position = m_outlineVertices[1].position;

        // Update outline colors
        UpdateOutlineColors();

        // Update the shape's bounds
        m_bounds = m_outlineVertices.getBounds();
    }


    ////////////////////////////////////////////////////////////
    void Shape::UpdateOutlineColors()
    {
        for (auto& vertex : m_outlineVertices)
            vertex.color = m_outlineColor;
    }

}
