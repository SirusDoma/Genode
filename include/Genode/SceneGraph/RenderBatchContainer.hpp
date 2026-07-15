#pragma once

#include <Genode/Graphics/SpriteBatch.hpp>

namespace Gx
{
    class RenderBatchContainer : public virtual RenderableContainer, public virtual UpdatableContainer
    {
    public:
        RenderBatchContainer() = default;
        RenderBatchContainer(const RenderBatchContainer&) = default;
        RenderBatchContainer(RenderBatchContainer&&) = default;

        RenderBatchContainer& operator=(const RenderBatchContainer&) = default;

        void SetBatchMode(SpriteBatch::Mode batchMode) const;

        void SetBatchUsage(SpriteBatch::Usage batchUsage) const;
        [[nodiscard]] SpriteBatch::Usage GetBatchUsage() const;

        [[nodiscard]] VertexPool& GetVertexPool() const;

    protected:
        void Update(const sf::Time& delta) override;
        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

    private:
        // Batcher is kept separate so it doesn't interfere with SceneGraph hierarchy
        mutable SpriteBatch m_batcher{SpriteBatch::Mode::LayerSort};
    };
}
