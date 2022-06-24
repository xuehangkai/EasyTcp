#ifndef _CELL_SERVER_HPP
#define _CELL_SERVER_HPP

#include <vector>
#include <map>

#include"Cell.hpp"
#include"CellClient.hpp"
#include"INetEvent.hpp"
#include"CELLConfig.hpp"
#include"CELLFDSet.hpp"

//������Ϣ���մ��������
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

	//�ر�socket
	void Close() {
		CELLLog_Info("CellServer<%d>.Close begin\n", _id);
		_taskServer.Close();
		_thread.Close();
		CELLLog_Info("CellServer<%d>.Close end\n", _id);
	}
	//����������Ϣ
	void onRun(CELLThread * pThread) {
		while (pThread->isRun())
		{
			if (!_clientsBuff.empty())
			{
				//�ӻ��������ȡ���ͻ�����
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

			if (_clients.empty()) {
				CELLThread::Sleep(1);
				//�ɵ�ʱ���
				_oldTime = CELLTime::getNowInMilliSec();
				continue;
			}

			CheckTime();
			if (!DoSelect()) {
				pThread->Exit();
			}
			DoMsg();
		}
		CELLLog_Info("CellServer<%d>.OnRun exit\n", _id);
	}

	bool DoSelect() {

		//fd_set fdExc;
		//����ɶ�����
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
		//�����д����
		bool bNeedWrite = false;
		_fdWrite.zero();
		for (auto iter : _clients)
		{
			//�����Ҫд���ݵĿͻ���
			if (iter.second->needWrite())
			{
				bNeedWrite = true;
				_fdWrite.add(iter.second->getSockfd());
			}
		}

		//memcpy(&fdWrite, &_fdRead_bak, sizeof(fd_set));

		//memcpy(&fdExc, &_fdRead_bak, sizeof(fd_set));
		///nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
		///���������ļ����������ֵ+1 ��Windows�������������д0
		timeval t{ 0,1 };
		int ret = 0;
		if (bNeedWrite) {
			ret = select(_maxSock + 1, _fdRead.fdset(), _fdWrite.fdset(), nullptr, &t);
		}
		else {
			ret = select(_maxSock + 1, _fdRead.fdset(), nullptr, nullptr, &t);
		}
		if (ret < 0) {
			CELLLog_Error("CellServer<%d>.OnRun.select Error exit:errno<%d>,errmsg<%s>\n", _id,errno,strerror(errno));
			return false;
		}
		else if (ret == 0) {
			return true;
		}
		//CELLLog_Info("����ʱ�䴦������ҵ�񡣡�\n");
		ReadData();
		WriteData();
		//WriteData(fdExc);
		//CELLLog_Info("CellServer<%d>.OnRun.select: fdRead=%d, fdWrite=%d��\n", _id,fdRead.fd_count,fdWrite.fd_count);
		/*if (fdExc.fd_count>0) {
			CELLLog_Info("###fdExc=%d��\n", fdExc.fd_count);
		}*/
		return true;
	}

	void CheckTime() {
		//��ǰʱ���
		auto nowTime =CELLTime::getNowInMilliSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			//�������
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
			////��ʱ���ͼ��
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

	void DoMsg() {
		CellClient* pClient = nullptr;
		for (auto itr:_clients) {
			pClient = itr.second;
			//ѭ�� �ж��Ƿ�����Ϣ��Ҫ����
			while (pClient->hasMsg()) {
				//����������Ϣ
				OnNetMsg(pClient, pClient->front_msg());
				//�Ƴ���Ϣ���У�����������ǰ��һ������
				pClient->pop_front_msg();
			}
		}	
	}

	//�������� ����ճ�� ��ְ�
	int RecvData(CellClient* pClient) {
		//���տͻ�������
		int nLen = pClient->RecvData();
		//����<���յ���������>�¼�
		_pNetEvent->OnNetRecv(pClient);
		return nLen;
	}
	//��Ӧ������Ϣ
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
	//��ʽ�ͻ�����
	std::map<SOCKET, CellClient*> _clients;
	//����ͻ�����
	std::vector<CellClient*> _clientsBuff;
	//������е���
	std::mutex _mutex;
	//�����¼�����
	INetEvent* _pNetEvent;
	//
	CellTaskServer _taskServer;
	//
	CELLFDSet _fdRead;
	CELLFDSet _fdWrite;
	//���ݿͻ�socket fd_set
	CELLFDSet _fdRead_bak;
	//
	SOCKET _maxSock;
	//�ɵ�ʱ���
	time_t _oldTime = CELLTime::getNowInMilliSec();
	//
	CELLThread _thread;
	//
	int _id = -1;
	//�ͻ��б��Ƿ��б仯
	bool _clients_change=true;
};

#endif // !_CELL_SERVER_HPP