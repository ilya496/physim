#pragma once

#include <cstdint>
#include <string>
#include <random>
#include <sstream>

class UUID
{
public:
    UUID()
    {
        static std::random_device rd;
        static std::mt19937_64 eng(rd());
        static std::uniform_int_distribution<uint64_t> distr;

        m_UUID = distr(eng);
    }

    UUID(uint64_t uuid)
        : m_UUID(uuid)
    {
    }

    UUID(const UUID& other)
        : m_UUID(other.m_UUID)
    {
    }

    explicit UUID(const std::string& uuidString)
    {
        m_UUID = std::stoull(uuidString);
    }

    ~UUID() = default;

    operator uint64_t() const { return m_UUID; }

    std::string string() const { return std::to_string(m_UUID); }

private:
    uint64_t m_UUID;
};

namespace std {
    template<>
    struct hash<UUID>
    {
        std::size_t operator()(const UUID& uuid) const noexcept
        {
            return std::hash<uint64_t>{}(static_cast<uint64_t>(uuid));
        }
    };
}
