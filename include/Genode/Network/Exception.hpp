#pragma once

#include <Genode/System/Exception.hpp>

#include <SFML/Network/Socket.hpp>

#include <fmt/format.h>
#include <string_view>

namespace Gx
{
    class NetworkException : public Exception
    {
    public:
        explicit NetworkException(const std::exception& ex) : Exception(ex.what()) {}

    protected:
        explicit NetworkException(const std::string& message) : Exception(message) {}
    };

    class ConnectionException : public NetworkException
    {
    public:
        explicit ConnectionException(
            const sf::Socket::Status status)
        : NetworkException(fmt::format("Unexpected socket state: {}", GetStatusName(status))),
          m_status(status) {}

        explicit ConnectionException(
            const sf::Socket::Status status,
            const std::string& message)
        : NetworkException(message), m_status(status) {}

        sf::Socket::Status GetStatus() const { return m_status; }

    private:
        static constexpr std::string_view GetStatusName(const sf::Socket::Status status)
        {
            switch (status)
            {
                case sf::Socket::Status::Done:         return "Done";
                case sf::Socket::Status::NotReady:     return "NotReady";
                case sf::Socket::Status::Partial:      return "Partial";
                case sf::Socket::Status::Disconnected: return "Disconnected";
                case sf::Socket::Status::Error:        return "Error";
            }

            return "Unknown";
        }

        sf::Socket::Status m_status;
    };

    class ProtocolException : public NetworkException
    {
    public:
        explicit ProtocolException(const std::string& message = "Invalid network protocol.") : NetworkException(message) {}
    };

    class TimeoutException : public NetworkException
    {
    public:
        explicit TimeoutException(const std::string& message = "Operation has timed out.") : NetworkException(message) {}
    };
}
