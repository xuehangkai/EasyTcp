#ifndef _CELL_SERVER_HPP
#define _CELL_SERVER_HPP

#include <vector>
#include <map>

#include"Cell.hpp"
#include"CellClient.hpp"
#include"INetEvent.hpp"
#include"CELLConfig.hpp"

//网络消息接收处理服务类
class CellServer
{
public:
	
	virtual ~CellServer() {
		CELLLog_Info("CellServer<%d>.~CellServer exit begin\n", _id);
		Close();
		CELLLog_Info("CellServer<%d>.~CellServer exit end\n", _id);
	}

	void setId(int id) {
		_id = id;
		_taskServer.serverId = id;
	}

	virtual void setClientNum(int nScoketNum) {
		

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
			if (!_clientsBuff.empty())
			{
				//从缓冲队列里取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff) {
					_clients[pClient->getSockfd()] = pClient;
					pClient->serverId = _id;
					if (_pNetEvent) {
						_pNetEvent->OnNetJoin(pClient);
						OnClientJoin(pClient);
					}
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			if (_clients.empty()) {
				CELLThread::Sleep(1);
				//旧的时间戳
				_oldTime = CELLTime::getNowInMilliSec();
				continue;
			}

			CheckTime();
			if (!DoNetEvents()) {
				pThread->Exit();
			}
			DoMsg();
		}
		CELLLog_Info("CellServer<%d>.OnRun exit\n", _id);
	}

	virtual bool DoNetEvents() = 0;

	void CheckTime() {
		//当前时间戳
		auto nowTime =CELLTime::getNowInMilliSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			auto pClient = iter->second;
			//心跳检测
			if (iter->second->checkHeart(dt)) {
#ifdef CELL_USE_IOCP
				if (pClient->isPostIoAction())
					pClient->destory();
				else
					OnClientLeave(pClient);
#else
				OnClientLeave(pClient);
#endif // CELL_USE_IOCP
				_clients.erase(iter);
				iter++;
				continue;
			}
			////定时发送检测
			//pClient->checkSend(dt);
			iter++;
		}
	}

	void OnClientLeave(CellClient * pClient) {
		if (_pNetEvent)
			_pNetEvent->OnNetLeave(pClient);
		_clients_change = true;
		delete pClient;
	}

	virtual void OnClientJoin(CellClient * pClient){
		
	}

	void OnNetRecv(CellClient* pClient)
	{
		if (_pNetEvent)
			_pNetEvent->OnNetRecv(pClient);
	}

	void DoMsg() {
		CellClient* pClient = nullptr;
		for (auto itr:_clients) {
			pClient = itr.second;
			//循环 判断是否有消息需要处理
			while (pClient->hasMsg()) {
				//处理网络消息
				OnNetMsg(pClient, pClient->front_msg());
				//移除消息队列（缓冲区）最前的一条数据
				pClient->pop_front_msg();
			}
		}	
	}

	//接受数据 处理粘包 拆分包
	int RecvData(CellClient* pClient) {
		//接收客户端数据
		int nLen = pClient->RecvData();
		//触发<接收到网络数据>事件
		_pNetEvent->OnNetRecv(pClient);
		return nLen;
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
protected:
	//正式客户队列
	std::map<SOCKET, CellClient*> _clients;
private:
	//缓冲客户队列
	std::vector<CellClient*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
	//网络事件对象
	INetEvent* _pNetEvent=nullptr;
	//
	CellTaskServer _taskServer;
	//旧的时间戳
	time_t _oldTime = CELLTime::getNowInMilliSec();
	//
	CELLThread _thread;
protected:
	//
	int _id = -1;
	//客户列表是否有变化
	bool _clients_change=true;
};

#endif // !_CELL_SERVER_HPP