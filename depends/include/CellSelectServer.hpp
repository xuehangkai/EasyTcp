#ifndef _CELLSELECTSERVER_HPP
#define _CELLSELECTSERVER_HPP

#include"CellServer.hpp"
#include"CELLFDSet.hpp"

#include <vector>
#include <map>

//网络消息接收处理服务类
class CellSelectServer:public CellServer
{
public:
	CellSelectServer() {
		
	}

	~CellSelectServer(){
		Close();
	}

	void setClientNum(int nScoketNum) {
		_fdRead.create(nScoketNum);
		_fdWrite.create(nScoketNum);
		_fdRead_bak.create(nScoketNum);
	}

	bool DoNetEvents() {
		//fd_set fdExc;
		//计算可读集合
		if (_clients_change) {
			_clients_change = false;
			_fdRead.zero();
			_maxSock = _clients.begin()->second->getSockfd();
			for (auto iter : _clients)
			{
				_fdRead.add(iter.second->getSockfd());
				if (_maxSock < iter.second->getSockfd())
				{
					_maxSock = iter.second->getSockfd();
				}
			}
			_fdRead_bak.copy(_fdRead);
		}
		else {
			_fdRead.copy(_fdRead_bak);
		}
		//计算可写集合
		bool bNeedWrite = false;
		_fdWrite.zero();
		for (auto iter : _clients)
		{
			//检测需要写数据的客户端
			if (iter.second->needWrite())
			{
				bNeedWrite = true;
				_fdWrite.add(iter.second->getSockfd());
			}
		}

		//memcpy(&fdWrite, &_fdRead_bak, sizeof(fd_set));

		//memcpy(&fdExc, &_fdRead_bak, sizeof(fd_set));
		///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
		///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
		timeval t{ 0,1 };
		int ret = 0;
		if (bNeedWrite) {
			ret = select(_maxSock + 1, _fdRead.fdset(), _fdWrite.fdset(), nullptr, &t);
		}
		else {
			ret = select(_maxSock + 1, _fdRead.fdset(), nullptr, nullptr, &t);
		}
		if (ret < 0) {
			if (errno == EINTR) {
				return true;
			}
			CELLLog_PError("CellSelectServer<%d>.OnRun.select Error exit\n", _id);
			return false;
		}
		else if (ret == 0) {
			return true;
		}
		//CELLLog_Info("空闲时间处理其他业务。。\n");
		ReadData();
		WriteData();
		//WriteData(fdExc);
		//CELLLog_Info("CellSelectServer<%d>.OnRun.select: fdRead=%d, fdWrite=%d。\n", _id,fdRead.fd_count,fdWrite.fd_count);
		/*if (fdExc.fd_count>0) {
			CELLLog_Info("###fdExc=%d。\n", fdExc.fd_count);
		}*/
		return true;
	}

	void WriteData() {
#ifdef _WIN32
		auto pfdset = _fdWrite.fdset();
		//std::vector<CellClient*> temp;
		for (int n = 0; n < pfdset->fd_count; n++)
		{
			auto iter = _clients.find(pfdset->fd_array[n]);
			if (iter != _clients.end()) {
				if (SOCKET_ERROR == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
		}
		//CELLLog_Info("diyici<%d>\n", (int)te->getSockfd());
		//for (auto pClient : temp)
		//{
		//	_clients.erase(pClient->getSockfd());
		//	delete pClient;
		//}
#else
		for (auto iter =_clients.begin();iter!=_clients.end(); )
		{
			if (iter->second->needWrite() && _fdWrite.has(iter->second->getSockfd()))
			{
				if (SOCKET_ERROR == iter->second->SendDataReal())
				{
					OnClientLeave(iter->second);
					auto iterOld = iter;
					iter++;
					_clients.erase(iterOld);
					continue;
				}
			}
			iter++;
		}
#endif
	}

	void ReadData() {
#ifdef _WIN32
		auto pfdset = _fdRead.fdset();
		//std::vector<CellClient*> temp;
		for (int n = 0; n < pfdset->fd_count; n++)
		{
			auto iter = _clients.find(pfdset->fd_array[n]);
			if (iter != _clients.end()) {
				if (SOCKET_ERROR == RecvData(iter->second)) {
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
		}
		//CELLLog_Info("diyici<%d>\n", (int)te->getSockfd());
		//for (auto pClient : temp)
		//{
		//	_clients.erase(pClient->getSockfd());
		//	delete pClient;
		//}
#else
		for (auto iter = _clients.begin(); iter != _clients.end(); )
		{
			if (_fdRead.has(iter->second->getSockfd()))
			{
				if (SOCKET_ERROR == RecvData(iter->second))
				{
					OnClientLeave(iter->second);
					auto iterOld = iter;
					iter++;
					_clients.erase(iterOld);
					continue;
				}
			}
			iter++;
		}
#endif
	}
private:
	//
	CELLFDSet _fdRead;
	CELLFDSet _fdWrite;
	//备份客户socket fd_set
	CELLFDSet _fdRead_bak;
	//
	SOCKET _maxSock;
};

#endif // !_CELL_SERVER_HPP