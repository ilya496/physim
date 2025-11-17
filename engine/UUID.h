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

    bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }
    bool operator!=(const UUID& other) const { return m_UUID != other.m_UUID; }
    operator uint64_t() const { return m_UUID; }

    std::string string() const { return std::to_string(m_UUID); }
private:
    uint64_t m_UUID;
};