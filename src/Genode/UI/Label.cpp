#include <Genode/UI/Label.hpp>
#include <Genode/Graphics/Font.hpp>

namespace Gx
{
    void Label::AddFallbackFont(const Gx::Font& font) const
    {
        if (m_defaultFont == nullptr)
            m_defaultFont = GetFont();

        m_fallbackFonts.insert(&font);
    }

    sf::FloatRect Label::GetLocalBounds() const
    {
        if (m_bounds != sf::FloatRect{})
            return m_bounds;

        return Text::GetLocalBounds();
    }

    void Label::SetLocalBounds(const sf::FloatRect& bounds)
    {
        m_bounds = bounds;
        Invalidate();
    }

    void Label::Update(const sf::Time& delta)
    {
        Control::Update(delta);

        Invalidate();
    }

    RenderStates Label::Render(RenderSurface& surface, RenderStates states) const
    {
        if (!IsVisible())
            return states;

        return Text::Render(surface, states);
    }

    void Label::OnFontChanged(const Gx::Font& font) const
    {
        Text::OnFontChanged(font);
        m_defaultFont = &font;
    }

    void Label::OnGeometryUpdating() const
    {
        Text::OnGeometryUpdating();

        if (m_fallbackFonts.empty())
            return;

        if (m_defaultFont == nullptr)
            m_defaultFont = GetFont();

        auto candidates = std::unordered_set<const Font*>{};
        for (const auto& font : m_fallbackFonts)
            candidates.insert(font);

        bool useDefault = m_defaultFont != nullptr;
        for (const char32_t codepoint : GetString())
        {
            useDefault = useDefault && m_defaultFont->HasGlyph(codepoint);
            for (auto& font : m_fallbackFonts)
            {
                if (candidates.find(font) == candidates.end())
                    continue;

                if (!font->HasGlyph(codepoint))
                    candidates.erase(font);
            }

            if (candidates.empty())
                return;
        }

        if (useDefault)
        {
            const_cast<Label*>(this)->SetFont(*m_defaultFont);
            return;
        }

        for (auto& candidate : candidates)
        {
            if (candidate)
            {
                const_cast<Label*>(this)->SetFont(*candidate);
                return;
            }
        }
    }

    void Label::Invalidate()
    {
        EnsureGeometryUpdate();

        if (m_bounds != sf::FloatRect{})
        {
            auto string  = GetString();
            bool wrapped = true;
            while (wrapped)
            {
                wrapped = false;

                std::size_t checkpoint = 0;
                for (const auto& glyph : GetShapedGlyphs())
                {
                    const auto c = static_cast<std::size_t>(glyph.cluster);
                    if (c >= string.getSize() || string[c] == '\n')
                        continue;

                    if (string[c] == ' ')
                    {
                        checkpoint = c;
                        continue;
                    }

                    const auto position = GetTransform().transformPoint(glyph.position);
                    if (m_bounds.position.x + position.x > m_bounds.position.x + m_bounds.size.x)
                    {
                        if (checkpoint == 0 && c > 0)
                            checkpoint = c - 1;

                        if (string[checkpoint] != '\n')
                        {
                            string.replace(checkpoint, 1, "\n");
                            SetString(string);

                            wrapped = true;
                            break;
                        }

                        checkpoint = 0;
                    }
                }
            }

            EnsureGeometryUpdate();
        }
    }
}
