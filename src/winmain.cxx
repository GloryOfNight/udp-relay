
#include <WinSock2.h>

extern int relay_main(int argc, char* argv[], char* envp[]);

int main(int argc, char* argv[], char* envp[])
{
	WSADATA wsaData;
	const int wsaRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaRes != 0)
		return 1;

	const auto ret = relay_main(argc, argv, envp);

	WSACleanup();

	return ret;
}