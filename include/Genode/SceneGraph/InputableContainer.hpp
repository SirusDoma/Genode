#pragma once

#include <SFML/Window/Event.hpp>

#include <Genode/SceneGraph/Node.hpp>
#include <Genode/Entities/Inputable.hpp>

namespace Gx
{
    class InputableContainer : public virtual Node, public virtual Inputable
    {
    protected:
        InputableContainer() = default;

        std::vector<Inputable*> GetInputableChildren() const;

        bool Input(const sf::Event& ev) override;

    private:
        mutable std::uint64_t m_version{0};
        mutable std::vector<Inputable*> m_inputables;
    };
}
