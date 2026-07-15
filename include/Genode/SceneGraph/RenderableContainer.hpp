#pragma once

#include <Genode/SceneGraph/Node.hpp>
#include <Genode/Entities/Renderable.hpp>

namespace Gx
{
    class RenderableContainer : public virtual Node, public virtual Renderable
    {
    protected:
        RenderableContainer() = default;
        RenderableContainer(const RenderableContainer&) = default;
        RenderableContainer(RenderableContainer&&) = default;

        RenderableContainer& operator=(const RenderableContainer&) = default;

        std::vector<Renderable*> GetRenderableChildren() const;

        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

    private:
        mutable std::uint64_t m_version{0};
        mutable std::vector<Renderable*> m_renderables;
    };
}
