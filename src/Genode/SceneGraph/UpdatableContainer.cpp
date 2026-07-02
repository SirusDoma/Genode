#include <Genode/SceneGraph/UpdatableContainer.hpp>

namespace Gx
{
    std::vector<Updatable*> UpdatableContainer::GetUpdatableChildren() const
    {
        const auto latestVersion = GetVersion();
        if (m_version != latestVersion)
        {
            m_updatables.clear();
            for (const auto child : GetChildren())
            {
                if (const auto updatable = dynamic_cast<Updatable*>(child))
                    m_updatables.push_back(updatable);
            }

            m_version = latestVersion;
        }

        return m_updatables;
    }

    void UpdatableContainer::Update(const sf::Time& delta)
    {
        for (const auto updatable : GetUpdatableChildren())
            updatable->Update(delta);
    }
}
