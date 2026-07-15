#include <Genode/SceneGraph/RenderBatchContainer.hpp>

namespace Gx
{
    void RenderBatchContainer::SetBatchMode(const SpriteBatch::Mode batchMode) const
    {
        m_batcher.SetBatchMode(batchMode);
    }

    void RenderBatchContainer::SetBatchUsage(const SpriteBatch::Usage batchUsage) const
    {
        m_batcher.SetBatchUsage(batchUsage);
    }

    SpriteBatch::Usage RenderBatchContainer::GetBatchUsage() const
    {
        return m_batcher.GetBatchUsage();
    }

    VertexPool& RenderBatchContainer::GetVertexPool() const
    {
        return m_batcher.GetVertexPool();
    }

    void RenderBatchContainer::Update(const sf::Time& delta)
    {
        m_batcher.Update(delta);
        UpdatableContainer::Update(delta);
    }

    RenderStates RenderBatchContainer::Render(RenderSurface& surface, RenderStates states) const
    {
        if (!IsVisible())
            return states;

        // Push render states
        const auto transform = states.transform;
        const float layer    = states.Layer;

        // Reset transform
        states.transform = sf::Transform();

        // Render child with sprite batch
        for (const auto node : GetRenderableChildren())
        {
            states.Layer += 1.f;
            if (const auto renderable = dynamic_cast<Renderable*>(node))
                renderable->Render(m_batcher, states);

            // Pop batch level
            states.Layer = layer;
        }

        // Pop transform
        states.transform = transform;

        // Render sprite batch
        return m_batcher.Render(surface, states);
    }
}