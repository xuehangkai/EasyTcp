﻿#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_


#include<thread>
#include<mutex>
#include <functional>
#include<atomic>

#include"Cell.hpp"
#include"CellClient.hpp"
#include"CellServer.hpp"
#include"INetEvent.hpp"
#include"CELLNetWork.hpp"


//new 在堆内存
class EasyTcpServer : public INetEvent
{
private:
	//
	CELLThread _thread;
	//消息处理对象，内部会创建线程
	std::vector<CellServer*> _cellServers;
	//每秒消息计时
	CELLTimestamp _tTime;
	SOCKET _sock;
protected:
	//收到消息计数
	std::atomic_int _recvCount;
	//客户端计数
	std::atomic_int _clientCount;
	//recv函数计数
	std::atomic_int _msgCount;

public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
		_msgCount = 0;
	}
	virtual ~EasyTcpServer() {
		Close();
	}

	//初始化socket
	SOCKET iniSocket() {
		CELLNetWork::Init();
		//1、建立一个socket
		if (INVALID_SOCKET != _sock) {
			CELLLog_Info("sock=%d关闭旧连接\n",(int) _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			CELLLog_Info("建立套接字失败\n");
		}
		else
		{
			CELLLog_Info("建立sock=%d套接字成功\n",(int)_sock);
		}
		return _sock;
	}
	//绑定ip和端口号
	int Bind(const char* ip,unsigned short port) {
		if (INVALID_SOCKET == _sock) {
			iniSocket();
		}
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net unsigned short
#ifdef	_WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (ret == SOCKET_ERROR)
		{
			CELLLog_Info("绑定端口<%d>失败\n",port);
		}
		else {
			CELLLog_Info("绑定端口<%d>成功\n",port);
		}
		return ret;
	}
	//监听端口号
	int Listen(int n) {
		int ret = listen(_sock, n);
		if (SOCKET_ERROR ==ret) {
			CELLLog_Info("socket=<%d>监听端口失败\n",(int)_sock);
		}
		else {
			CELLLog_Info("socket=<%d>监听端口成功\n",(int)_sock);
		}
		return ret;
	}
	//接受客户端连接
	SOCKET Accept() {
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET cSocket = INVALID_SOCKET;
#ifdef _WIN32
		cSocket = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSocket = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSocket) {
			CELLLog_Info("socket=<%d>接受到无效的客户端socket\n",(int)_sock);
		}
		else
		{
			netmsg_NewUserJoin userJoin;
			//SendDataToAll(&userJoin);
			addClientToCellServer(new CellClient(cSocket));
			//CELLLog_Info("socket=<%d>新客户端加入：csocket=%d ,ip= %s \n", (int)_sock,(int)cSocket, inet_ntoa(clientAddr.sin_addr));
		}
		return cSocket;
	}
	//将新客户端分配给客户数量最小的cellserver
	void addClientToCellServer(CellClient* pClient) {
		//查找客户数量最少cellserver消息处理对象
		auto pMinServer = _cellServers[0];
		for (auto pCellServer: _cellServers) {
			if (pMinServer->getClientCount() > pCellServer->getClientCount()) {
				pMinServer = pCellServer;
			}
		}
		pMinServer->addclient(pClient);
	}

	void Start(int nCellServer) {

		for (int n = 0; n < nCellServer;n++) {
			auto ser = new CellServer(n+1);
			_cellServers.push_back(ser);
			//注册网络事件接受对象
			ser->setEventObj(this);
			//启动服务线程
			ser->Start();
		}
		_thread.Start(nullptr, [this](CELLThread* pThread)->void { onRun(pThread); }, nullptr);
	}

	//关闭socket
	void Close() {
		CELLLog_Info("EasyTcpServer.Close begin\n");
		_thread.Close();
		//关闭Win Socket 2.x环境
		if (_sock != INVALID_SOCKET) {
			for (auto s:_cellServers) {
				delete s;
			}
			_cellServers.clear();
#ifdef	_WIN32
			closesocket(_sock);
#else
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
		CELLLog_Info("EasyTcpServer.Close end\n");
	}

	//只会被一个线程调用 安全
	virtual void OnNetJoin(CellClient* pClient) {
		_clientCount++;
	}

	//cellserver 4 多个线程触发不安全
	virtual void OnNetLeave(CellClient* pClient) {
		_clientCount--;
	}

	//cellserver 4 多个线程触发不安全
	virtual void OnNetMsg(CellServer* pCellServer,CellClient* pClient, netmsg_DataHeader* header) {
		_msgCount++;
	}

	//
	virtual void OnNetRecv(CellClient* pClient) {
		_recvCount++;
	}
private:
	//处理网络消息
	void onRun(CELLThread* pThread) {
		while (pThread->isRun()) {
			time4msg();
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExp;
			FD_ZERO(&fdRead);
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);
			FD_SET(_sock, &fdRead);
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);

			timeval t = { 0,1 };
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0) {
				CELLLog_Info("EasyTcpServer.OnRun Accept Select exit。\n");
				pThread->Exit();
				break;
			}
			//判断描述符（socket）是否在集合中
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			//CELLLog_Info("空闲时间处理其他业务。。\n");
		}
		CELLLog_Info("EasyTcpServer.OnRun exit\n");
	}

	//计算并输出每秒收到的网络消息
	void time4msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0) {
			CELLLog_Info("thread<%d>, time<%lf>, socket<%d>, clients<%d>, recvCount<%d>, msgCount<%d>\n", (int)_cellServers.size(), t1, (int)_sock, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}
};

#endif