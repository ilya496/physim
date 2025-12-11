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
        // basic random 64bit ID; replace with better generator if required
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

    // Allow construction from string (needed for JSON restore)
    explicit UUID(const std::string& uuidString)
    {
        // simple conversion; assumes the string was created by string()
        m_UUID = std::stoull(uuidString);
    }

    ~UUID() = default;

    operator uint64_t() const { return m_UUID; }

    // String form used by JSON
    std::string string() const { return std::to_string(m_UUID); }

private:
    uint64_t m_UUID;
};

// Allow std::unordered_map<UUID, T>
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
