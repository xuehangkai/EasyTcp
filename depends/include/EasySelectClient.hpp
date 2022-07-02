#ifndef _EasySelectClient_hpp_
#define _EasySelectClient_hpp_

#include"EasyTcpClient.hpp"
#include"CELLFDSet.hpp"

class EasySelectClient:public EasyTcpClient
{
public:
	EasySelectClient() {
		_fdRead.create(1);
		_fdWrite.create(1);
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
				if (SOCKET_ERROR == RecvData()) {
					CELLLog_Error("error,<sock=%d>OnRun.select RecvData任务结束。\n", (int)_sock);
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
protected:
	CELLFDSet _fdRead;
	CELLFDSet _fdWrite;
};
#endif