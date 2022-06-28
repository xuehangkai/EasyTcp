#ifndef _EasyEpollClient_hpp_
#define _EasyEpollClient_hpp_

#include"EasyTcpClient.hpp"
#include"CELLEpoll.hpp"

class EasyEpollClient:public EasyTcpClient
{
public:
	void OnIniSocket(){
		_ep.create(1);
		_ep.ctl(EPOLL_CTL_ADD,_pClient,EPOLLIN);
	}
	void Close(){
		_ep.destory();
		EasyTcpClient::Close();
	}
	//处理网络消息
	bool OnRun(int microseconds = 1) {
		if (isRun()) {
			if (_pClient->needWrite()) {
				_ep.ctl(EPOLL_CTL_MOD,_pClient, EPOLLIN|EPOLLOUT);
			}else{
				_ep.ctl(EPOLL_CTL_MOD,_pClient, EPOLLIN);
			}

			int ret=_ep.wait(microseconds);
			if(ret<0){
				CELLLog_Error("EasyEpollClient.OnRun.wait Error exit\n");
				return false;
			}else if(ret==0){
				return true;
			}
				auto events=_ep.events();
				for(int i=0;i<ret;i++){
					CellClient * pClient=(CellClient *)events[i].data.ptr;
					if(pClient){
						if(events[i].events & EPOLLIN){
							if(SOCKET_ERROR==RecvData()){
								CELLLog_Error("error,<sock=%d>OnRun.epoll RecvData任务结束。\n", (int)pClient->getSockfd());
								Close();
								continue;
							}
						}
						if(events[i].events & EPOLLOUT){
							if(SOCKET_ERROR==pClient-> SendDataReal()){
								CELLLog_Info("error,<sock=%d>OnRun.epoll SendDataReal任务结束。\n", (int)pClient->getSockfd());
								Close();
							}
						}
					}
				}
			
			return true;
		}
		return false;
	}
protected:
	CELLEpoll _ep;
};
#endif