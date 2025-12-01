#pragma once

#include <cstdint>
#include <string>

class UUID
{
public:
    UUID();
    UUID(uint64_t uuid);
    UUID(const UUID& other);
    ~UUID() = default;

    // bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }
    // bool operator!=(const UUID& other) const { return m_UUID != other.m_UUID; }
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
