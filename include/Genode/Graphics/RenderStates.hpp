#pragma once

#include <SFML/Graphics/RenderStates.hpp>

namespace Gx
{
    class SpriteBatch;
    class RenderStates : public sf::RenderStates
    {
    public:
        using sf::RenderStates::RenderStates;

        const unsigned int FrameID = 0;
        const double Delta         = 0;
        float Layer                = 0.f;

        static const RenderStates Default;

        // ReSharper disable CppNonExplicitConvertingConstructor
        RenderStates(const sf::BlendMode& blendMode);
        RenderStates(const sf::StencilMode& stencilMode);
        RenderStates(const sf::Transform& transform);
        RenderStates(const sf::Texture* texture);
        RenderStates(const sf::Shader* shader);
        // ReSharper restore CppNonExplicitConvertingConstructor

        explicit RenderStates(const sf::RenderStates& states, unsigned int frameID = 0, double delta = 0);

        RenderStates& operator=(const RenderStates& states);

    private:
        friend class Application;
    };
}
