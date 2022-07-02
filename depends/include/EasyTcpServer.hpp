#ifndef _EasyTcpServer_hpp_
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
#include"CELLConfig.hpp"

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
	//
	SOCKET _sock;

protected:
	//客户端发送缓冲区大小
	int _nSendBuffSize;
	//客户端接收缓冲区大小
	int _nRecvBuffSize;
	//客户端连接上限
	int _nMaxClient;
	//收到消息计数
	std::atomic_int _recvCount;
	//recv函数计数
	std::atomic_int _msgCount;
	//客户端计数
	std::atomic_int _clientAccept;
	//已分配客户端计数
	std::atomic_int _clientJoin;
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientAccept = 0;
		_msgCount = 0;
		_clientJoin = 0;
		_nSendBuffSize = CELLConfig::Instance().getInt("nSendBuffSize", SEND_BUFF_SZIE);
		_nRecvBuffSize = CELLConfig::Instance().getInt("nRecvBuffSize", RECV_BUFF_SZIE);
		_nMaxClient = CELLConfig::Instance().getInt("nMaxClient", FD_SETSIZE);
	}
	virtual ~EasyTcpServer() {
		Close();
	}

	//初始化socket
	SOCKET iniSocket() {
		CELLNetWork::Init();
		//1、建立一个socket
		if (INVALID_SOCKET != _sock) {
			CELLLog_Warring("sock=%d关闭旧连接\n",(int) _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			CELLLog_PError("建立套接字失败\n");
		}
		else
		{
			CELLNetWork::make_reuseaddr(_sock);
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
			CELLLog_PError("绑定端口<%d>失败\n",port);
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
			CELLLog_PError("socket=<%d>监听端口失败\n",(int)_sock);
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
			CELLLog_PError("socket=<%d>接受到无效的客户端socket\n",(int)_sock);
		}
		else
		{
			if (_clientAccept < _nMaxClient)
			{
				_clientAccept++;
				//将新客户端分配给客户数量最少的cellServer
				addClientToCellServer(new CellClient(cSocket, _nSendBuffSize, _nRecvBuffSize));
				//获取IP地址 inet_ntoa(clientAddr.sin_addr)
			}
			else {
				CELLNetWork::destorySocket(cSocket);
				CELLLog_Warring("Accept to nMaxClient.\n");
			}
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

	template<class ServerT>
	void Start(int nCellServer) {
		for (int n = 0; n < nCellServer;n++) {
			auto ser = new ServerT();
			ser->setId(n + 1);
			ser->setClientNum((_nMaxClient/ nCellServer)+1);
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
			CELLNetWork::destorySocket(_sock);
			_sock = INVALID_SOCKET;
		}
		CELLLog_Info("EasyTcpServer.Close end\n");
	}

	//只会被一个线程调用 安全
	virtual void OnNetJoin(CellClient* pClient) {
		_clientJoin++;
	}

	//cellserver 4 多个线程触发不安全
	virtual void OnNetLeave(CellClient* pClient) {
		_clientAccept--;
		_clientJoin--;
	}

	//cellserver 4 多个线程触发不安全
	virtual void OnNetMsg(CellServer* pCellServer,CellClient* pClient, netmsg_DataHeader* header) {
		_msgCount++;
	}

	//
	virtual void OnNetRecv(CellClient* pClient) {
		_recvCount++;
	}
protected:
	//处理网络消息
	virtual void onRun(CELLThread* pThread) = 0;

	//计算并输出每秒收到的网络消息
	void time4msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0) {
			CELLLog_Info("thread<%d>, time<%lf>, socket<%d>, accept<%d>, join<%d>, recv<%d>, msgCount<%d>\n", (int)_cellServers.size(), t1, (int)_sock, (int)_clientAccept, (int)_clientJoin, (int)_recvCount, (int)_msgCount);
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}
	SOCKET sockfd() {
		return _sock;
	}
};

#endif