#ifndef _CellClient_hpp_
#define _CellClient_hpp_

#include"Cell.hpp"
#include"CELLBuffer.hpp"

//�ͻ����������������ʱʱ��
#define CLIENT_HEART_DEAD_TIME 60000

//�ڼ��ָ��ʱ���ѷ��ͻ������ڻ������Ϣ���ݷ��͸��ͻ���
#define CLIENT_SEND_BUFF_TIME 200

//�ͻ�����������
class CellClient
{
public:
	int id = -1;
	//����serverid
	int serverId = -1;
	//���Խ��շ��߼���
	//����server�����յ�����ϢID�Ƿ�����
	int nRecvMsgID = 1;
	//���Խ��շ��߼���
	//����client�����յ�����ϢID�Ƿ�����
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
	//�����������������ݷ��͸��ͻ���
	int SendDataReal() {
		resetDTSend();
		return _sendBuff.write2socket(_sockfd);
	}
	//�������Ŀ��Ƹ���ҵ������Ĳ��������
	//��������
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
	//�������
	bool checkHeart(time_t dt) {
		_dtHeart += dt;
		if (_dtHeart>=CLIENT_HEART_DEAD_TIME) {
			CELLLog_Info("checkHeart dead:socket= %d,time=%ld\n",(int)_sockfd, _dtHeart);
			return true;
		}
		return false;
	}
	//��ʱ������Ϣ
	bool checkSend(time_t dt) {
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//CELLLog_Info("_dtSend:socket= %d,time=%d\n", _sockfd, _dtSend);
			//�����������������ݷ��ͳ�ȥ
			SendDataReal();
			//���÷��ͼ�ʱ
			resetDTSend();
			return true;
		}
		return false;
	}
private:
	// socket fd_set file desc set
	SOCKET _sockfd;
	//������Ϣ������
	CELLBuffer _recvBuff;
	//���ͻ�����
	CELLBuffer _sendBuff;
	//����������ʱ
	time_t _dtHeart;
	//�ϴη�����Ϣ���ݵ�ʱ��
	time_t _dtSend;
	//���ͻ���������д���������
	int _sendBuffFullCount = 0;

};
#endif // !1
