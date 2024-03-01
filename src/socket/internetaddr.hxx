#pragma once

#include <cstdint>
struct internetaddr
{
    virtual ~internetaddr() = default;

    virtual int32_t getIp() const = 0;
    virtual void setIp(const int32_t ip) = 0;

    virtual uint16_t getPort() const = 0;
    virtual void setPort(const uint16_t port) = 0;
};