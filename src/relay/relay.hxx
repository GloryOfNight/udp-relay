#pragma once
#include "socket/internetaddr.hxx"
#include "socket/udpsocket.hxx"

#include <cstdint>
#include <memory>

class relay
{
public:
	relay() = default;
	~relay() = default;

	bool run();

	void stop();

private:
	bool init();

	bool bRunning{false};

	std::unique_ptr<udpsocket> m_socket;
};