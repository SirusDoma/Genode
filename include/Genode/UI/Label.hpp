#pragma once

#include <Genode/UI/Control.hpp>
#include <Genode/Graphics/Text.hpp>

#include <unordered_map>

namespace Gx
{
    class Label : public virtual Control, public virtual Text
    {
    public:
        enum class VerticalAlignment { Top, Center, Bottom };

        using Text::Text;

        void AddFallbackFont(const Font& font) const;

        void SetString(const sf::String& string);

        [[nodiscard]] VerticalAlignment GetVerticalAlignment() const;
        void SetVerticalAlignment(VerticalAlignment alignment);

        [[nodiscard]] const sf::String& GetEllipsis() const;
        void SetEllipsis(const sf::String& ellipsis);

        [[nodiscard]] sf::FloatRect GetLocalBounds() const override;
        virtual void SetLocalBounds(const sf::FloatRect& bounds);

        void ClipQuads(const sf::FloatRect& rect) const;

    protected:
        void Update(const sf::Time& delta) override;
        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

        void OnFontChanged(const Font&) const override;
        void OnGeometryUpdating() const override;
        void OnGeometryUpdated() const override;

        void Invalidate() override;

    private:
        void EnsureLayout() const;
        void Layout() const;
        void SetDisplayString(const sf::String& string) const;

        mutable const Font* m_defaultFont{nullptr};
        mutable std::unordered_set<const Font*> m_fallbackFonts{};

        sf::FloatRect m_bounds{};
        sf::String m_ellipsis{};
        VerticalAlignment m_verticalAlignment{VerticalAlignment::Center};
        mutable sf::String m_sourceString{};
        mutable sf::Vector2f m_alignOffset{};
        mutable bool m_hasSource{};
        mutable bool m_layoutNeeded{};
    };
}
