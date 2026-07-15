#pragma once

#include <Genode/SceneGraph/Node.hpp>
#include <Genode/Entities/Updatable.hpp>

namespace Gx
{
    class UpdatableContainer : public virtual Node, public virtual Updatable
    {
    protected:
        UpdatableContainer() = default;
        UpdatableContainer(const UpdatableContainer&) = default;
        UpdatableContainer(UpdatableContainer&&) = default;

        UpdatableContainer& operator=(const UpdatableContainer&) = default;

        std::vector<Updatable*> GetUpdatableChildren() const;

        void Update(const sf::Time& delta) override;

    private:
        mutable std::uint64_t m_version{0};
        mutable std::vector<Updatable*> m_updatables;
    };
}
