////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2026 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <Genode/Graphics/Text.hpp>
#include <Genode/Graphics/Font.hpp>

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <SheenBidi/SheenBidi.h>
#include <algorithm>
#include <hb-ft.h>
#include <iterator>
#include <limits>
#include <mutex>
#include <string>
#include <utility>

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace
{
    struct CacheID
    {
        typedef std::uint64_t sf::Texture::* type;
        friend type Get(CacheID);
    };

    template<typename Tag, typename Tag::type M>
    struct Private
    {
        friend typename Tag::type Get(Tag)
        {
            return M;
        }
    };

    template struct Private<CacheID, &sf::Texture::m_cacheId>;

    // Add a horizontal underline or strikethrough line to the vertex array
    void AddLineHorizontal(sf::VertexArray& vertices,
                           const float      lineLeft,
                           const float      lineRight,
                           const float      lineTop,
                           const sf::Color  color,
                           const float      offset,
                           const float      thickness,
                           const float      outlineThickness = 0)
    {
        const float top    = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
        const float bottom = top + std::floor(thickness + 0.5f);

        vertices.append({{lineLeft - outlineThickness, top - outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{lineRight + outlineThickness, top - outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{lineLeft - outlineThickness, bottom + outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{lineLeft - outlineThickness, bottom + outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{lineRight + outlineThickness, top - outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{lineRight + outlineThickness, bottom + outlineThickness}, color, {1.0f, 1.0f}});
    }

    // Add a vertical strikethrough line to the vertex array
    void AddLineVertical(sf::VertexArray& vertices,
                         const float      lineTop,
                         const float      lineBottom,
                         const sf::Color  color,
                         const float      offset,
                         const float      thickness,
                         const float      outlineThickness = 0)
    {
        const float left  = std::floor(offset - (thickness / 2) + 0.5f);
        const float right = left + std::floor(thickness + 0.5f);

        vertices.append({{left - outlineThickness, lineTop - outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{right + outlineThickness, lineTop - outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{left - outlineThickness, lineBottom + outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{left - outlineThickness, lineBottom + outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{right + outlineThickness, lineTop - outlineThickness}, color, {1.0f, 1.0f}});
        vertices.append({{right + outlineThickness, lineBottom + outlineThickness}, color, {1.0f, 1.0f}});
    }

    // Add a glyph quad to the vertex array
    void AddGlyphQuad(sf::VertexArray& vertices, const sf::Vector2f position, const sf::Color color,
        const sf::Glyph& glyph, const float italicShear, const sf::Vector2f offset = sf::Vector2f(0.f, 0.f),
        const sf::Vector2f padding = sf::Vector2f(1.f, 1.f))
    {
        const sf::Vector2f p1 = glyph.bounds.position - sf::Vector2f{1.f, 1.f};
        const sf::Vector2f p2 = glyph.bounds.position + glyph.bounds.size + padding + offset;

        const auto uv1 = sf::Vector2f(glyph.textureRect.position) - padding;
        const auto uv2 = sf::Vector2f(glyph.textureRect.position + glyph.textureRect.size) + padding;

        vertices.append({position + sf::Vector2f(p1.x - italicShear * p1.y, p1.y), color, {uv1.x, uv1.y}});
        vertices.append({position + sf::Vector2f(p2.x - italicShear * p1.y, p1.y), color, {uv2.x, uv1.y}});
        vertices.append({position + sf::Vector2f(p1.x - italicShear * p2.y, p2.y), color, {uv1.x, uv2.y}});
        vertices.append({position + sf::Vector2f(p1.x - italicShear * p2.y, p2.y), color, {uv1.x, uv2.y}});
        vertices.append({position + sf::Vector2f(p2.x - italicShear * p1.y, p1.y), color, {uv2.x, uv1.y}});
        vertices.append({position + sf::Vector2f(p2.x - italicShear * p2.y, p2.y), color, {uv2.x, uv2.y}});
    }

    struct TextSegment
    {
        std::size_t    offset{};
        std::size_t    length{};
        hb_script_t    script{};
        hb_direction_t direction{};
    };

    // Split string into segments with uniform text properties
    std::vector<TextSegment> SegmentString(const sf::String& input)
    {
        std::vector<TextSegment> segments;

        const SBCodepointSequence codepointSequence{SBStringEncodingUTF32,
                                                    static_cast<const void*>(input.getData()),
                                                    input.getSize()};
        auto* const               scriptLocator   = SBScriptLocatorCreate();
        const auto* const         algorithm       = SBAlgorithmCreate(&codepointSequence);
        SBUInteger                paragraphOffset = 0;

        while (paragraphOffset < input.getSize())
        {
            SBUInteger paragraphLength{};
            SBUInteger separatorLength{};
            SBAlgorithmGetParagraphBoundary(algorithm,
                                            paragraphOffset,
                                            std::numeric_limits<SBUInteger>::max(),
                                            &paragraphLength,
                                            &separatorLength);

            // If the paragraph contains characters besides the separator,
            // split the separator off into its own paragraph in the next iteration
            // We do this to ensure line breaks are inserted into segments last
            // after all character runs on the same line have already been inserted
            // This allows us to draw our segments in left-to-right top-to-bottom order
            if (separatorLength < paragraphLength)
                paragraphLength -= separatorLength;

            const auto* const paragraph = SBAlgorithmCreateParagraph(algorithm, paragraphOffset, paragraphLength, SBLevelDefaultLTR);
            const auto* const line      = SBParagraphCreateLine(paragraph, paragraphOffset, paragraphLength);
            const auto        runCount  = SBLineGetRunCount(line);
            const auto*       runArray  = SBLineGetRunsPtr(line);

            for (SBUInteger i = 0; i < runCount; i++)
            {
                // Odd levels are RTL, even levels are LTR
                const auto direction = (runArray[i].level % 2) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;

                const SBCodepointSequence codepointSubsequence{SBStringEncodingUTF32,
                                                               static_cast<const void*>(input.getData() + runArray[i].offset),
                                                               runArray[i].length};

                SBScriptLocatorLoadCodepoints(scriptLocator, &codepointSubsequence);

                while (SBScriptLocatorMoveNext(scriptLocator))
                {
                    const auto* agent  = SBScriptLocatorGetAgent(scriptLocator);
                    const auto  script = hb_script_from_iso15924_tag(SBScriptGetUnicodeTag(agent->script));

                    segments.emplace_back(TextSegment{runArray[i].offset + agent->offset, agent->length, script, direction});
                }

                SBScriptLocatorReset(scriptLocator);
            }

            SBLineRelease(line);
            SBParagraphRelease(paragraph);

            paragraphOffset += paragraphLength;
        }


        SBAlgorithmRelease(algorithm);
        SBScriptLocatorRelease(scriptLocator);

        return segments;
    }
}


namespace Gx
{
    ////////////////////////////////////////////////////////////
    struct Text::ShaperImpl
    {
        // HarfBuzz font objects are linked to a specific
        // FreeType face and a specific character size
        // If we need to shape text using another font or another
        // character size we will have to create a new shaper
        ShaperImpl(void* fontHandle, const std::uint64_t theFontId, const unsigned int theCharacterWidth, const unsigned int theCharacterHeight) :
            fontId(theFontId),
            characterWidth(theCharacterWidth),
            characterHeight(theCharacterHeight),
            shaper(hb_ft_font_create(static_cast<FT_Face>(fontHandle), nullptr)),
            shapingBuffer(hb_buffer_create())
        {
            // Make HarfBuzz use FreeType font functions
            hb_ft_font_set_funcs(shaper.get());
        }

        // To save on memory, instead of every sf::Text object having
        // its own shaper, we try to share shapers among multiple
        // sf::Text objects if they all reference the same font and
        // want to shape text at the same character size
        // Because we are allowing simultaneous access to shapers
        // from multiple sf::Text objects we have to guard both the
        // shaper cache and the shaper objects (which contain buffers)
        // themselves using mutexes
        static std::shared_ptr<ShaperImpl> GetShaper(void* fontHandle, std::uint64_t fontId, unsigned int characterWidth, unsigned int characterHeight)
        {
            // Our shaper cache
            struct ShaperCache
            {
                std::mutex                             mutex;
                std::vector<std::weak_ptr<ShaperImpl>> cache;
            };
            static ShaperCache    shaperCache;
            const std::lock_guard lock(shaperCache.mutex);

            std::shared_ptr<ShaperImpl> result;

            // Iterate over the cache looking for a font ID and character size match
            // while at the same time cleaning out stale cache entries
            for (auto iter = shaperCache.cache.begin(); iter != shaperCache.cache.end();)
            {
                if (auto shaper = iter->lock(); !shaper)
                {
                    // Remove stale entries
                    iter = shaperCache.cache.erase(iter);
                }
                else
                {
                    if (shaper->fontId == fontId && shaper->characterWidth == characterWidth && shaper->characterHeight == characterHeight)
                        result = std::move(shaper);

                    // Don't break even if we find a match since we want to finish cleaning up
                    ++iter;
                }
            }

            if (!result)
            {
                // We didn't find a cached shaper, create a new one
                result = std::make_shared<ShaperImpl>(fontHandle, fontId, characterWidth, characterHeight);
                shaperCache.cache.emplace_back(result);
            }

            return result;
        }

        // The glyph data that is output after shaping
        struct GlyphData
        {
            std::uint32_t  id{};        //!< Glyph ID (not codepoint!)
            std::uint32_t  cluster{};   //!< Cluster the glyph belongs to
            sf::Vector2f   offset;      //!< The offset to apply to the glyph
            sf::Vector2f   advance;     //!< The advance to apply to the cursor when transitioning to the next glyph
            hb_direction_t direction{}; //!< Text direction
        };

        // Our shape function
        std::vector<GlyphData> Shape(
            const sf::String&                 input,
            const std::vector<std::uint32_t>& indices,
            hb_script_t                       script,
            hb_direction_t                    direction,
            TextOrientation                   orientation,
            ClusterGrouping                   clusterGrouping,
            float                             outlineThickness,
            std::uint32_t                     style)
        {
            assert(input.getSize() == indices.size() && "Input string length does not match indices count");

            // Adjust the direction if a vertical orientation was set
            if (orientation == TextOrientation::TopToBottom)
            {
                direction = HB_DIRECTION_TTB;
            }
            else if (orientation == TextOrientation::BottomToTop)
            {
                direction = HB_DIRECTION_BTT;
            }

            const std::lock_guard lock(mutex);
            auto*                 buffer = shapingBuffer.get();

            // Clear out and add the input to the buffer
            hb_buffer_clear_contents(buffer);
            hb_buffer_pre_allocate(buffer, static_cast<unsigned int>(input.getSize()));

            // Instead of using hb_buffer_add_utf32, we have to use hb_buffer_add
            // to specify the initial cluster IDs for every character
            for (auto i = 0u; i < input.getSize(); ++i)
                hb_buffer_add(buffer, input[i], indices[i]);

            // hb_buffer_add doesn't automatically set the buffer content type so do it now
            hb_buffer_set_content_type(buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);

            // Set the script and direction we detected during segmentation
            hb_buffer_set_script(buffer, script);
            hb_buffer_set_direction(buffer, direction);

            // Try to guess the language of the text the user provided
            hb_buffer_guess_segment_properties(buffer);

            // Set the cluster level
            switch (clusterGrouping)
            {
                case ClusterGrouping::Grapheme:
                    hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
                    break;
                case ClusterGrouping::Character:
                    hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
                    break;
                case ClusterGrouping::None:
                    hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_CHARACTERS);
                    break;
            }

            // Set load flags analogous to the Font implementation
            FT_Int32 flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT;
            if (outlineThickness != 0)
                flags |= FT_LOAD_NO_BITMAP;
            hb_ft_font_set_load_flags(shaper.get(), flags);

            // Shape the text
            hb_shape(shaper.get(), buffer, nullptr, 0);

            // Retrieve position data glyph by glyph
            const auto  glyphCount     = hb_buffer_get_length(buffer);
            const auto* glyphInfo      = hb_buffer_get_glyph_infos(buffer, nullptr);
            const auto* glyphPositions = hb_buffer_get_glyph_positions(buffer, nullptr);

            std::vector<GlyphData> output;
            output.reserve(glyphCount);

            // HarfBuzz returns position data in scaled units
            // We need to convert them to pixels by dividing by the
            // current font scale factoring out the current character size
            // Also flip Y since HarfBuzz uses positive Y up coordinates
            sf::Vector2i scale;
            hb_font_get_scale(shaper.get(), &scale.x, &scale.y);
            const sf::Vector2f divisor{static_cast<float>(scale.x) / static_cast<float>(characterWidth == 0 ? characterHeight : characterWidth),
                                   static_cast<float>(scale.y) / -static_cast<float>(characterHeight)};

            for (auto i = 0u; i < glyphCount; ++i)
            {
                auto& glyphData = output.emplace_back(
                    GlyphData{glyphInfo[i].codepoint,
                              glyphInfo[i].cluster,
                              {std::round(static_cast<float>(glyphPositions[i].x_offset) / divisor.x),
                               std::round(static_cast<float>(glyphPositions[i].y_offset) / divisor.y)},
                              {std::round(static_cast<float>(glyphPositions[i].x_advance) / divisor.x),
                               std::round(static_cast<float>(glyphPositions[i].y_advance) / divisor.y)},
                              hb_buffer_get_direction(buffer)});

                // Adjust advances if we are shaping bold text
                // Use 1.0f to match the advance adjustment the Font applies when emboldening glyphs
                if ((style & Text::Bold) != 0)
                {
                    glyphData.advance.x += (glyphData.advance.x != 0.0f) ? ((glyphData.advance.x >= 0.0f) ? 1.0f : -1.0f) : 0.0f;
                    glyphData.advance.y += (glyphData.advance.y != 0.0f) ? ((glyphData.advance.y >= 0.0f) ? 1.0f : -1.0f) : 0.0f;
                }
            }

            return output;
        }

        struct ShaperDeleter
        {
            void operator()(hb_font_t* pointer) const
            {
                hb_font_destroy(pointer);
            }
        };

        struct ShaperBufferDeleter
        {
            void operator()(hb_buffer_t* buffer) const
            {
                hb_buffer_destroy(buffer);
            }
        };

        std::mutex                                        mutex;             //!< Mutex guarding our shaper and shaping buffer
        const std::uint64_t                               fontId{};          //!< Font ID this shaper is linked to
        const unsigned int                                characterWidth{};  //!< Character size this shaper was created with
        const unsigned int                                characterHeight{}; //!< Character size this shaper was created with
        std::unique_ptr<hb_font_t, ShaperDeleter>         shaper;            //!< Our shaper
        std::unique_ptr<hb_buffer_t, ShaperBufferDeleter> shapingBuffer;     //!< The buffer to perform shaping with
    };


    ////////////////////////////////////////////////////////////
    Text::Text(const Font& font, sf::String  string, const unsigned int characterSize) :
        m_string             (std::move(string)),
        m_font               (&font),
        m_characterHeight    (characterSize),
        m_geometryNeedUpdate (true)
    {
    }


    ////////////////////////////////////////////////////////////
    void Text::SetString(const sf::String& string)
    {
        if (m_string != string)
        {
            m_string             = string;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetFont(const Font& font)
    {
        if (m_font != &font)
        {
            m_font               = &font;
            m_geometryNeedUpdate = true;

            OnFontChanged(*m_font);
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetMasked(const bool masked)
    {
        if (m_masked != masked)
        {
            m_masked             = masked;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetMaskingCharacter(const char32_t character)
    {
        if (m_maskingCharacter != character)
        {
            m_maskingCharacter   = character;
            m_geometryNeedUpdate = m_geometryNeedUpdate || m_masked;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetCharacterSize(const unsigned int size)
    {
        if (m_characterHeight != size || m_characterWidth != 0)
        {
            m_characterHeight    = size;
            m_characterWidth     = 0;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetCharacterWidth(const unsigned int width)
    {
        if (m_characterWidth != width)
        {
            m_characterWidth     = width;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetCharacterHeight(unsigned int height)
    {
        if (m_characterHeight != height)
        {
            m_characterHeight    = height;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetItalicShear(const sf::Angle& italicSheer)
    {
        m_italicShear = italicSheer;
    }


    ////////////////////////////////////////////////////////////
    void Text::SetLetterSpacing(const float spacing)
    {
        if (m_letterSpacing != spacing)
        {
            m_letterSpacing      = spacing;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetLineSpacing(const float spacing)
    {
        if (m_lineSpacing != spacing)
        {
            m_lineSpacing        = spacing;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetStyle(const std::uint32_t style)
    {
        if (m_style != style)
        {
            m_style = style;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetColor(const sf::Color& color)
    {
        if (color != m_fillColor)
        {
            m_fillColor = color;

            // Change vertex colors directly, no need to update whole geometry
            // (if geometry is updated anyway, we can skip this step)
            if (!m_geometryNeedUpdate)
            {
                for (auto& vertex : m_vertices)
                    vertex.color = m_fillColor;
            }
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetOutlineColor(const sf::Color& color)
    {
        if (color != m_outlineColor)
        {
            m_outlineColor = color;

            // Change vertex colors directly, no need to update whole geometry
            // (if geometry is updated anyway, we can skip this step)
            if (!m_geometryNeedUpdate)
            {
                for (auto& vertex : m_outlineVertices)
                    vertex.color = m_outlineColor;
            }
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetOutlineThickness(const float thickness)
    {
        if (thickness != m_outlineThickness)
        {
            m_outlineThickness   = thickness;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetOutlineOffset(const sf::Vector2f& offset)
    {
        if (offset != m_outlineOffset)
        {
            m_outlineOffset      = offset;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetLineAlignment(const LineAlignment lineAlignment)
    {
        if (m_lineAlignment != lineAlignment)
        {
            m_lineAlignment      = lineAlignment;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetTextOrientation(const TextOrientation textOrientation)
    {
        if (m_textOrientation != textOrientation)
        {
            m_textOrientation    = textOrientation;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    const sf::String& Text::GetString() const
    {
        return m_string;
    }


    ////////////////////////////////////////////////////////////
    const Font* Text::GetFont() const
    {
        return m_font;
    }


    ////////////////////////////////////////////////////////////
    bool Text::IsMasked() const
    {
        return m_masked;
    }


    ////////////////////////////////////////////////////////////
    char32_t Text::GetMaskingCharacter() const
    {
        return m_maskingCharacter;
    }


    ////////////////////////////////////////////////////////////
    unsigned int Text::GetCharacterSize() const
    {
        return m_characterHeight;
    }


    ////////////////////////////////////////////////////////////
    unsigned int Text::GetCharacterWidth() const
    {
        return m_characterWidth;
    }


    ////////////////////////////////////////////////////////////
    unsigned int Text::GetCharacterHeight() const
    {
        return m_characterHeight;
    }


    ////////////////////////////////////////////////////////////
    float Text::GetLetterSpacing() const
    {
        return m_letterSpacing;
    }


    ////////////////////////////////////////////////////////////
    float Text::GetLineSpacing() const
    {
        return m_lineSpacing;
    }


    ////////////////////////////////////////////////////////////
    std::uint32_t Text::GetStyle() const
    {
        return m_style;
    }


    ////////////////////////////////////////////////////////////
    sf::Angle Text::GetItalicShear() const
    {
        return m_italicShear;
    }


    ////////////////////////////////////////////////////////////
    const sf::Color& Text::GetColor() const
    {
        return m_fillColor;
    }


    ////////////////////////////////////////////////////////////
    const sf::Color& Text::GetOutlineColor() const
    {
        return m_outlineColor;
    }


    ////////////////////////////////////////////////////////////
    float Text::GetOutlineThickness() const
    {
        return m_outlineThickness;
    }


    ////////////////////////////////////////////////////////////
    sf::Vector2f Text::GetOutlineOffset() const
    {
        return m_outlineOffset;
    }


    ////////////////////////////////////////////////////////////
    Text::LineAlignment Text::GetLineAlignment() const
    {
        return m_lineAlignment;
    }


    ////////////////////////////////////////////////////////////
    Text::TextOrientation Text::GetTextOrientation() const
    {
        return m_textOrientation;
    }


    ////////////////////////////////////////////////////////////
    sf::Vector2f Text::FindCharacterPosition(std::size_t index) const
    {
        EnsureGeometryUpdate();

        // Make sure that we have a valid font
        if (!m_font)
            return sf::Vector2f();

        // Adjust the index if it's out of range
        index = std::min(index, m_glyphs.size());

        // Precompute the variables needed by the algorithm
        const float letterSpacing = m_letterSpacing;
        const float lineSpacing   = m_font->GetLineSpacing(m_characterWidth, m_characterHeight) + m_lineSpacing;

        // Special handling for newlines:
        // The old shaping implementation placed newline characters
        // at the start of the new line that they create
        // The new implementation places them at the end of the previous line
        // Placing them at the end of the previous line is normally what is expected
        // since the cursor is rendered before the glyph whose location marks the
        // location at which new characters will be inserted into the text
        // We provide a workaround for the old behavior until FindCharacterPosition can be removed

        if (m_glyphs.empty())
        {
            // Get the position of the Text itself in global coordinates
            return GetTransform().transformPoint({});
        }

        if (index == m_glyphs.size())
        {
            // Return the position of the last glyph + its advance in global coordinates
            const auto& lastGlyph = m_glyphs.back();

            // Newline, past-the-end
            if (m_string[m_string.getSize() - 1] == '\n')
                return GetTransform().transformPoint(
                    sf::Vector2f{0.0f, lastGlyph.position.y + lineSpacing - static_cast<float>(m_characterHeight)});

            return GetTransform().transformPoint(
                lastGlyph.position + sf::Vector2f{lastGlyph.glyph.advance + letterSpacing, -static_cast<float>(m_characterHeight)});
        }

        // Newline, not past-the-end
        if (m_string[index] == '\n')
            return GetTransform().transformPoint(
                sf::Vector2f{0.0f, m_glyphs[index].position.y + lineSpacing - static_cast<float>(m_characterHeight)});

        return GetTransform().transformPoint(m_glyphs[index].position - sf::Vector2f{0.0f, static_cast<float>(m_characterHeight)});
    }


    ////////////////////////////////////////////////////////////
    void Text::Truncate(const std::size_t maxWidth)
    {
        if (maxWidth == 0)
        {
            SetString(sf::String());
            return;
        }

        EnsureGeometryUpdate();
        if (m_bounds.size.x < maxWidth)
            return;

        // Find the last cluster whose glyph still fits entirely within the maximum width
        // Whitespace advances the pen through the shaped positions but cannot be the cut point
        std::size_t index = 0;
        for (const auto& glyph : m_glyphs)
        {
            // Newlines reset the shaped positions to the start of the next line, skip them
            if (!m_masked && glyph.cluster < m_string.getSize() && m_string[glyph.cluster] == U'\n')
                continue;

            const bool hasAdvance = glyph.glyph.advance > 0.0f;
            const auto right      = glyph.position.x + glyph.glyph.advance + ((hasAdvance) ? m_letterSpacing : 0.0f);

            if (right >= static_cast<float>(maxWidth))
                break;

            if (glyph.glyph.textureRect.size.x != 0 && glyph.glyph.textureRect.size.y != 0)
                index = glyph.cluster;
        }

        if (m_string.getSize() - (index + 1) > 1)
            SetString(m_string.substring(0, index + 1));
    }


    ////////////////////////////////////////////////////////////
    const std::vector<Text::ShapedGlyph>& Text::GetShapedGlyphs() const
    {
        EnsureGeometryUpdate();

        return m_glyphs;
    }


    ////////////////////////////////////////////////////////////
    Text::ClusterGrouping Text::GetClusterGrouping() const
    {
        return m_clusterGrouping;
    }


    ////////////////////////////////////////////////////////////
    void Text::SetClusterGrouping(const ClusterGrouping clusterGrouping)
    {
        if (m_clusterGrouping != clusterGrouping)
        {
            m_clusterGrouping    = clusterGrouping;
            m_geometryNeedUpdate = true;
        }
    }


    ////////////////////////////////////////////////////////////
    void Text::SetGlyphPreProcessor(GlyphPreProcessor glyphPreProcessor)
    {
        m_glyphPreProcessor  = std::move(glyphPreProcessor);
        m_geometryNeedUpdate = true;
    }


    ////////////////////////////////////////////////////////////
    sf::VertexArray& Text::GetVertexData() const
    {
        return m_vertices;
    }


    ////////////////////////////////////////////////////////////
    sf::VertexArray& Text::GetOutlineVertexData() const
    {
        return m_outlineVertices;
    }


    ////////////////////////////////////////////////////////////
    sf::FloatRect Text::GetLocalBounds() const
    {
        EnsureGeometryUpdate();

        return m_bounds;
    }


    ////////////////////////////////////////////////////////////
    sf::FloatRect Text::GetGlobalBounds() const
    {
        return GetTransform().transformRect(GetLocalBounds());
    }


    ////////////////////////////////////////////////////////////
    RenderStates Text::Render(RenderSurface& surface, RenderStates states) const
    {
        if (!IsVisible())
            return states;

        if (m_font)
        {
            EnsureGeometryUpdate();

            states.transform *= GetTransform();
            states.texture    = &m_font->GetTexture(m_characterWidth, m_characterHeight);

            // Only draw the outline if there is something to draw
            if (m_outlineThickness != 0)
                surface.Render(m_outlineVertices, states);

            surface.Render(m_vertices, states);
        }

        return RenderableContainer::Render(surface, states);
    }

    void Text::EnsureGeometryUpdate() const
    {
        if (!m_font)
            return;

        // Do nothing, if geometry has not changed and the font texture has not changed
        const auto cacheId = m_font->GetTexture(m_characterWidth, m_characterHeight).*Get(CacheID());
        if (!m_geometryNeedUpdate && cacheId == m_fontTextureId)
            return;

        OnGeometryUpdating();

        // Save the current fonts texture id
        m_fontTextureId = cacheId;

        // Mark geometry as updated
        m_geometryNeedUpdate = false;

        // Clear the previous geometry
        m_vertices.clear();
        m_outlineVertices.clear();
        m_glyphs.clear();
        m_bounds = sf::FloatRect();

        // No text: nothing to draw
        if (m_string.isEmpty())
            return;

        // Substitute every character with the masking character when masking is enabled
        sf::String maskedString;
        if (m_masked)
            maskedString = sf::String(std::u32string(m_string.getSize(), m_maskingCharacter));

        const sf::String& string = m_masked ? maskedString : m_string;

        // Compute values related to the text style
        const bool  isBold             = m_style & Bold;
        const bool  isUnderlined       = m_style & Underlined;
        const bool  isStrikeThrough    = m_style & StrikeThrough;
        const float italicShear        = m_style & Italic ? m_italicShear.asRadians() : 0.f;
        const float underlineOffset    = m_font->GetUnderlinePosition(m_characterWidth, m_characterHeight);
        const float underlineThickness = m_font->GetUnderlineThickness(m_characterWidth, m_characterHeight);

        // Compute the location of the strike through dynamically
        // We use the center point of the lowercase 'x' glyph as the reference
        // We reuse the underline thickness as the thickness of the strike through as well
        const float strikeThroughOffset = m_font->GetGlyph(U'x', m_characterWidth, m_characterHeight, isBold).bounds.getCenter().y;

        // Precompute the variables needed by the algorithm
        const float letterSpacing = m_letterSpacing;
        const float lineSpacing   = m_font->GetLineSpacing(m_characterWidth, m_characterHeight) + m_lineSpacing;
        float x                   = 0.f;
        auto y                    = (m_textOrientation == TextOrientation::Default) ? static_cast<float>(m_characterHeight) : 0.0f;

        // Create one quad for each character
        auto minX = static_cast<float>(m_characterWidth);
        auto minY = static_cast<float>(m_characterHeight);
        auto maxX = 0.f;
        auto maxY = 0.f;

        // Check that we have a usable font
        const auto  fontId     = m_font->GetInfo().id;
        auto* const fontHandle = m_font->GetFontHandle();

        assert(fontId && fontHandle && "Font not usable for shaping text");

        // Ensure the font is set to the size expected by the shaper
        if (!m_font->SetCurrentSize(m_characterWidth, m_characterHeight))
        {
            assert(false && "Failed to set font size");
            return;
        }

        // Shaping only supports single lines, we will need to
        // break multi-line strings into individual lines ourselves
        sf::String                 currentLine;
        std::vector<std::uint32_t> currentLineIndices;
        std::vector<std::uint32_t> currentLineTabIndices;
        hb_script_t                currentScript{};
        hb_direction_t             currentDirection{};

        const auto outputLine = [&]
        {
            if (!m_shaper || m_shaper->fontId != fontId || m_shaper->characterWidth != m_characterWidth || m_shaper->characterHeight != m_characterHeight)
            {
                // We need to get a new shaper implementation
                m_shaper = ShaperImpl::GetShaper(fontHandle, fontId, m_characterWidth, m_characterHeight);
            }

            const auto shapeOutput = m_shaper->Shape(currentLine,
                                                     currentLineIndices,
                                                     currentScript,
                                                     currentDirection,
                                                     m_textOrientation,
                                                     m_clusterGrouping,
                                                     m_outlineThickness,
                                                     m_style & Bold);

            // Variables used to compute bounds for the current line
            auto lineMinX = static_cast<float>(m_characterWidth);
            auto lineMinY = static_cast<float>(m_characterHeight);
            auto lineMaxX = 0.0f;
            auto lineMaxY = 0.0f;

            // Track widths and y positions of underline segments when applying underline to vertical text
            struct VerticalUnderlineEntry
            {
                sf::Vector2f position;
                float        width{};
            };

            std::vector<VerticalUnderlineEntry> verticalUnderlineData;

            if (m_textOrientation != TextOrientation::Default)
                verticalUnderlineData.reserve(shapeOutput.size());

            // We want to combine the multiple spaces we substituted a tab with back into a single glyph
            // To do that we just enlarge the first of its shaped spaces to 4 times its width
            // We then skip the following 3 spaces
            auto glyphsToSkip = 0;

            // Create one quad for each character
            for (const auto& shapeGlyph : shapeOutput)
            {
                // Skip trailing spaces of a tab
                if (glyphsToSkip > 0)
                {
                    --glyphsToSkip;
                    continue;
                }

                // Extract the current glyph's description
                const sf::Glyph& glyph = m_font->GetGlyphByID(shapeGlyph.id, m_characterWidth, m_characterHeight, isBold);

                // Add the glyph to the glyph list
                auto& glyphEntry    = m_glyphs.emplace_back(ShapedGlyph{glyph, {}, {}, {}});
                glyphEntry.cluster  = shapeGlyph.cluster;
                glyphEntry.position = sf::Vector2f{x, y} + shapeGlyph.offset;
                auto isHorizontal   = false;
                auto isVertical     = false;

                switch (shapeGlyph.direction)
                {
                    case HB_DIRECTION_LTR:
                        glyphEntry.textDirection = TextDirection::LeftToRight;
                        glyphEntry.glyph.advance = shapeGlyph.advance.x;
                        glyphEntry.baseline      = y;
                        isHorizontal             = true;
                        break;
                    case HB_DIRECTION_RTL:
                        glyphEntry.textDirection = TextDirection::RightToLeft;
                        glyphEntry.glyph.advance = shapeGlyph.advance.x;
                        glyphEntry.baseline      = y;
                        isHorizontal             = true;
                        break;
                    case HB_DIRECTION_TTB:
                        glyphEntry.textDirection = TextDirection::TopToBottom;
                        glyphEntry.glyph.advance = shapeGlyph.advance.y;
                        glyphEntry.baseline      = x;
                        isVertical               = true;
                        break;
                    case HB_DIRECTION_BTT:
                        glyphEntry.textDirection = TextDirection::BottomToTop;
                        glyphEntry.glyph.advance = shapeGlyph.advance.y;
                        glyphEntry.baseline      = x;
                        isVertical               = true;
                        break;
                    default:
                        glyphEntry.textDirection = TextDirection::Unspecified;
                        break;
                }

                std::uint32_t style            = m_style;
                sf::Color     fillColor        = m_fillColor;
                sf::Color     outlineColor     = m_outlineColor;
                float         outlineThickness = m_outlineThickness;
                float         italicShear      = 0.0f;

                // Add the glyph to the vertices, if the texture rect has an area
                // (not the case with transparent glyphs e.g. space character)
                if (glyph.textureRect.size.x != 0 && glyph.textureRect.size.y != 0)
                {
                    if (m_glyphPreProcessor)
                        m_glyphPreProcessor(glyphEntry, style, fillColor, outlineColor, outlineThickness);

                    italicShear = (style & Italic) ? m_italicShear.asRadians() : 0.0f;

                    // Apply the outline
                    if (outlineThickness != 0)
                    {
                        const sf::Glyph& outlineGlyph = m_font->GetGlyphByID(shapeGlyph.id, m_characterWidth, m_characterHeight, style & Bold, outlineThickness);

                        // Add the outline glyph to the vertices
                        AddGlyphQuad(m_outlineVertices, glyphEntry.position, outlineColor, outlineGlyph, italicShear, m_outlineOffset);
                    }

                    glyphEntry.vertexOffset = m_vertices.getVertexCount();

                    const sf::Glyph& fillGlyph = m_font->GetGlyphByID(shapeGlyph.id, m_characterWidth, m_characterHeight, style & Bold);
                    AddGlyphQuad(m_vertices, glyphEntry.position, fillColor, fillGlyph, italicShear);

                    glyphEntry.vertexCount = m_vertices.getVertexCount() - glyphEntry.vertexOffset;
                }
                else
                {
                    // Remove unwanted x-offset when dealing with vertical spaces
                    if ((shapeGlyph.direction == HB_DIRECTION_TTB) || (shapeGlyph.direction == HB_DIRECTION_BTT))
                        glyphEntry.position.x -= shapeGlyph.offset.x;

                    // Check if we are dealing with the start of a tab
                    if (std::find(currentLineTabIndices.begin(), currentLineTabIndices.end(), shapeGlyph.cluster) !=
                        currentLineTabIndices.end())
                    {
                        // Widen the size to 4 times the advance of the space glyph
                        glyphEntry.glyph.advance *= 4.0f;

                        if (isHorizontal)
                        {
                            glyphEntry.glyph.bounds.size.x = glyphEntry.glyph.advance;
                        }
                        else if (isVertical)
                        {
                            glyphEntry.glyph.bounds.size.y = glyphEntry.glyph.advance;
                        }

                        // Skip the next 3 space glyphs
                        glyphsToSkip = 3;
                    }
                    else
                    {
                        // If the glyph doesn't have a texture it is probably a space
                        // Set the bounds width to the width of the space so the user can
                        // make use of it
                        if (isHorizontal)
                        {
                            glyphEntry.glyph.bounds.size = {shapeGlyph.advance.x, 0.0f};
                        }
                        else if (isVertical)
                        {
                            glyphEntry.glyph.bounds.size = {0.0f, shapeGlyph.advance.y};
                        }
                    }

                    if (m_glyphPreProcessor)
                        m_glyphPreProcessor(glyphEntry, style, fillColor, outlineColor, outlineThickness);

                    italicShear = (style & Italic) ? m_italicShear.asRadians() : 0.0f;
                }

                // Update the current bounds
                const sf::Vector2f p1 = glyph.bounds.position + shapeGlyph.offset;
                const sf::Vector2f p2 = p1 + glyphEntry.glyph.bounds.size;

                lineMinX = std::min(lineMinX, x + p1.x - italicShear * p2.y);
                lineMaxX = std::max(lineMaxX, x + p2.x - italicShear * p1.y);
                lineMinY = std::min(lineMinY, y + p1.y);
                lineMaxY = std::max(lineMaxY, y + p2.y);

                // Advance to the next character
                // Only apply additional letter spacing if the current glyph has an advance (base glyph)
                // and a visible texture area, so whitespace advances stay untouched
                // Applying the letter spacing to glyphs without an advance would affect
                // mark glyphs as well which would lead to incorrect results
                const auto hasAdvance = glyphEntry.glyph.advance > 0.0f;

                if (isHorizontal)
                {
                    x += glyphEntry.glyph.advance + (hasAdvance ? letterSpacing : 0.0f);
                }
                else if (isVertical)
                {
                    // We only add data to the vertical underline list for base glyphs
                    if (isUnderlined && hasAdvance)
                        verticalUnderlineData.emplace_back(
                            VerticalUnderlineEntry{glyphEntry.position + sf::Vector2f(glyphEntry.glyph.bounds.position.x, 0.0f),
                                                   glyphEntry.glyph.bounds.size.x});

                    y += glyphEntry.glyph.advance + (hasAdvance ? letterSpacing : 0.0f);
                }
            }

            // Update the multi-line bounds
            minX = std::min(minX, lineMinX);
            maxX = std::max(maxX, lineMaxX);
            minY = std::min(minY, lineMinY);
            maxY = std::max(maxY, lineMaxY);

            currentLine.clear();
            currentLineIndices.clear();
            currentLineTabIndices.clear();

            // If we're using the underlined style and there's a new line, draw a line
            if (isUnderlined)
            {
                if (m_textOrientation == TextOrientation::Default)
                {
                    AddLineHorizontal(m_vertices, 0.0f, x, y, m_fillColor, underlineOffset, underlineThickness);

                    if (m_outlineThickness != 0)
                        AddLineHorizontal(m_outlineVertices, 0.0f, x, y, m_outlineColor, underlineOffset, underlineThickness, m_outlineThickness);
                }
                else
                {
                    // If we have to apply underline to vertical text we will have to do it per glyph
                    for (const auto& entry : verticalUnderlineData)
                    {
                        AddLineHorizontal(m_vertices,
                                          entry.position.x,
                                          entry.position.x + entry.width,
                                          entry.position.y,
                                          m_fillColor,
                                          underlineOffset,
                                          underlineThickness);

                        if (m_outlineThickness != 0)
                            AddLineHorizontal(m_outlineVertices,
                                              entry.position.x,
                                              entry.position.x + entry.width,
                                              entry.position.y,
                                              m_outlineColor,
                                              underlineOffset,
                                              underlineThickness,
                                              m_outlineThickness);
                    }
                }
            }

            // If we're using the strikethrough style and there's a new line, draw a line across all characters
            if (isStrikeThrough)
            {
                if (m_textOrientation == TextOrientation::Default)
                {
                    AddLineHorizontal(m_vertices, 0.0f, x, y, m_fillColor, strikeThroughOffset, underlineThickness);

                    if (m_outlineThickness != 0)
                        AddLineHorizontal(m_outlineVertices, 0.0f, x, y, m_outlineColor, strikeThroughOffset, underlineThickness, m_outlineThickness);
                }
                else
                {
                    // Slanting the text by 12 degrees means the strikethrough would
                    // have to be offset by
                    // width * sin(12 degrees) / 2
                    // which is approximately
                    // width * 0.1
                    AddLineVertical(m_vertices,
                                    lineMinY,
                                    lineMaxY,
                                    m_fillColor,
                                    (m_style & Italic) ? (lineMaxX - lineMinX) * 0.1f : 0.0f,
                                    underlineThickness);

                    if (m_outlineThickness != 0)
                    {
                        AddLineVertical(m_outlineVertices,
                                        lineMinY,
                                        lineMaxY,
                                        m_outlineColor,
                                        (m_style & Italic) ? (lineMaxX - lineMinX) * 0.1f : 0.0f,
                                        underlineThickness,
                                        m_outlineThickness);
                    }
                }
            }
        };

        // Split the input string into multiple segments with uniform
        // script and direction using the unicode bidirectional algorithm
        const auto segments = SegmentString(string);

        // In order to be able to align text we have to record all line data until we can compute the text metrics
        // We then use the record data to shift the necessary lines to the right/left as necessary
        struct LineRecord
        {
            std::size_t    glyphsStart{};
            std::size_t    glyphsCount{};
            std::size_t    verticesStart{};
            std::size_t    verticesCount{};
            std::size_t    outlineVerticesStart{};
            std::size_t    outlineVerticesCount{};
            std::size_t    firstCodepointOffset = std::numeric_limits<std::size_t>::max();
            hb_direction_t direction{};
            float          lineWidth{};
        };

        std::vector<LineRecord> lines;

        const auto beginLineRecord = [&]
        {
            // Start a new line record
            auto& lineRecord                = lines.emplace_back();
            lineRecord.glyphsStart          = m_glyphs.size();
            lineRecord.verticesStart        = m_vertices.getVertexCount();
            lineRecord.outlineVerticesStart = m_outlineVertices.getVertexCount();
        };

        const auto endLineRecord = [&]
        {
            // Complete the line record
            auto& lineRecord                = lines.back();
            lineRecord.glyphsCount          = m_glyphs.size() - lineRecord.glyphsStart;
            lineRecord.verticesCount        = m_vertices.getVertexCount() - lineRecord.verticesStart;
            lineRecord.outlineVerticesCount = m_outlineVertices.getVertexCount() - lineRecord.outlineVerticesStart;
            lineRecord.lineWidth            = x;
        };

        if (!segments.empty())
            beginLineRecord();

        // Iterate over all segments
        for (const auto& segment : segments)
        {
            currentScript    = segment.script;
            currentDirection = segment.direction;

            if (segment.offset < lines.back().firstCodepointOffset)
            {
                lines.back().firstCodepointOffset = segment.offset;
                lines.back().direction            = currentDirection;
            }

            // We use the index into the input string as the input cluster IDs as well
            // This way the user will be able to map the resulting cluster IDs back to
            // characters in the input text they provided for advanced functionality
            for (auto index = static_cast<std::uint32_t>(segment.offset);
                 index < static_cast<std::uint32_t>(segment.offset + segment.length);
                 ++index)
            {
                const auto& curChar = string[index];

                // Handle special characters
                if ((curChar == U'\n'))
                {
                    if (!currentLine.isEmpty())
                        outputLine();

                    // Add new entry to glyphs
                    // For our purposes, we consider the newline
                    // character to constitute its own cluster
                    auto& glyph = m_glyphs.emplace_back(
                        ShapedGlyph{m_font->GetGlyph('\n', m_characterWidth, m_characterHeight, isBold), {}, {}, {}});
                    glyph.glyph.bounds.size = {0.0f, 0.0f};
                    glyph.baseline          = y;

                    // We give some effort to position the newline glyph "after"
                    // its preceeding glyph taking into account the text direction
                    if (m_glyphs.size() == 1)
                    {
                        // Nothing but a newline so far, nothing we can do
                        glyph.position = {x, y};
                    }
                    else
                    {
                        // Search for the cluster with the highest cluster value, see comment below on why this works
                        // We are guaranteed to find an element, so the iterator returned is guaranteed to be valid
                        const auto& highestClusterGlyph = *std::max_element(m_glyphs.begin() +
                                                                                static_cast<int>(lines.back().glyphsStart),
                                                                            m_glyphs.end(),
                                                                            [](const ShapedGlyph& left, const ShapedGlyph& right)
                                                                            { return left.cluster < right.cluster; });

                        glyph.position = {highestClusterGlyph.position.x +
                                              (highestClusterGlyph.textDirection == TextDirection::RightToLeft
                                                   ? 0.0f
                                                   : highestClusterGlyph.glyph.advance),
                                          y};
                    }

                    // Only set the cluster here so we can search for the glyph
                    // with the highest cluster before this one was inserted
                    // ShapedGlyph clusters get initialized to 0 and if there
                    // are multiple elements in m_glyphs this means they will
                    // be returned instead of this glyph
                    glyph.cluster = index;

                    endLineRecord();
                    beginLineRecord();

                    // Update the current bounds (min coordinates)
                    minX = std::min(minX, x);
                    minY = std::min(minY, y);

                    y += lineSpacing;
                    x = 0;

                    // Update the current bounds (max coordinates)
                    maxX = std::max(maxX, x);
                    maxY = std::max(maxY, y);

                    // Next glyph, no need to create a quad for newline
                    continue;
                }

                if (curChar == U'\t')
                {
                    // Replace tab character with 4 spaces for shaping
                    currentLine += "    ";
                    currentLineIndices.resize(currentLineIndices.size() + 4u, index);
                    currentLineTabIndices.emplace_back(index);
                    continue;
                }

                if ((curChar < 0x80) && std::iscntrl(static_cast<int>(curChar)))
                {
                    // Skip all other control characters to avoid weird graphical issues
                    continue;
                }

                currentLine += curChar;
                currentLineIndices.emplace_back(index);
            }

            if (!currentLine.isEmpty())
                outputLine();
        }

        if (!segments.empty())
            endLineRecord();

        // Sort shaped glyphs so that clusters are in ascending order
        std::sort(m_glyphs.begin(),
                  m_glyphs.end(),
                  [](const ShapedGlyph& left, const ShapedGlyph& right) { return left.cluster < right.cluster; });

        // If we're using outline, update the current bounds
        if (m_outlineThickness != 0)
        {
            const float outline = std::abs(std::ceil(m_outlineThickness));
            minX -= outline;
            maxX += outline;
            minY -= outline;
            maxY += outline;
        }

        // Update the bounding rectangle
        m_bounds.position = sf::Vector2f(minX, minY);
        m_bounds.size     = sf::Vector2f(maxX, maxY) - sf::Vector2f(minX, minY);

        // Use line record data to post-process lines e.g. re-alignment etc.
        if (!lines.empty())
        {
            // Get width of widest line
            const auto maxWidth = std::max_element(lines.begin(),
                                                   lines.end(),
                                                   [](const LineRecord& left, const LineRecord& right)
                                                   { return left.lineWidth < right.lineWidth; })
                                      ->lineWidth;

            for (auto& line : lines)
            {
                auto shift = 0.0f;

                if (m_lineAlignment == LineAlignment::Center)
                {
                    shift = line.lineWidth / -2.0f;
                }
                else if (m_lineAlignment == LineAlignment::Right)
                {
                    shift = -line.lineWidth;
                }
                else if ((m_lineAlignment == LineAlignment::Default) && (line.direction == HB_DIRECTION_RTL))
                {
                    shift = maxWidth - line.lineWidth;
                }
                else
                {
                    // Skip modifying the data if there is nothing to shift
                    continue;
                }

                // Keep the shift aligned to the pixel grid to avoid blurry glyphs
                shift = std::floor(shift);

                // Shift glyphs
                for (auto i = line.glyphsStart; i < line.glyphsStart + line.glyphsCount; ++i)
                    m_glyphs[i].position.x += shift;

                // Shift vertices
                for (auto i = line.verticesStart; i < line.verticesStart + line.verticesCount; ++i)
                    m_vertices[i].position.x += shift;

                // Shift vertices
                for (auto i = line.outlineVerticesStart; i < line.outlineVerticesStart + line.outlineVerticesCount; ++i)
                    m_outlineVertices[i].position.x += shift;
            }

            // Update bounds if necessary
            if (m_lineAlignment == LineAlignment::Center)
            {
                m_bounds.position.x -= m_bounds.size.x / 2.0f;
            }
            else if (m_lineAlignment == LineAlignment::Right)
            {
                m_bounds.position.x -= m_bounds.size.x;
            }
        }

        OnGeometryUpdated();
    }
}