#ifndef _EasyIOCPServer_hpp_
#define _EasyIOCPServer_hpp_

#ifndef CELL_USE_IOCP
	#define CELL_USE_IOCP
#endif // !CELL_USE_IOCP

#include"EasyTcpServer.hpp"
#include"CELLIOCPServer.hpp"
#include"CELLIOCP.hpp"

class EasyIOCPServer : public EasyTcpServer
{
public:
	void Start(int nCELLServer)
	{
		EasyTcpServer::Start<CELLIOCPServer>(nCELLServer);
	}
protected:
	//处理网络消息
	void onRun(CELLThread* pThread)
	{
		CELLIOCP iocp;
		iocp.create();
		iocp.reg(sockfd());
		iocp.loadAcceptEx(sockfd());
		//
		//const int len = 2 * (sizeof(sockaddr_in) + 16);
		//不需要客户端再连接后立即发送数据的情况下最低长度len
		const int len = 1024;
		char buf[len] = {};

		IO_DATA_BASE ioData = {};
		ioData.wsabuff.buf = buf;
		ioData.wsabuff.len = len;

		iocp.postAccept(&ioData);
		//
		IO_EVENT ioEvent = {};
		while (pThread->isRun())
		{
			time4msg();
			//---
			int ret = iocp.wait(ioEvent, 1);
			if (ret < 0)
			{
				CELLLog_Error("EasyIOCPServer.OnRun ep.wait exit.\n");
				pThread->Exit();
				break;
			}
			if (ret == 0)
			{
				continue;
			}
			//---
			//接受连接 完成
			if (IO_TYPE::ACCEPT == ioEvent.pIoData->iotype)
			{
				//CELLLog_Info("新客户端加入 sockfd=%d", ioEvent.pIoData->sockfd);
				//
				IocpAccept(ioEvent.pIoData->sockfd);
				//继续 向IOCP投递接受连接任务
				iocp.postAccept(&ioData);
			}
		}
	}

	//接受客户端连接
	SOCKET IocpAccept(SOCKET cSock)
	{
		// 4 accept 等待接受客户端连接
		//sockaddr_in clientAddr = {};
		//int nAddrLen = sizeof(sockaddr_in);
		if (INVALID_SOCKET == cSock)
		{
			CELLLog_PError("accept INVALID_SOCKET...\n");
		}
		else
		{
			if (_clientAccept < _nMaxClient)
			{
				_clientAccept++;
				CELLNetWork::make_reuseaddr(cSock);
				//将新客户端分配给客户数量最少的cellServer
				addClientToCellServer(new CellClient(cSock, _nSendBuffSize, _nRecvBuffSize));
				//获取IP地址 inet_ntoa(clientAddr.sin_addr)
			}
			else {
				//获取IP地址 inet_ntoa(clientAddr.sin_addr)
				CELLNetWork::destorySocket(cSock);
				CELLLog_Warring("Accept to nMaxClient\n");
			}
		}
		return cSock;
	}
};

#endif // !_EasyIOCPServer_hpp_
