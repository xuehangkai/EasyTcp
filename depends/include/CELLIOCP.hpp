#ifndef _CELL_IOCP_HPP_
#define _CELL_IOCP_HPP_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include<windows.h>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#include<mswsock.h>
#include<stdio.h>

#include"CELLLog.hpp"

enum IO_TYPE
{
	ACCEPT = 10,
	RECV,
	SEND
};

//数据缓冲区空间大小
//#define IO_DATA_BUFF_SIZE 1024

struct IO_DATA_BASE
{
	//重叠体
	OVERLAPPED	overlapped;
	//
	SOCKET		sockfd;
	//数据缓冲区
	WSABUF		wsabuff;
	//操作类型
	IO_TYPE		iotype;
};

struct IO_EVENT
{
	union
	{
		void* ptr;
		SOCKET sockfd;
	}data;
	IO_DATA_BASE* pIoData;
	DWORD bytesTrans = 0;
};

class CELLIOCP
{
public:
	~CELLIOCP()
	{
		destory();
	}

	//创建完成端口IOCP
	bool create()
	{
		_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (NULL == _completionPort)
		{
			CELLLog_PError("IOCP create failed, CreateIoCompletionPort");
			return false;
		}
		return true;
	}

	void destory()
	{
		if (_completionPort)
		{
			CloseHandle(_completionPort);
			_completionPort = NULL;
		}
	}

	//关联IOCP与sockfd
	bool reg(SOCKET sockfd)
	{
		//完成键
		auto ret = CreateIoCompletionPort((HANDLE)sockfd, _completionPort, (ULONG_PTR)sockfd, 0);
		if (!ret)
		{
			CELLLog_PError("IOCP reg sockfd failed, CreateIoCompletionPort");
			return false;
		}
		return true;
	}

	//关联IOCP与自定义数据地址
	bool reg(SOCKET sockfd, void* ptr)
	{
		//完成键
		auto ret = CreateIoCompletionPort((HANDLE)sockfd, _completionPort, (ULONG_PTR)ptr, 0);
		if (!ret)
		{
			CELLLog_PError("IOCP reg sockfd+ptr failed, CreateIoCompletionPort");
			return false;
		}
		return true;
	}

	//向IOCP投递接受连接的任务
	bool postAccept(IO_DATA_BASE* pIO_DATA)
	{
		if (!_AcceptEx)
		{
			CELLLog_Error("error, postAccept _AcceptEx is null");
			return false;
		}

		pIO_DATA->iotype = IO_TYPE::ACCEPT;
		pIO_DATA->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (FALSE == _AcceptEx(_sockServer
			, pIO_DATA->sockfd
			, pIO_DATA->wsabuff.buf
			, 0
			, sizeof(sockaddr_in) + 16
			, sizeof(sockaddr_in) + 16
			, NULL
			, &pIO_DATA->overlapped
		))
		{
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err)
			{
				CELLLog_Error("AcceptEx failed with error %d", err);
				return false;
			}
		}
		return true;
	}

	//向IOCP投递接收数据的任务
	bool postRecv(IO_DATA_BASE* pIO_DATA)
	{
		pIO_DATA->iotype = IO_TYPE::RECV;
		DWORD flags = 0;
		ZeroMemory(&pIO_DATA->overlapped, sizeof(OVERLAPPED));

		if (SOCKET_ERROR == WSARecv(pIO_DATA->sockfd, &pIO_DATA->wsabuff, 1, NULL, &flags, &pIO_DATA->overlapped, NULL))
		{
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err)
			{
				if (WSAECONNRESET == err)
				{
					return false;
				}
				CELLLog_Error("WSARecv failed with error %d", err);
				return false;
			}
		}
		return true;
	}
	//向IOCP投递发送数据的任务
	bool postSend(IO_DATA_BASE* pIO_DATA)
	{
		pIO_DATA->iotype = IO_TYPE::SEND;
		DWORD flags = 0;
		ZeroMemory(&pIO_DATA->overlapped, sizeof(OVERLAPPED));

		if (SOCKET_ERROR == WSASend(pIO_DATA->sockfd, &pIO_DATA->wsabuff, 1, NULL, flags, &pIO_DATA->overlapped, NULL))
		{
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err)
			{
				if (WSAECONNRESET == err)
				{
					return false;
				}
				CELLLog_Error("WSASend failed with error %d", err);
				return false;
			}
		}
		return true;
	}

	int wait(IO_EVENT& ioEvent, int timeout)
	{
		ioEvent.bytesTrans = 0;
		ioEvent.pIoData = NULL;
		ioEvent.data.ptr = NULL;
		if (FALSE == GetQueuedCompletionStatus(_completionPort
			, &ioEvent.bytesTrans
			, (PULONG_PTR)&ioEvent.data
			, (LPOVERLAPPED*)&ioEvent.pIoData
			, timeout))
		{
			int err = GetLastError();
			if (WAIT_TIMEOUT == err)
			{
				return 0;
			}
			if (ERROR_NETNAME_DELETED == err)
			{
				return 1;
			}
			if (ERROR_CONNECTION_ABORTED == err)
			{
				return 1;
			}
			CELLLog_PError("GetQueuedCompletionStatus failed");
			return -1;
		}
		return 1;
	}

	bool loadAcceptEx(SOCKET ListenSocket)
	{
		if (INVALID_SOCKET != _sockServer)
		{
			CELLLog_Error("loadAcceptEx _sockServer != INVALID_SOCKET");
			return false;
		}
		if (_AcceptEx)
		{
			CELLLog_Error("loadAcceptEx _AcceptEx != NULL");
			return false;
		}
		_sockServer = ListenSocket;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;
		DWORD dwBytes = 0;
		int iResult = WSAIoctl(ListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidAcceptEx, sizeof(GuidAcceptEx),
			&_AcceptEx, sizeof(_AcceptEx),
			&dwBytes, NULL, NULL);

		if (iResult == SOCKET_ERROR) {
			CELLLog_Error("WSAIoctl failed with error: %u", WSAGetLastError());
			return false;
		}
		return true;
	}
private:
	//将AcceptEx函数加载内存中，调用效率更高
	LPFN_ACCEPTEX _AcceptEx = NULL;
	HANDLE _completionPort = NULL;
	SOCKET _sockServer = INVALID_SOCKET;
};

#endif //_WIN32

#endif // !_CELL_IOCP_HPP_