#pragma once

#include <Genode/Tasks/Delay.hpp>

namespace Gx
{
    class Scheduler : public Task
    {
    public:
        Scheduler();
        Scheduler(sf::Time schedule, const std::function<void(const Scheduler&, const sf::Time&)> &update);
        Scheduler(std::size_t repeatCount, sf::Time schedule, const std::function<void(const Scheduler&, const sf::Time&)> &update);
        Scheduler(sf::Time duration, sf::Time interval, const std::function<void(const Scheduler&, const sf::Time&)> &update);

        [[nodiscard]] static Scheduler Once(sf::Time schedule, const std::function<void(const Scheduler&, const sf::Time&)> &update);

        [[nodiscard]] sf::Time GetDuration() const;
        [[nodiscard]] sf::Time GetInterval() const;

        void Complete() override;
        void Reset() override;

    protected:
        void Initialize() override;
        void Update(const sf::Time& delta) override;

    private:
        sf::Time m_duration, m_interval;
        sf::Time m_scheduleDelta;
        std::size_t m_count, m_repeatCount;
        std::function<void(const Scheduler&, const sf::Time&)> m_callback;
    };
}
