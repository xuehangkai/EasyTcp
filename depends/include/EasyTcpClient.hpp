#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#include"Cell.hpp"
#include"MessageHeader.hpp"
#include"CELLNetWork.hpp"
#include"CellClient.hpp"
#include"CELLFDSet.hpp"

class EasyTcpClient
{
public:
	EasyTcpClient() {
		_isConnect = false;
	}

	virtual ~EasyTcpClient() {
		Close();
	}

	//初始化socket
	SOCKET InitSocket(int sendSize=SEND_BUFF_SZIE,int recvSize=RECV_BUFF_SZIE) {
		CELLNetWork::Init();
		//1、建立一个socket
		if (_pClient) {
			CELLLog_Info("sock=%d关闭旧连接\n",(int)_pClient->getSockfd());
			Close();
		}
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock) {
			CELLLog_Info("建立套接字socket失败\n");
		}
		else
		{
			CELLNetWork::make_reuseaddr(sock);
			//CELLLog_Info("建立套接字socket=%d成功\n", (int)sock);
			_pClient = new CellClient(sock,sendSize,recvSize);
		}
		return sock;
	}
	//连接服务器
	int Connect(const char* ip,short port) {
		if (!_pClient) {
			if (INVALID_SOCKET==InitSocket()) {
				return SOCKET_ERROR;
			}
		}
		//CELLLog_Info("<socket=%d>正在连接服务器<%s:%d>。。。\n", (int)_pClient->getSockfd(), ip, port);
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net unsigned short
#ifdef	_WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_pClient->getSockfd(), (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			CELLLog_Info("socket=<%d>错误，连接服务器<%s:%d>失败\n",(int)_pClient->getSockfd(),ip,port);
		}
		else {
			_isConnect = true;
			//CELLLog_Info("<socket=%d>连接服务器<%s:%d>成功\n",(int)_pClient->getSockfd(), inet_ntoa(_sin.sin_addr),port);
		}
		return ret;
	}
	//关闭socket
	void Close() {
		//关闭Win Socket 2.x环境
		if (_pClient) {
			delete _pClient;
			_pClient = nullptr;
		}
		_isConnect = false;
	}
	//处理网络消息
	bool OnRun(int microseconds = 1) {
		if (isRun()) {
			SOCKET _sock = _pClient->getSockfd();

			_fdRead.zero();
			_fdRead.add(_sock);

			_fdWrite.zero();

			timeval t = { 0,microseconds };
			int ret = 0;
			if (_pClient->needWrite()) {
				_fdWrite.add(_sock);
				ret = select(_sock + 1, _fdRead.fdset(), _fdWrite.fdset(), nullptr, &t);
			}
			else {
				ret = select(_sock + 1, _fdRead.fdset(), nullptr, nullptr, &t);
			}
			if (ret < 0) {
				CELLLog_Info("error,<sock=%d>OnRun.select 任务结束。\n",(int) _sock);
				Close();
				return false;
			}
			if (_fdRead.has(_sock)) {
				if (SOCKET_ERROR == RecvData(_sock)) {
					CELLLog_Info("error,<sock=%d>OnRun.select RecvData任务结束。\n", (int)_sock);
					Close();
					return false;
				}
			}
			if (_fdWrite.has(_sock)) {
				if (SOCKET_ERROR == _pClient->SendDataReal()) {
					CELLLog_Info("error,<sock=%d>OnRun.select SendDataReal任务结束。\n", (int)_sock);
					Close();
					return false;
				}
			}
			//CELLLog_Info("空闲时间处理其他业务。。\n");
			//Sleep(1000);
			return true;
		}
		return false;
	}
	//是否工作中
	bool isRun() {
		return _pClient && _isConnect;
	}
	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET cSocket) {
		if (isRun()) {
			//接收客户端数据
			int nLen = _pClient->RecvData();
			if (nLen > 0) {
				//循环 判断是否有消息需要处理
				while (_pClient->hasMsg()) {
					//处理网络消息
					OnNetMsg(_pClient->front_msg());
					//移除消息队列（缓冲区）最前的一条数据
					_pClient->pop_front_msg();
				}
			}
			return nLen;
		}
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(netmsg_DataHeader* header)=0;
	//发送数据
	int SendData(netmsg_DataHeader* header) {
		if (isRun()) {
			return _pClient->SendData(header);
		}
		return 0;
	}
	int SendData(const char* pData, int len) {
		if (isRun()) {
			return _pClient->SendData(pData,len);
		}
		return 0;
	}
protected:
	CELLFDSet _fdRead;
	CELLFDSet _fdWrite;
	CellClient* _pClient=nullptr;
	bool _isConnect=false;
};
#endif