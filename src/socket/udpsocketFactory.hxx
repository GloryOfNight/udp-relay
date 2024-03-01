#pragma once

#include "udpsocket.hxx"

#include <memory>

class udpsocketFactory
{
public:
	static std::unique_ptr<udpsocket> createUdpSocket();

	static std::unique_ptr<internetaddr> createInternetAddrUnique();

	static std::shared_ptr<internetaddr> createInternetAddr();
};