#pragma once

#include "udpsocket.hxx"

#include <memory>

class udpsocketFactory
{
public:
    static std::unique_ptr<udpsocket> createUdpSocket(const int32_t port);
};