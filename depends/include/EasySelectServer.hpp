#ifndef _EasySelectServer_hpp_
#define _EasySelectServer_hpp_


#include"EasyTcpServer.hpp"
#include"CellSelectServer.hpp"
#include"CELLFDSet.hpp"

//new 在堆内存
class EasySelectServer : public EasyTcpServer
{
public:
	void Start(int nCellServer) {
		EasyTcpServer::Start<CellSelectServer>(nCellServer);
	}

protected:
	//处理网络消息
	void onRun(CELLThread* pThread) {
		CELLFDSet fdRead;
		fdRead.create(_nMaxClient);
		while (pThread->isRun()) {
			time4msg();
			//fd_set fdWrite;
			//fd_set fdExp;
			fdRead.zero();
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);
			fdRead.add(sockfd());
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);

			timeval t = { 0,1 };
			int ret = select(sockfd() + 1, fdRead.fdset(), nullptr, nullptr, &t);
			if (ret < 0) {
				if (errno==EINTR) {
					continue;
				}
				CELLLog_PError("EasySelectServer.OnRun Accept Select exit。\n");
				pThread->Exit();
				break;
			}
			//判断描述符（socket）是否在集合中
			if (fdRead.has(sockfd())) {
				//fdRead.del(_sock);
				Accept();
			}
			//CELLLog_Info("空闲时间处理其他业务。。\n");
		}
		CELLLog_Info("EasySelectServer.OnRun exit\n");
	}
};

#endif