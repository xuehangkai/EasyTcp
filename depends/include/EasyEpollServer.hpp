#ifndef _EasyEpollServer_hpp_
#define _EasyEpollServer_hpp_

#include"EasyTcpServer.hpp"
#include"CellEpollServer.hpp"
#include"CELLEpoll.hpp"

//new 在堆内存
class EasyEpollServer : public EasyTcpServer
{
public:
	void Start(int nCellServer) {
		EasyTcpServer::Start<CellEpollServer>(nCellServer);
	}

protected:
	//处理网络消息
	void onRun(CELLThread* pThread) {
		CELLEpoll ep;
		ep.create(1);
		ep.ctl(EPOLL_CTL_ADD,sockfd(), EPOLLIN);
		while (pThread->isRun()) {
			time4msg();
			int ret = ep.wait(1);
			if (ret < 0) {
				CELLLog_PError("EasyEpollServer.OnRun Accept Select exit。\n");
				pThread->Exit();
				break;
			}
			//判断描述符（socket）是否在集合中
			auto events = ep.events();
			for(int i = 0; i < ret; i++)
			{
				//当服务端socket发生事件时，表示有新客户端连接
				if(events[i].data.fd == sockfd())
				{
					if(events[i].events & EPOLLIN)
					{
						Accept();
					}
				}
			}
			//CELLLog_Info("空闲时间处理其他业务。。\n");
		}
		CELLLog_Info("EasySelectServer.OnRun exit\n");
	}
};

#endif