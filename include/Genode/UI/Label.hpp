#pragma once

#include <Genode/UI/Control.hpp>
#include <Genode/Graphics/Text.hpp>

#include <unordered_map>

namespace Gx
{
    class Label : public virtual Control, public virtual Text
    {
    public:
        using Text::Text;

        void AddFallbackFont(const Font& font) const;

        [[nodiscard]] sf::FloatRect GetLocalBounds() const override;
        virtual void SetLocalBounds(const sf::FloatRect& bounds);

    protected:
        void Update(const sf::Time& delta) override;
        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

        void OnFontChanged(const Font&) const override;
        void OnGeometryUpdating() const override;

        void Invalidate() override;

    private:
        mutable const Font* m_defaultFont{nullptr};
        mutable std::unordered_set<const Font*> m_fallbackFonts{};

        sf::FloatRect m_bounds{};
    };
}
