// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include <WinSock2.h>

extern int relay_healthchecker_main(int argc, char* argv[], char* envp[]);

int main(int argc, char* argv[], char* envp[])
{
	WSADATA wsaData;
	const int wsaRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaRes != 0)
		return 1;

	const auto ret = relay_healthchecker_main(argc, argv, envp);

	WSACleanup();

	return ret;
}