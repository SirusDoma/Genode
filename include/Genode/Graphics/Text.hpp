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

#pragma once

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <Genode/SceneGraph/Node.hpp>
#include <Genode/SceneGraph/RenderableContainer.hpp>
#include <Genode/SceneGraph/UpdatableContainer.hpp>
#include <Genode/SceneGraph/InputableContainer.hpp>
#include <Genode/Entities/Colorable.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include <functional>
#include <memory>

#include <cstddef>
#include <cstdint>

namespace Gx
{
    class Font;
    typedef std::shared_ptr<const Font> FontHandle;

    ////////////////////////////////////////////////////////////
    /// @brief Graphical text that can be drawn to a render target
    ///
    ////////////////////////////////////////////////////////////
    class Text : public virtual Node,
                 public virtual RenderableContainer,
                 public virtual UpdatableContainer,
                 public virtual InputableContainer,
                 public virtual Colorable
    {
    public:
        ////////////////////////////////////////////////////////////
        /// @brief Enumeration of the string drawing styles
        ///
        ////////////////////////////////////////////////////////////
        enum Style
        {
            Regular       = 0,      //!< Regular characters, no style
            Bold          = 1 << 0, //!< Bold characters
            Italic        = 1 << 1, //!< Italic characters
            Underlined    = 1 << 2, //!< Underlined characters
            StrikeThrough = 1 << 3  //!< Strike through characters
        };

        ////////////////////////////////////////////////////////////
        /// @brief Enumeration of the text alignment options
        ///
        ////////////////////////////////////////////////////////////
        enum class LineAlignment
        {
            Default, //!< Automatically align lines by script direction, left-align left-to-right text and right-align right-to-left text
            Left,    //!< Force align all lines to the left, regardless of script direction
            Center,  //!< Force align all lines centrally
            Right    //!< Force align lines to the right, regardless of script direction
        };

        ////////////////////////////////////////////////////////////
        /// @brief Cluster Grouping
        ///
        ////////////////////////////////////////////////////////////
        enum class ClusterGrouping
        {
            Grapheme,  //!< Group clusters by grapheme
            Character, //!< Group clusters by character
            None       //!< Do not group clusters
        };

        ////////////////////////////////////////////////////////////
        /// @brief Text Direction
        ///
        ////////////////////////////////////////////////////////////
        enum class TextDirection
        {
            Unspecified, //!< Unspecified
            LeftToRight, //!< Left-to-right
            RightToLeft, //!< Right-to-left
            TopToBottom, //!< Top-to-bottom
            BottomToTop  //!< Bottom-to-top
        };

        ////////////////////////////////////////////////////////////
        /// @brief Text Orientation
        ///
        ////////////////////////////////////////////////////////////
        enum class TextOrientation
        {
            Default,     //!< Default (left-to-right or right-to-left depending on detected script)
            TopToBottom, //!< Top-to-bottom
            BottomToTop  //!< Bottom-to-top
        };

        ////////////////////////////////////////////////////////////
        /// @brief Structure describing a glyph after shaping
        ///
        ////////////////////////////////////////////////////////////
        struct ShapedGlyph
        {
            sf::Glyph glyph;

            sf::Vector2f  position;        //!< Position of the glyph within a text
            std::uint32_t cluster{};       //!< Cluster ID
            TextDirection textDirection{}; //!< Text direction
            float         baseline{};      //!< The baseline position of the line this glyph is a part of
            std::size_t   vertexOffset{};  //!< Starting offset of the vertex data belonging to this glyph
            std::size_t   vertexCount{};   //!< Count of vertices belonging to this glyph
        };

        ////////////////////////////////////////////////////////////
        /// @brief Default constructor
        ///
        ////////////////////////////////////////////////////////////
        Text() = default;

        ////////////////////////////////////////////////////////////
        /// @brief Construct the text from a string, font and size
        ///
        /// Note that if the used font is a bitmap font, it is not
        /// scalable, thus not all requested sizes will be available
        /// to use. This needs to be taken into consideration when
        /// setting the character size. If you need to display text
        /// of a certain size, make sure the corresponding bitmap
        /// font that supports that size is used.
        ///
        /// @param string         Text assigned to the string
        /// @param font           Font used to draw the string
        /// @param characterSize  Base size of characters, in pixels
        ///
        ////////////////////////////////////////////////////////////
        explicit Text(const Font& font, sf::String string = "", unsigned int characterSize = 30);

        ////////////////////////////////////////////////////////////
        /// @brief Disallow construction from a temporary font
        ///
        ////////////////////////////////////////////////////////////
        explicit Text(const Font&& font, const sf::String& string = "", unsigned int characterSize = 30) = delete;

        ////////////////////////////////////////////////////////////
        /// @brief Set the text's string
        ///
        /// The @a `string` argument is a `sf::String`, which can
        /// automatically be constructed from standard string types.
        /// So, the following calls are all valid:
        /// @code
        /// text.SetString("hello");
        /// text.SetString(L"hello");
        /// text.SetString(std::string("hello"));
        /// text.SetString(std::wstring(L"hello"));
        /// @endcode
        /// A text's string is empty by default.
        ///
        /// @param string New string
        ///
        /// @see `GetString`
        ///
        ////////////////////////////////////////////////////////////
        void SetString(const sf::String& string);

        ////////////////////////////////////////////////////////////
        /// @brief Set the text's font
        ///
        /// The @a `font` argument refers to a font that must
        /// exist as long as the text uses it. Indeed, the text
        /// doesn't store its own copy of the font, but rather keeps
        /// a pointer to the one that you passed to this function.
        /// If the font is destroyed and the text tries to
        /// use it, the behavior is undefined.
        ///
        /// @param font New font
        ///
        /// @see `GetFont`
        ///
        ////////////////////////////////////////////////////////////
        void SetFont(const Font& font);

        ////////////////////////////////////////////////////////////
        /// @brief Disallow setting from a temporary font
        ///
        ////////////////////////////////////////////////////////////
        void SetFont(const Font&& font) = delete;

        ////////////////////////////////////////////////////////////
        /// @brief Specify whether the string should be displayed as masked string.
        ///
        /// A text's string is empty by default.
        ///
        /// @param masked A value determines whether the string should be masked.
        ///
        /// @see `IsMasked`
        ///
        ////////////////////////////////////////////////////////////
        void SetMasked(bool masked);

        ////////////////////////////////////////////////////////////
        /// @brief Set the character used to mask the string
        ///
        /// The default masking character is U+25CF (●), commonly
        /// used to mask password fields.
        ///
        /// @param character New masking character
        ///
        /// @see `GetMaskingCharacter`, `SetMasked`
        ///
        ////////////////////////////////////////////////////////////
        void SetMaskingCharacter(char32_t character);

        ////////////////////////////////////////////////////////////
        /// @brief Set the character size
        ///
        /// The default size is 30.
        /// Setting a new size with this method will set character width to 0.
        /// See @a `SetCharacterWidth` for more details.
        ///
        /// Note that if the used font is a bitmap font, it is not
        /// scalable, thus not all requested sizes will be available
        /// to use. This needs to be taken into consideration when
        /// setting the character size. If you need to display text
        /// of a certain size, make sure the corresponding bitmap
        /// font that supports that size is used.
        ///
        /// @param size New character size, in pixels
        ///
        /// @see `GetCharacterSize`
        ///
        ////////////////////////////////////////////////////////////
        void SetCharacterSize(unsigned int size);

        ////////////////////////////////////////////////////////////
        /// @brief Set the character width
        ///
        /// The default size is 0, which mean it automatically computed from the character height.
        ///
        /// @param width New character width, in pixels.
        ///              Set it to 0 if you want the font to figure out the width based on the character height.
        ///
        /// @see `GetCharacterWidth`
        ///
        ////////////////////////////////////////////////////////////
        void SetCharacterWidth(unsigned int width);

        ////////////////////////////////////////////////////////////
        /// @brief Set the character height.
        ///
        /// The default size is 30.
        ///
        /// Note that if the used font is a bitmap font, it is not
        /// scalable, thus not all requested sizes will be available
        /// to use. This needs to be taken into consideration when
        /// setting the character size. If you need to display text
        /// of a certain size, make sure the corresponding bitmap
        /// font that supports that size is used.
        ///
        /// @param height New character height, in pixels
        ///
        /// @see `GetCharacterHeight`
        ///
        ////////////////////////////////////////////////////////////
        void SetCharacterHeight(unsigned int height);

        ////////////////////////////////////////////////////////////
        /// @brief Set the line spacing
        ///
        /// The default spacing between lines is defined by the font.
        /// This method enables you to set a factor for the spacing
        /// between lines. By default, the line spacing is 1.
        ///
        /// @param spacing New line spacing in pixels
        ///
        /// @see `GetLineSpacing`
        ///
        ////////////////////////////////////////////////////////////
        void SetLineSpacing(float spacing);

        ////////////////////////////////////////////////////////////
        /// @brief Set the letter spacing
        ///
        /// The default spacing between letters is defined by the font.
        /// This spacing doesn't directly apply to the existing
        /// spacing between each character, it rather adds a fixed
        /// space between them which is calculated from the font
        /// metrics and the character size.
        /// Note that spacing below 1 (including negative numbers) bring
        /// characters closer to each other.
        /// By default, the letter spacing spacing is 1.
        ///
        /// @param spacing New letter spacing in pixels
        ///
        /// @see `GetLetterSpacing`
        ///
        ////////////////////////////////////////////////////////////
        void SetLetterSpacing(float spacing);

        ////////////////////////////////////////////////////////////
        /// @brief Set the text's style
        ///
        /// You can pass a combination of one or more styles, for
        /// example `Gx::Text::Bold | Gx::Text::Italic`.
        /// The default style is `Gx::Text::Regular`.
        ///
        /// @param style New style
        ///
        /// @see `GetStyle`
        ////////////////////////////////////////////////////////////
        void SetStyle(std::uint32_t style);

        ////////////////////////////////////////////////////////////
        /// @brief Set the shear when rendering text in italic
        ///
        /// By default shear is 12.
        ///
        /// @param italicSheer New shear for italic style
        ///
        /// @see `GetItalicShear`
        ////////////////////////////////////////////////////////////
        void SetItalicShear(const sf::Angle& italicSheer);

        ///////////////////////////////////////////////////////////
        /// @brief Set the fill color of the text.
        ///
        /// By default, the text's fill color is opaque white.
        /// Setting the fill color to a transparent color with an outline
        /// will cause the outline to be displayed in the fill area of the text.
        ///
        /// @param color New fill color of the text
        ///
        /// @see `GetColor`
        ////////////////////////////////////////////////////////////
        void SetColor(const sf::Color& color) override;

        ////////////////////////////////////////////////////////////
        /// @brief Set the outline color of the text
        ///
        /// By default, the text's outline color is opaque black.
        ///
        /// @param color New outline color of the text
        ///
        /// @see `GetOutlineColor`
        ////////////////////////////////////////////////////////////
        void SetOutlineColor(const sf::Color& color);

        ////////////////////////////////////////////////////////////
        /// @brief Set the thickness of the text's outline
        ///
        /// By default, the outline thickness is 0.
        ///
        /// Be aware that using a negative value for the outline
        /// thickness will cause distorted rendering.
        ///
        /// @param thickness New outline thickness, in pixels
        ///
        /// @see `GetOutlineThickness`
        ////////////////////////////////////////////////////////////
        void SetOutlineThickness(float thickness);

        ////////////////////////////////////////////////////////////
        /// @brief Set the offset of the text's outline
        ///
        /// By default, the outline offset is 0.
        ///
        /// Be aware that using a negative value for the outline
        /// offset will cause distorted rendering.
        ///
        /// @param offset New outline offset, in pixels
        ///
        /// @see `GetOutlineOffset`
        ////////////////////////////////////////////////////////////
        void SetOutlineOffset(const sf::Vector2f& offset);

        ////////////////////////////////////////////////////////////
        /// @brief Set the line alignment for a multi-line text
        ///
        /// By default, the lines will be aligned according to the
        /// direction of the line's script. Left-to-right scripts
        /// will be aligned to the left and right-to-left scripts
        /// will be aligned to the right.
        ///
        /// Forcing alignment will ignore script direction and always
        /// align according to the requested line alignment.
        ///
        /// @param lineAlignment New line alignment
        ///
        /// @see `GetLineAlignment`
        ///
        ////////////////////////////////////////////////////////////
        void SetLineAlignment(LineAlignment lineAlignment);

        ////////////////////////////////////////////////////////////
        /// @brief Set the text orientation
        ///
        /// By default, the lines will have horizontal orientation.
        ///
        /// Be aware that most fonts don't natively support vertical
        /// orientations. Fonts that are the most likely to natively
        /// support vertical orientations are those whose scripts
        /// also support vertical orientations e.g. east asian scripts.
        ///
        /// If a font does not natively support vertical orientation,
        /// vertical metrics might still be provided for shaping.
        /// In this case, they are very likely to be emulated and might
        /// not result in good visual output.
        ///
        /// Some metrics such as advance and baseline position will
        /// be rotated so they match the vertical axis.
        ///
        /// @param textOrientation New text orientation
        ///
        /// @see `GetTextOrientation`
        ///
        ////////////////////////////////////////////////////////////
        void SetTextOrientation(TextOrientation textOrientation);

        ////////////////////////////////////////////////////////////
        /// @brief Get the text's string
        ///
        /// The returned string is a `sf::String`, which can automatically
        /// be converted to standard string types. So, the following
        /// lines of code are all valid:
        /// @code
        /// sf::String   s1 = text.getString();
        /// std::string  s2 = text.getString();
        /// std::wstring s3 = text.getString();
        /// @endcode
        ///
        /// @return Text's string
        ///
        /// @see `SetString`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] const sf::String& GetString() const;
        
        ////////////////////////////////////////////////////////////
        /// @brief Get the text's font
        ///
        /// The returned reference is const, which means that you
        /// cannot modify the font when you get it from this function.
        ///
        /// @return Reference to the text's font
        ///
        /// @see `SetFont`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] const Font* GetFont() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get a value determine whether the text is displayed as masked string
        ///
        /// @return `true` if rendered as masked string, otherwise `false`.
        ///
        /// @see `SetMasked`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] bool IsMasked() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the character used to mask the string
        ///
        /// @return Masking character
        ///
        /// @see `SetMaskingCharacter`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] char32_t GetMaskingCharacter() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the character size
        ///
        /// @return Size of the characters, in pixels
        ///
        /// @see `SetCharacterSize`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] unsigned int GetCharacterSize() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the character width
        ///
        /// @return Width of the characters, in pixels
        ///
        /// @see `SetCharacterWidth`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] unsigned int GetCharacterWidth() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the character height
        ///
        /// @return Height of the characters, in pixels
        ///
        /// @see `SetCharacterHeight`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] unsigned int GetCharacterHeight() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the size of the letter spacing
        ///
        /// @return Size of the letter spacing in pixels
        ///
        /// @see `SetLetterSpacing`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] float GetLetterSpacing() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the size of the line spacing
        ///
        /// @return Size of the line spacing in pixels
        ///
        /// @see `SetLineSpacing`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] float GetLineSpacing() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the text's style
        ///
        /// @return Text's style
        ///
        /// @see `setStyle`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] std::uint32_t GetStyle() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the shear when rendering text in italic
        ///
        /// @return Shear for italic style
        ///
        /// @see `SetItalicShear`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] sf::Angle GetItalicShear() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the fill color of the text
        ///
        /// @return Fill color of the text
        ///
        /// @see `SetColor`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] const sf::Color& GetColor() const override;

        ////////////////////////////////////////////////////////////
        /// @brief Get the outline color of the text
        ///
        /// @return Outline color of the text
        ///
        /// @see `SetOutlineColor`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] const sf::Color& GetOutlineColor() const;
        
        ////////////////////////////////////////////////////////////
        /// @brief Get the outline thickness of the text
        ///
        /// @return Outline thickness of the text, in pixels
        ///
        /// @see `SetOutlineThickness`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] float GetOutlineThickness() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the outline offset of the text
        ///
        /// @return Outline offset of the text, in pixels
        ///
        /// @see `SetOutlineOffset`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] sf::Vector2f GetOutlineOffset() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the line alignment for a multi-line text
        ///
        /// @return Line alignment
        ///
        /// @see `SetLineAlignment`
        ///
        ////////////////////////////////////////////////////////////
        LineAlignment GetLineAlignment() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the text orientation
        ///
        /// @return Text orientation
        ///
        /// @see `SetTextOrientation`
        ///
        ////////////////////////////////////////////////////////////
        TextOrientation GetTextOrientation() const;

        ////////////////////////////////////////////////////////////
        /// @brief Return the position of the @a `index`-th character
        ///
        /// This function computes the visual position of a character
        /// from its index in the string. The returned position is
        /// in global coordinates (translation, rotation, scale and
        /// origin are applied).
        /// If @a `index` is out of range, the position of the end of
        /// the string is returned.
        ///
        /// @param index Index of the character
        ///
        /// @return Position of the character
        ////////////////////////////////////////////////////////////
        [[deprecated("Use GetShapedGlyphs() instead")]] [[nodiscard]] sf::Vector2f FindCharacterPosition(std::size_t index) const;

        ////////////////////////////////////////////////////////////
        /// @brief Return a list of shaped glyphs that make up the text
        ///
        /// The result of shaping i.e. positioning individual glyphs
        /// based on the properties of the font and the input text
        /// is a sequence of shaped glyphs that each have a collection
        /// of properties.
        ///
        /// In addition to the glyph information that is available
        /// by looking up a glyph from a font, the glyph position,
        /// glyph cluster ID and direction of the text represented
        /// by the glyph is provided.
        ///
        /// When specifying unicode text, multiple unicode codepoints
        /// might combine to form e.g. a ligature such as æ or
        /// base-and-mark sequence such as é which are composed of
        /// multiple individual glyphs. These combinations are known
        /// as grapheme clusters. When segmenting text into grapheme
        /// clusters, each cluster identifies a complete unit of text
        /// that will be drawn. There are other methods of segmenting
        /// text into clusters e.g. without combining marks.
        /// Character cluster segmentation is used as the default.
        /// A single grapheme can be represented by an individual
        /// codepoint or by a composition of codepoints e.g. an e as
        /// the base and an accent as the mark which together compose
        /// the grapheme é. The cluster groups that result from shaping
        /// depend on whether the input text provides composed
        /// codepoints or decomposed codepoints. This is an advanced
        /// topic known as unicode normalisation.
        ///
        /// When positioning e.g. a cursor within the text, grapheme
        /// clusters can be treated as the basic units of which the
        /// text is composed and not subdivided into their individual
        /// components or glyphs. If positioning of the cursor within
        /// a single grapheme e.g. a ligature is required, a more
        /// fine-grained cluster segmentation algorithm should be used.
        ///
        /// The returned glyph positions are in local coordinates
        /// (translation, rotation, scale and origin are not applied).
        ///
        /// @return List of shaped glyphs that make up the text
        ///
        /// @see `SetClusterGrouping`
        ///
        ////////////////////////////////////////////////////////////
        [[nodiscard]] const std::vector<ShapedGlyph>& GetShapedGlyphs() const;

        ////////////////////////////////////////////////////////////
        /// @brief Return the cluster grouping algorithm in use
        ///
        /// @return The cluster grouping algorithm in use
        ///
        ////////////////////////////////////////////////////////////
        [[nodiscard]] ClusterGrouping GetClusterGrouping() const;

        ////////////////////////////////////////////////////////////
        /// @brief Set the cluster grouping algorithm to use
        ///
        /// By default, character cluster grouping is used.
        ///
        /// Character cluster grouping is good enough to be able to
        /// position cursors in most scenarios. If more coarse-grained
        /// grouping is required, grapheme grouping can be selected.
        ///
        /// Cluster grouping can also be disabled if necessary.
        ///
        /// @param clusterGrouping The cluster grouping algorithm to use
        ///
        ////////////////////////////////////////////////////////////
        void SetClusterGrouping(ClusterGrouping clusterGrouping);

        ////////////////////////////////////////////////////////////
        /// @brief Callable that is provided with glyph data for pre-processing
        ///
        /// When re-generating the text geometry, shaping will be
        /// performed on the input string using the set font. The
        /// result of shaping is a set of shaped glyphs. Shaped
        /// glyphs are glyphs that have been positioned by the shaper
        /// and whose script direction has also been determined.
        ///
        /// Because multiple input codepoints can be merged into a
        /// single glyph and single codepoints decomposed into multiple
        /// glyphs, the shaper provides a way to map the shaping output
        /// back to the input. When the input string is provided to
        /// the shaper, a monotonically increasing character index is
        /// attached to each input codepoint. If the input string
        /// consists of 10 codepoints, the indices will be 0 to 9.
        ///
        /// After shaping each shaped glyph will be assigned a
        /// cluster value. These cluster values are derived from the
        /// input indices that were provided to the shaper. Because
        /// of the merging and decomposing that happens during shaping,
        /// there isn't a 1 to 1 mapping between input indices and
        /// output cluster values.
        ///
        /// In order to set the glyph properties reliably, they have
        /// to be set based on text segmentation boundaries such as
        /// graphemes, words and sentences. See the corresponding
        /// methods in `sf::String` that can check for these boundaries.
        ///
        /// Once the input text segments to be pre-processed have
        /// been determined, they have to be applied to the shaped
        /// glyphs. When using character or grapheme cluster grouping
        /// it is guaranteed that the resulting cluster values are
        /// monotonic. This means that cluster values will not be
        /// reordered beyond the bounds of the indices that were
        /// provided with the input text.
        ///
        /// What this means is that given a segment of text that
        /// should e.g. be colored differently, if a beginning and
        /// end index can be determined from the input codepoints,
        /// these index boundaries can be used to select the clusters
        /// of the shaped glyphs that correspond to the input segment
        /// and thus whose color needs to be set.
        ///
        /// Here is an example string with codepoint indices:
        /// @code
        /// I   l i k e   f l o w e r s ,   m u f f i n s   a n d   w a f f l e s .
        /// 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3
        /// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
        /// @endcode
        /// After shaping, due to ligature merging of fl, ffi and ffl,
        /// the output clusters might look like:
        /// @code
        /// I   l i k e   fl o w e r s ,   m u ffi n s   a n d   w a ffl e s .
        /// 0 0 0 0 0 0 0 0  0 1 1 1 1 1 1 1 1 1   2 2 2 2 2 2 2 2 2 3   3 3 3
        /// 0 1 2 3 4 5 6 7  9 0 1 2 3 4 5 6 7 8   1 2 3 4 5 6 7 8 9 0   3 4 5
        /// @endcode
        ///
        /// In order to e.g. color the word "muffins", the beginning
        /// and end codepoint indices of the word have to be determined,
        /// in this case 16 and 22. After shaping, any glyphs belonging
        /// to the word "muffins" will have cluster values between and
        /// including 16 and 22. In the example above the clusters
        /// 16, 17, 18, 21 and 22 belong to the word "muffins".
        /// Coloring the glyphs with those indices will result in the
        /// word "muffins" being colored.
        ///
        /// The same applies to "flowers" and "waffles" in the example
        /// above.
        ///
        /// Because merging and decomposition of codepoints cannot
        /// happen beyond word boundaries, applying properties to
        /// glyphs using the above method is safe when segmenting
        /// based on words. As can be seen above it would not work
        /// when attempting to apply a different property to the
        /// single graphemes 'f', 'l' or 'i' since they can be
        /// merged with neighbouring graphemes into a single glyph.
        ///
        /// The opposite, decomposition, of the following input:
        /// @code
        /// I   f i n d   c l i c h é s   f u n n y .
        /// 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1
        /// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
        /// @endcode
        /// into glyphs would look like:
        /// @code
        /// I   f i n d   c l i c h e + s   f u n n y .
        /// 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1
        /// 0 1 2 3 4 5 6 7 8 9 0 1 2 2 3 4 5 6 7 8 9 0
        /// @endcode
        /// The + at cluster 12 is a placeholder for the acute accent.
        /// This can occur if the font provides the glyph for the accent
        /// seperate from the base glyph e which also has the cluster
        /// value 12. The codepoint é is thus decomposed into a base glyph
        /// and a mark glyph. The cluster value of the mark in such a
        /// decomposition will be identical to the base. Because of this,
        /// the same procedure as demonstrated in the fist example can be
        /// applied here as well.
        ///
        /// The above examples are just simple examples of how to map
        /// input codepoint indices to output cluster values. While
        /// merging and decomposition are an exception in latin script,
        /// they can occur very frequently in other scripts. The mapping
        /// procedure described above will work for all scripts.
        ///
        /// Once the boundaries of the cluster values whose properties to
        /// modify have been determined, they can be used from within
        /// the callable to set said properties on a glyph by glyph basis.
        ///
        /// The callable will be called in the order in which glyph
        /// geometry is generated. This does not always happen in
        /// ascending cluster order such as in right-to-left text where
        /// it happens in descending cluster order.
        ///
        /// Be aware that while changing the character size per glyph
        /// is not possible, changing its style or outline thickness
        /// is. Doing this, however, might lead to slight inconsistencies
        /// when the text bounds are computed at the end of the geometry
        /// update process. The same applies to the italic style.
        ///
        /// In contrast, changing the fill or outline color is safe
        /// since they don't have any effect on the pixel coverage of
        /// the glyph.
        ///
        /// Setting the underlined and strikethrough styles per glyph
        /// is technically possible but not yet implemented.
        ///
        /// Note: Because text bounds are computed based on the
        /// geometry, it is not safe or reliable to query the text bounds
        /// from within this callable. If it is absolutely necessary
        /// to make decisions within this callable based on text bounds,
        /// multiple geometry updates will be necessary. The first
        /// geometry update is run with the pre-processor set to
        /// pass through data. Based on the first update the text bounds
        /// can be queried and stored. The stored text bounds can then
        /// be used in the second geometry update.
        ///
        ////////////////////////////////////////////////////////////
        using GlyphPreProcessor = std::function<
            void(const ShapedGlyph& shapedGlyph, std::uint32_t& style, sf::Color& fillColor, sf::Color& outlineColor, float& outlineThickness)>;

        ////////////////////////////////////////////////////////////
        /// @brief Set the glyph pre-processor to be called per glyph
        ///
        /// The glyph pre-processor is a callable that will be called
        /// with glyph data to be pre-processed.
        ///
        /// @param glyphPreProcessor The glyph pre-processor to be called per glyph, pass an empty pre-processor to disable pre-processing
        ///
        ////////////////////////////////////////////////////////////
        void SetGlyphPreProcessor(GlyphPreProcessor glyphPreProcessor);

        ////////////////////////////////////////////////////////////
        /// @brief Get a reference to the vertex data of this text
        ///
        /// The vertex data is regenerated by the text whenever it is
        /// necessary. Any changes made to the vertex data will be
        /// discarded whenever this happens.
        ///
        /// @return Reference to the vertex data of this text
        ///
        ////////////////////////////////////////////////////////////
        sf::VertexArray& GetVertexData() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get a reference to the outline vertex data of this text
        ///
        /// The outline vertex data is regenerated by the text whenever
        /// it is necessary. Any changes made to the outline vertex data
        /// will be discarded whenever this happens.
        ///
        /// @return Reference to the vertex data of this text
        ///
        ////////////////////////////////////////////////////////////
        sf::VertexArray& GetOutlineVertexData() const;

        ////////////////////////////////////////////////////////////
        /// @brief Truncate text to a specified maximum width
        ///
        ////////////////////////////////////////////////////////////
        void Truncate(std::size_t maxWidth);

        ////////////////////////////////////////////////////////////
        /// @brief Get the local bounding rectangle of the entity
        ///
        /// The returned rectangle is in local coordinates, which means
        /// that it ignores the transformations (translation, rotation,
        /// scale, ...) that are applied to the entity.
        /// In other words, this function returns the bounds of the
        /// entity in the entity's coordinate system.
        ///
        /// @return Local bounding rectangle of the entity
        ////////////////////////////////////////////////////////////
        [[nodiscard]] virtual sf::FloatRect GetLocalBounds() const;

        ////////////////////////////////////////////////////////////
        /// @brief Get the global bounding rectangle of the entity
        ///
        /// The returned rectangle is in global coordinates, which means
        /// that it takes into account the transformations (translation,
        /// rotation, scale, ...) that are applied to the entity.
        /// In other words, this function returns the bounds of the
        /// text in the global 2D world's coordinate system.
        ///
        /// @return Global bounding rectangle of the entity
        ////////////////////////////////////////////////////////////
        [[nodiscard]] sf::FloatRect GetGlobalBounds() const;

    protected:
        ////////////////////////////////////////////////////////////
        /// @brief Make sure the text's geometry is updated
        ///
        /// All the attributes related to rendering are cached, such
        /// that the geometry is only updated when necessary.
        ///
        ////////////////////////////////////////////////////////////
        void EnsureGeometryUpdate() const;

        ////////////////////////////////////////////////////////////
        /// @brief An overridable callback that called when the font is changed
        ///
        ////////////////////////////////////////////////////////////
        virtual void OnFontChanged(const Gx::Font&) const {};

        ////////////////////////////////////////////////////////////
        /// @brief An overridable callback that called when the geometry is updating
        ///
        ////////////////////////////////////////////////////////////
        virtual void OnGeometryUpdating() const {};

        ////////////////////////////////////////////////////////////
        /// @brief An overridable callback that called when the geometry is updated
        ///
        ////////////////////////////////////////////////////////////
        virtual void OnGeometryUpdated() const {};

        ////////////////////////////////////////////////////////////
        /// @brief Render the text to a render surface
        ///
        /// @param surface Render surface to draw to
        /// @param states Current render states
        ////////////////////////////////////////////////////////////
        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

    private:
        struct ShaperImpl;

        ////////////////////////////////////////////////////////////
        // Member data
        ////////////////////////////////////////////////////////////
        sf::String                          m_string;                                        //!< String to display
        const Font*                         m_font{};                                        //!< Font used to display the string
        unsigned int                        m_characterHeight{30};                           //!< Base height of characters, in pixels
        unsigned int                        m_characterWidth{0};                             //!< Base width of characters, in pixels
        float                               m_letterSpacing{0.f};                            //!< Spacing between letters
        float                               m_lineSpacing{1.f};                              //!< Spacing between lines
        sf::Angle                           m_italicShear{sf::degrees(12)};             //!< Shear of italic characters
        std::uint32_t                       m_style{Regular};                                //!< Text style (see Style enum)
        sf::Color                           m_fillColor{sf::Color::White};                   //!< Text fill color
        sf::Color                           m_outlineColor{sf::Color::Black};                //!< Text outline color
        float                               m_outlineThickness{0.f};                         //!< Thickness of the text's outline
        sf::Vector2f                        m_outlineOffset{};                               //!< Offset of the text's outline
        bool                                m_masked{};                                      //!< Does text masking rendering enabled?
        char32_t                            m_maskingCharacter{U'\u25CF'};              //!< Character used to mask the string
        LineAlignment                       m_lineAlignment{LineAlignment::Default};         //!< Line alignment for a multi-line text
        TextOrientation                     m_textOrientation{TextOrientation::Default};     //!< Text orientation
        ClusterGrouping                     m_clusterGrouping{ClusterGrouping::Character};   //!< Cluster grouping algorithm
        GlyphPreProcessor                   m_glyphPreProcessor;                             //!< Glyph pre-processor
        mutable sf::VertexArray             m_vertices{sf::PrimitiveType::Triangles};        //!< Vertex array containing the fill geometry
        mutable sf::VertexArray             m_outlineVertices{sf::PrimitiveType::Triangles}; //!< Vertex array containing the outline geometry
        mutable sf::FloatRect               m_bounds;                                        //!< Bounding rectangle of the text (in local coordinates)
        mutable bool                        m_geometryNeedUpdate{};                          //!< Does the geometry need to be recomputed?
        mutable std::uint64_t               m_fontTextureId{};                               //!< The font texture id
        mutable std::vector<ShapedGlyph>    m_glyphs;                                        //!< Cluster positions
        mutable std::shared_ptr<ShaperImpl> m_shaper;                                        //!< The shaper implementation
    };

}
