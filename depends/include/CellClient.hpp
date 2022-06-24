#ifndef _CellClient_hpp_
#define _CellClient_hpp_

#include"Cell.hpp"
#include"CELLBuffer.hpp"

//客户端心跳检测死亡计时时间
#define CLIENT_HEART_DEAD_TIME 60000

//在间隔指定时间后把发送缓冲区内缓存的消息数据发送给客户端
#define CLIENT_SEND_BUFF_TIME 200

//客户端数据类型
class CellClient
{
public:
	int id = -1;
	//所属serverid
	int serverId = -1;
	//测试接收发逻辑用
	//用于server检测接收到的消息ID是否连续
	int nRecvMsgID = 1;
	//测试接收发逻辑用
	//用于client检测接收到的消息ID是否连续
	int nSendMsgID = 1;
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET, int sendSize = SEND_BUFF_SZIE, int recvSize = RECV_BUFF_SZIE) :
		_sendBuff(sendSize),
		_recvBuff(recvSize)
	{
		static int n = 1;
		id = n++;
		_sockfd = sockfd;

		resetDTHeart();
		resetDTSend();
	}
	~CellClient() {
		CELLLog_Debug("<s=%d>CellClient<%d>.~CellClient\n",serverId,id);
		if (INVALID_SOCKET!=_sockfd) {
#ifdef _WIN32
			closesocket(_sockfd);
#else
			close(_sockfd);
#endif
			_sockfd = INVALID_SOCKET;
		}
	}
	SOCKET getSockfd() {
		return _sockfd;
	}
	//
	int RecvData() {
		return _recvBuff.read4socket(_sockfd);
	}
	//
	bool hasMsg() {
		return _recvBuff.hasMsg();
	}
	//
	netmsg_DataHeader* front_msg() {
		return (netmsg_DataHeader *)_recvBuff.data();
	}
	//
	void pop_front_msg() {
		if (hasMsg()) {
			_recvBuff.pop(front_msg()->dataLength);
		}
	}
	//
	bool needWrite() {
		return _sendBuff.needWrite();
	}
	//立即将缓冲区的数据发送给客户端
	int SendDataReal() {
		resetDTSend();
		return _sendBuff.write2socket(_sockfd);
	}
	//缓冲区的控制根据业务需求的差异而调整
	//发送数据
	int SendData(netmsg_DataHeader* header) {
		return SendData((const char*)header, header->dataLength);
	}

	int SendData(const char*pData,int len) {
		if (_sendBuff.push((const char*)pData, len))
		{
			return len;
		}
		return SOCKET_ERROR;
	}
	//
	void resetDTHeart() {
		_dtHeart = 0;
	}
	void resetDTSend() {
		_dtSend = 0;
	}
	//心跳检测
	bool checkHeart(time_t dt) {
		_dtHeart += dt;
		if (_dtHeart>=CLIENT_HEART_DEAD_TIME) {
			CELLLog_Info("checkHeart dead:socket= %d,time=%ld\n",(int)_sockfd, _dtHeart);
			return true;
		}
		return false;
	}
	//定时发送消息
	bool checkSend(time_t dt) {
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//CELLLog_Info("_dtSend:socket= %d,time=%d\n", _sockfd, _dtSend);
			//立即将缓冲区的数据发送出去
			SendDataReal();
			//重置发送计时
			resetDTSend();
			return true;
		}
		return false;
	}
private:
	// socket fd_set file desc set
	SOCKET _sockfd;
	//接收消息缓冲区
	CELLBuffer _recvBuff;
	//发送缓冲区
	CELLBuffer _sendBuff;
	//心跳死亡计时
	time_t _dtHeart;
	//上次发送消息数据的时间
	time_t _dtSend;
	//发送缓冲区遇到写满情况计数
	int _sendBuffFullCount = 0;

};
#endif // !1
