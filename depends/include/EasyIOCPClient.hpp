#ifndef _EasyIOCPClient_hpp_
#define _EasyIOCPClient_hpp_

#ifndef CELL_USE_IOCP
	#define CELL_USE_IOCP
#endif // !CELL_USE_IOCP

#include"EasyTcpClient.hpp"
#include"CellIOCP.hpp"

class EasyIOCPClient:public EasyTcpClient
{
public:
	void OnIniSocket(){
		_iocp.create();
		_iocp.reg(_pClient->getSockfd(),_pClient);
	}
	void Close(){
		_iocp.destory();
		EasyTcpClient::Close();
	}
	//处理网络消息
	bool OnRun(int microseconds = 1) {
		if (isRun()) {
			//需要写数据的客户端,才postSend
			if (_pClient->needWrite())
			{
				auto pIoData = _pClient->makeSendIoData();
				if (pIoData)
				{
					if (!_iocp.postSend(pIoData))
					{
						Close();
						return false;
					}
				}
				//--
				pIoData = _pClient->makeRecvIoData();
				if (pIoData)
				{
					if (!_iocp.postRecv(pIoData))
					{
						Close();
						return false;
					}
				}
			}
			else {
				auto pIoData = _pClient->makeRecvIoData();
				if (pIoData)
				{
					if (!_iocp.postRecv(pIoData))
					{
						Close();
						return false;
					}
				}
			}

			while (true)
			{
				int ret = DoIocpNetEvents(microseconds);
				if (ret < 0)
				{
					return false;
				}
				else if (ret == 0)
				{
					DoMsg();
					return true;
				}
			}
			return true;
		}
		return false;
	}

protected:
	int DoIocpNetEvents(int microseconds)
	{
		int ret = _iocp.wait(_ioEvent, microseconds);
		if (ret < 0)
		{
			CELLLog_Error("EasyIOCPClient.DoIocpNetEvents.wait clientID<%d> socketfd<%d>\n", _pClient->id,(int)_pClient->getSockfd());
			return ret;
		}
		else if (ret == 0)
		{
			return ret;
		}
		//---

		//接收数据 完成 Completion
		if (IO_TYPE::RECV == _ioEvent.pIoData->iotype)
		{
			if (_ioEvent.bytesTrans <= 0)
			{//客户端断开处理
				CELLLog_Error("EasyIOCPClient.DoIocpNetEvents.recv sockfd=%d, IO_TYPE::RECV bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
				Close();
				return -1;
			}
			//
			CellClient* pClient = (CellClient*)_ioEvent.data.ptr;
			if (pClient)
			{
				pClient->recv4iocp(_ioEvent.bytesTrans);
			}
			//
			//CELLLog_Info("IO_TYPE::RECV sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
		}
		//发送数据 完成 Completion
		else if (IO_TYPE::SEND == _ioEvent.pIoData->iotype)
		{
			if (_ioEvent.bytesTrans <= 0)
			{//客户端断开处理
				CELLLog_Error("EasyIOCPClient.DoIocpNetEvents.send sockfd=%d, IO_TYPE::SEND bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
				Close();
				return -1;
			}
			//
			CellClient* pClient = (CellClient*)_ioEvent.data.ptr;
			if (pClient)
				pClient->send2iocp(_ioEvent.bytesTrans);
			//
			//CELLLog_Info("IO_TYPE::SEND sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
		}
		else {
			CELLLog_Warring("undefine io type.\n");
		}
		return ret;
	}

	
protected:
	CELLIOCP _iocp;
	IO_EVENT _ioEvent = {};
};
#endif