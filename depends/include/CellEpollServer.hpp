#ifndef _CELLEPOLLSERVER_HPP
#define _CELLEPOLLSERVER_HPP

#include"CellServer.hpp"
#include"CELLEpoll.hpp"

#include <vector>
#include <map>

//网络消息接收处理服务类
class CellEpollServer:public CellServer
{
public:
	CellEpollServer(){
		
	}

	~CellEpollServer(){
		Close();
	}

	void setClientNum(int nScoketNum) {
		_ep.create(nScoketNum);
	}

	bool DoNetEvents() {
		
		for(auto iter:_clients){
			if(iter.second->needWrite()){
				_ep.ctl(EPOLL_CTL_MOD,iter.second, EPOLLIN|EPOLLOUT);
			}else{
				_ep.ctl(EPOLL_CTL_MOD,iter.second, EPOLLIN);
			}
		}
		int ret=_ep.wait(1);
		if(ret<0){
			CELLLog_Error("CellepollServer<%d>.OnRun.wait Error exit\n", _id);
			return false;
		}else if(ret==0){
			return true;
		}else{
			auto events=_ep.events();
			for(int i=0;i<ret;i++){
				CellClient * pClient=(CellClient *)events[i].data.ptr;
				if(pClient){
					if(events[i].events & EPOLLIN){
						if(SOCKET_ERROR==RecvData(pClient)){
							rmClient(pClient);
							continue;
						}
					}
					if(events[i].events & EPOLLOUT){
						if(SOCKET_ERROR==pClient-> SendDataReal()){
							rmClient(pClient);
						}
					}
				}
			}
		}
		return true;
	}

	void rmClient(CellClient * pClient){
		auto iter=_clients.find(pClient->getSockfd());
		if(iter !=_clients.end()){
			_clients.erase(iter);
		}
		OnClientLeave(pClient);
	}

	void OnClientJoin(CellClient * pClient){
		_ep.ctl(EPOLL_CTL_ADD,pClient, EPOLLIN);
	}
private:
	CELLEpoll _ep;
};

#endif // !_CELL_SERVER_HPP