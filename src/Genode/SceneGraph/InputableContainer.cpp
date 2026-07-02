#include <Genode/SceneGraph/InputableContainer.hpp>

namespace Gx
{
    std::vector<Inputable*> InputableContainer::GetInputableChildren() const
    {
        const auto latestVersion = GetVersion();
        if (m_version != latestVersion)
        {
            m_inputables.clear();
            for (const auto child : GetChildren())
            {
                if (const auto inputable = dynamic_cast<Inputable*>(child))
                    m_inputables.push_back(inputable);
            }

            m_version = latestVersion;
        }

        return m_inputables;
    }

    bool InputableContainer::Input(const sf::Event& ev)
    {
        if (Inputable::Input(ev))
        {
            for (const auto inputable : GetInputableChildren())
                inputable->Input(ev);

            return true;
        }

        return false;
    }
}
