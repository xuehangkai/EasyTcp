#ifndef _CELL_EPOLL_HPP_
#define _CELL_EPOLL_HPP_

#if __linux__

#include"Cell.hpp"
#include"CellClient.hpp"
#include<sys/epoll.h>
#define EPOLL_ERROR (-1)

class CELLEpoll
{

public:
	~CELLEpoll(){
		destory();
	}
	int create(int maxEvents) {
		if (_epfd>0) {
			//warring
			destory();
		}
		_epfd = epoll_create(maxEvents);
		if (EPOLL_ERROR ==_epfd) {
			CELLLog_PError("epoll_create \n");
			return _epfd;
		}
		nMaxEvents = maxEvents;
		_pEvents = new epoll_event[maxEvents];
		return _epfd;
	}

	void destory() {
		if (_epfd>0) {
			CELLNetWork::destorySocket(_epfd);
			_epfd =- 1;
		}
		if (_pEvents) {
			delete[] _pEvents;
			_pEvents = nullptr;
		}
	}

	int ctl(int op,SOCKET sockfd,uint32_t events) {
		epoll_event ev;
		ev.events = events;
		ev.data.fd = sockfd;
		int ret = epoll_ctl(_epfd, op, sockfd, &ev);
		if (EPOLL_ERROR==ret) {
			CELLLog_PError("epoll_ctl1\n");
		}
		return ret;
	}

	int ctl(int op,CellClient* pClient,uint32_t events) {
		epoll_event ev;
		ev.events = events;
		ev.data.ptr = pClient;
		int ret = epoll_ctl(_epfd, op, pClient->getSockfd(), &ev);
		if (EPOLL_ERROR==ret) {
			CELLLog_PError("epoll_ctl2\n");
		}
		return ret;
	}

	int wait(int timeout) {
		int ret = epoll_wait(_epfd, _pEvents, nMaxEvents, timeout);
		if (EPOLL_ERROR==ret) {
			if (errno == EINTR) {
				CELLLog_Info("epoll_wait EINTR\n");
				return 0;
			}
			CELLLog_PError("epoll_wait\n");
		}
		return ret;
	}

	epoll_event* events() {
		return _pEvents;
	}

private:
	epoll_event* _pEvents=nullptr;
	int nMaxEvents = 1;
	int _epfd = -1;
};

#endif // __LINUX__
#endif // !_CELL_EPOLL_HPP_
