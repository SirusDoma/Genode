#pragma once

#include <SFML/Graphics/Rect.hpp>

#include <Genode/SceneGraph/RenderableContainer.hpp>
#include <Genode/Graphics/Sprite.hpp>

#include <SFML/System/Time.hpp>

#include <initializer_list>
#include <functional>

namespace Gx
{
    class Animation : public Gx::Sprite
    {
    public:
        enum class AnimationState
        {
            Initial,
            Playing,
            Stopped,
            Completed
        };

        struct Frame
        {
            sf::IntRect                 TexCoords;
            std::optional<sf::Vector2f> Origin    = std::nullopt;
            std::optional<sf::Vector2f> Position  = std::nullopt;
            std::optional<float>        Rotation  = std::nullopt;
            std::optional<sf::Vector2f> Scale     = std::nullopt;
        };

        Animation();
        Animation(const sf::Texture& texture, const sf::Time& duration, std::initializer_list<Frame> frames);

        template<typename... Args>
        void AddFrame(const Frame& first, const Args&... args);
        void AddFrame(const Frame& frame);
        [[nodiscard]] unsigned int GetFrameCount() const;
        [[nodiscard]] Frame& GetFrame(unsigned int index);

        [[nodiscard]] const sf::Time& GetDuration() const;
        void SetDuration(const sf::Time& duration);

        [[nodiscard]] float GetSpeed() const;
        void SetSpeed(float speed);

        [[nodiscard]] unsigned int GetRepeatCount() const;
        void SetRepeatCount(unsigned int repeatCount);

        [[nodiscard]] bool IsLoop() const;
        void SetLoop(bool loop);

        [[nodiscard]] AnimationState GetState() const;
        void SetAnimationCallback(const std::function<void(Animation &)> &animationCallback);

        virtual void Stop();
        virtual void Reset();

        void Update(const sf::Time& delta) override;
        RenderStates Render(RenderSurface& surface, RenderStates states) const override;

    private:
        void SetFrame(unsigned int index);

        AnimationState m_state;
        sf::Time m_duration;
        sf::Time m_elapsed;
        float m_speed;
        unsigned int m_currentFrame;
        unsigned int m_currentRepeat;
        unsigned int m_repeatCount;
        bool m_loop;
        std::vector<Frame> m_frames;

        std::function<void(Animation& sender)> m_animationCallback;
    };
}

#include <Genode/Graphics/Animation.inl>
