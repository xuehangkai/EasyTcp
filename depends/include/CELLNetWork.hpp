﻿#ifndef _CELL_NET_WORK_HPP
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

	static int make_nonblocking(SOCKET fd)
	{
#ifdef _WIN32
		{
			unsigned long nonblocking = 1;
			if (ioctlsocket(fd, FIONBIO, &nonblocking) == SOCKET_ERROR) {
				CELLLog_Warring("fcntl(%d, F_GETFL)", (int)fd);
				return -1;
			}
		}
#else
		{
			int flags;
			if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
				CELLLog_Warring("fcntl(%d, F_GETFL)", fd);
				return -1;
			}
			if (!(flags & O_NONBLOCK)) {
				if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
					CELLLog_Warring("fcntl(%d, F_SETFL)", fd);
					return -1;
				}
			}
		}
#endif
		return 0;
	}

	static int make_reuseaddr(SOCKET fd) {
		int flag = 1;
		if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag))) {
			CELLLog_Info("setsockopt SO_REUSEADDR fail");
			return SOCKET_ERROR;
		}
		return 0;
	}

	static int make_nodelay(SOCKET fd)
	{
		int flag = 1;
		if (SOCKET_ERROR == setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag))) {
			CELLLog_Warring("setsockopt socket<%d> IPPROTO_TCP TCP_NODELAY failed", (int)fd);
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
