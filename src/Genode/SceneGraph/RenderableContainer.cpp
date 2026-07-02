#include <Genode/SceneGraph/RenderableContainer.hpp>

namespace Gx
{
    std::vector<Renderable*> RenderableContainer::GetRenderableChildren() const
    {
        const auto latestVersion = GetVersion();
        if (m_version != latestVersion)
        {
            m_renderables.clear();
            for (const auto child : GetChildren())
            {
                if (const auto renderable = dynamic_cast<Renderable*>(child))
                    m_renderables.push_back(renderable);
            }

            m_version = latestVersion;
        }

        return m_renderables;
    }

    RenderStates RenderableContainer::Render(RenderSurface& surface, RenderStates states) const
    {
        if (!IsVisible())
            return states;

        for (const auto node : GetRenderableChildren())
        {
            states.Layer += 1.0f;
            if (const auto renderable = dynamic_cast<Renderable*>(node))
                renderable->Render(surface, states);
        }

        return states;
    }
}
