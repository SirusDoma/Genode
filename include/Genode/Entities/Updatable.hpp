#pragma once

#include <SFML/System/Time.hpp>

namespace Gx
{
    class Updatable
    {
    public:
        virtual void Update(const sf::Time& delta) = 0;
        virtual ~Updatable() = default;
    };
}
