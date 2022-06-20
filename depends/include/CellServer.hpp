#ifndef _CELL_SERVER_HPP
#define _CELL_SERVER_HPP

#include <vector>
#include <map>

#include"Cell.hpp"
#include"CellClient.hpp"
#include"INetEvent.hpp"

//网络消息接收处理服务类
class CellServer
{
public:
	CellServer(int id) {
		_id = id;
		_pNetEvent = nullptr;  
		_taskServer.serverId = id;
	}
	~CellServer() {
		CELLLog_Info("CellServer<%d>.~CellServer exit begin\n", _id);
		Close();
		CELLLog_Info("CellServer<%d>.~CellServer exit end\n", _id);
	}

	void setEventObj(INetEvent* event) {
		_pNetEvent = event;
	}

	//关闭socket
	void Close() {
		CELLLog_Info("CellServer<%d>.Close begin\n", _id);
		_taskServer.Close();
		_thread.Close();
		CELLLog_Info("CellServer<%d>.Close end\n", _id);
	}
	//处理网络消息
	void onRun(CELLThread * pThread) {
		while (pThread->isRun())
		{
			if (_clientsBuff.size() > 0)
			{
				//从缓冲队列里取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff) {
					_clients[pClient->getSockfd()] = pClient;
					pClient->serverId = _id;
					if (_pNetEvent) {
						_pNetEvent->OnNetJoin(pClient);
					}
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			CheckTime();

			if (_clients.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				//旧的时间戳
				_oldTime = CELLTime::getNowInMilliSec();
				continue;
			}
			fd_set fdRead;
			fd_set fdWrite;
			//fd_set fdExc;
			if (_clients_change) {
				_clients_change = false;
				FD_ZERO(&fdRead);
				_maxSock = _clients.begin()->second->getSockfd();
				for (auto iter : _clients)
				{
					FD_SET(iter.second->getSockfd(), &fdRead);
					if (_maxSock < iter.second->getSockfd())
					{
						_maxSock = iter.second->getSockfd();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			bool bNeedWrite = false;
			FD_ZERO(&fdWrite);
			for (auto iter : _clients)
			{
				//检测需要写数据的客户端
				if (iter.second->needWrite())
				{
					bNeedWrite = true;
					FD_SET(iter.second->getSockfd(), &fdWrite);
				}
			}

			//memcpy(&fdWrite, &_fdRead_bak, sizeof(fd_set));
			
			//memcpy(&fdExc, &_fdRead_bak, sizeof(fd_set));
			///nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
			///既是所有文件描述符最大值+1 在Windows中这个参数可以写0
			timeval t{0,1};
			int ret = 0;
			if (bNeedWrite) {
				ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
			}
			else {
				ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);
			}
			if (ret < 0) {
				CELLLog_Info("CellServer<%d>.OnRun.select Error exit。\n",_id);
				pThread->Exit();
				break;
			}
			else if (ret == 0) {
				continue;
			}
			//CELLLog_Info("空闲时间处理其他业务。。\n");
			ReadData(fdRead);
			WriteData(fdWrite);
			//WriteData(fdExc);
			//CELLLog_Info("CellServer<%d>.OnRun.select: fdRead=%d, fdWrite=%d。\n", _id,fdRead.fd_count,fdWrite.fd_count);
			/*if (fdExc.fd_count>0) {
				CELLLog_Info("###fdExc=%d。\n", fdExc.fd_count);
			}*/

		}
		CELLLog_Info("CellServer<%d>.OnRun exit\n", _id);
	}
	void CheckTime() {
		//当前时间戳
		auto nowTime =CELLTime::getNowInMilliSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			//心跳检测
			if (iter->second->checkHeart(dt)) {
				if (_pNetEvent) {
					_pNetEvent->OnNetLeave(iter->second);
				}
				_clients_change = true;
				delete iter->second;
				auto iterOld = iter;
				iter++;
				_clients.erase(iterOld);
				continue;
			}
			////定时发送检测
			//iter->second->checkSend(dt);
			iter++;
		}
	}

	void OnClientLeave(CellClient * pClient) {
		if (_pNetEvent)
			_pNetEvent->OnNetLeave(pClient);
		_clients_change = true;
		delete pClient;
	}

	void WriteData(fd_set& fdWrite) {
#ifdef _WIN32
		//std::vector<CellClient*> temp;
		for (int n = 0; n < fdWrite.fd_count; n++)
		{
			auto iter = _clients.find(fdWrite.fd_array[n]);
			if (iter != _clients.end()) {
				if (-1 == iter->second->SendDataReal()) {
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
			if (iter->second->needWrite() && FD_ISSET(iter->second->getSockfd(), &fdWrite))
			{
				if (-1 == iter->second->SendDataReal())
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

	void ReadData(fd_set& fdRead) {
#ifdef _WIN32
		//std::vector<CellClient*> temp;
		for (int n = 0; n < fdRead.fd_count; n++)
		{
			auto iter = _clients.find(fdRead.fd_array[n]);
			if (iter != _clients.end()) {
				if (-1 == RecvData(iter->second)) {
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
			if (FD_ISSET(iter->second->getSockfd(), &fdRead))
			{
				if (-1 == RecvData(iter->second))
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
	//接受数据 处理粘包 拆分包
	int RecvData(CellClient* pClient) {
		//接收客户端数据
		int nLen = pClient->RecvData();
		if (nLen <= 0) {
			return -1;
		}
		//触发<接收到网络数据>事件
		_pNetEvent->OnNetRecv(pClient);
		//循环 判断是否有消息需要处理
		while (pClient->hasMsg()) {
			//处理网络消息
			OnNetMsg(pClient, pClient->front_msg());
			//移除消息队列（缓冲区）最前的一条数据
			pClient->pop_front_msg();
		}
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(CellClient* pClient, netmsg_DataHeader* header) {
		_pNetEvent->OnNetMsg(this, pClient, header);
		//_recvCount++;
		//auto t1 = _tTime.getElapsedSecond();
		//if (t1 >= 1.0) {
		//	CELLLog_Info("time<%lf>, socket<%d>, csocket<%d>, clients<%d>, recvCount<%d>\n", t1, _sock, cSocket, _clients.size(), _recvCount);
		//	_recvCount = 0;
		//	_tTime.update();
		//}
	}

	void addclient(CellClient* pClient) {
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}
	void Start() {
		_taskServer.Start();
		_thread.Start(nullptr, [this](CELLThread* pThread)->void{ onRun (pThread); }, [this](CELLThread* pThread)->void { 	ClearClients(); });
	}

	size_t getClientCount() {
		return _clients.size() + _clientsBuff.size();
	}
	//void addSendTask(CellClient* pClient, netmsg_DataHeader* header) {
	//	//CellSendMsg2ClientTask* task = new CellSendMsg2ClientTask(pClient, header);
	//	_taskServer.addTask([pClient,header]() {
	//		pClient->SendData(header);
	//		delete header;
	//	});
	//}
private:
	//
	void ClearClients() {
		for (auto iter : _clients) {
			delete iter.second;
		}
		_clients.clear();
		for (auto iter : _clientsBuff) {
			delete iter;
		}
		_clientsBuff.clear();
	}
private:
	//正式客户队列
	std::map<SOCKET, CellClient*> _clients;
	//缓冲客户队列
	std::vector<CellClient*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
	//网络事件对象
	INetEvent* _pNetEvent;
	//
	CellTaskServer _taskServer;
	//备份客户socket fd_set
	fd_set _fdRead_bak;
	//
	SOCKET _maxSock;
	//旧的时间戳
	time_t _oldTime = CELLTime::getNowInMilliSec();
	//
	CELLThread _thread;
	//
	int _id = -1;
	//客户列表是否有变化
	bool _clients_change=true;
};

#endif // !_CELL_SERVER_HPP
