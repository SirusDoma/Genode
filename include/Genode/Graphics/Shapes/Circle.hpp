////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2018 Laurent Gomila (laurent@sfml-dev.org)
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

#include <Genode/Graphics/Shape.hpp>

namespace Gx
{
    ////////////////////////////////////////////////////////////
    /// @brief Specialized shape representing a circle
    ////////////////////////////////////////////////////////////
    class Circle : public Shape
    {
    public:
        ////////////////////////////////////////////////////////////
        /// @brief Default constructor
        ///
        /// @param radius     Radius of the circle
        /// @param pointCount Number of points composing the circle
        ////////////////////////////////////////////////////////////
        explicit Circle(float radius = 0, std::size_t pointCount = 30);

        ////////////////////////////////////////////////////////////
        /// @brief Set the radius of the circle
        ///
        /// @param radius New radius of the circle
        ///
        /// @see `GetRadius`
        ////////////////////////////////////////////////////////////
        void SetRadius(float radius);

        ////////////////////////////////////////////////////////////
        /// @brief Get the radius of the circle
        ///
        /// @return Radius of the circle
        ///
        /// @see `SetRadius`
        ///
        ////////////////////////////////////////////////////////////
        [[nodiscard]] float GetRadius() const;

        ////////////////////////////////////////////////////////////
        /// @brief Set the number of points of the circle
        ///
        /// @param count New number of points of the circle
        ///
        /// @see `GetPointCount`
        ////////////////////////////////////////////////////////////
        void SetPointCount(std::size_t count);

        ///////////////////////////////////////////////////////////
        /// @brief Get the number of points of the circle
        ///
        /// @return Number of points of the circle
        ///
        /// @see `SetPointCount`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] std::size_t GetPointCount() const override;

        ////////////////////////////////////////////////////////////
        /// @brief Get a point of the circle
        ///
        /// The returned point is in local coordinates, that is,
        /// the shape's transforms (position, rotation, scale) are
        /// not taken into account.
        /// The result is undefined if \a `index` is out of the valid range.
        ///
        /// @param index Index of the point to get, in range [0 .. GetPointCount() - 1]
        ///
        /// @return index-th point of the shape
        ////////////////////////////////////////////////////////////
        [[nodiscard]] sf::Vector2f GetPoint(std::size_t index) const override;

        ////////////////////////////////////////////////////////////
        /// @brief Get the geometric center of the circle
        ///
        /// The returned point is in local coordinates, that is,
        /// the shape's transforms (position, rotation, scale) are
        /// not taken into account.
        ///
        /// @return The geometric center of the shape
        ////////////////////////////////////////////////////////////
        [[nodiscard]] sf::Vector2f GetGeometricCenter() const override;

    private:
        ////////////////////////////////////////////////////////////
        // Member data
        ////////////////////////////////////////////////////////////
        float       m_radius;     //!< Radius of the circle
        std::size_t m_pointCount; //!< Number of points composing the circle
    };

}