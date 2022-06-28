#ifndef _CELL_NET_WORK_HPP
#define _CELL_NET_WORK_HPP

#include"Cell.hpp"

class CELLNetWork
{
private:
	CELLNetWork() {
#ifdef _WIN32
		//启动Win Socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif

#ifndef _WIN32
		/*if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			return (1);*/
			//忽略异常信号量，默认情况会导致进程终止
		signal(SIGPIPE, SIG_IGN);
#endif
	}

	~CELLNetWork() {
#ifdef	_WIN32
		WSACleanup();
#endif
	}

public:
	static void Init() {
		static CELLNetWork obj;
	}
	static int make_reuseaddr(SOCKET fd) {
		int flag = 1;
		if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag))) {
			CELLLog_Info("setsockopt SO_REUSEADDR fail");
			return SOCKET_ERROR;
		}
		return 0;
	}
	static int destorySocket(SOCKET fd) {
#ifdef	_WIN32
		int ret=closesocket(fd);
#else
		int ret=close(fd);
#endif
		if (ret<0) {
			CELLLog_PError("cellnetwork::destorySocket sockfd<%d>",(int)fd);
		}
		return ret;
	}
private:

};

#endif // !_CELL_NET_WORK_HPP
