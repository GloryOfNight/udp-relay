#pragma once
#include <cstdint>

using SOCKET = int32_t;

class relay
{
public:
	relay() = default;
	~relay() = default;

    bool init();

	void run();

	void stop();

private:
    SOCKET m_socket{};

    bool bRunning{false};
};