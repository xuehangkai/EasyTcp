#ifndef _CELL_FDSET_HPP_
#define _CELL_FDSET_HPP_

#include"Cell.hpp"

//#define CELL_MAX_FD 10240

class CELLFDSet
{

public:
	CELLFDSet() {

	}
	~CELLFDSet() {
		destory();
	}
	//linux下表示sock fd最大值
	//windows下表示sock fd的数量
	void create(int maxFds) {
		int nSocketNum = maxFds;
#ifdef _WIN32
		if (nSocketNum<64) {
			nSocketNum = 64;
		}
		_nfdSize = sizeof(u_int) + (sizeof(SOCKET) * nSocketNum);
#else
		if (nSocketNum < 65535) {
			nSocketNum = 65535;
		}
		_nfdSize = nSocketNum / (8 * sizeof(char))+1;
#endif // _WIN32
		_pfdset = (fd_set*)new char[_nfdSize];
		memset(_pfdset, 0, _nfdSize);
		_MAX_SOCK_FD = nSocketNum;
	}

	void destory() {
		if (_pfdset) {
			delete[] _pfdset;
			_pfdset = nullptr;
		}
	}

	inline void add(SOCKET s) {
#ifdef _WIN32
		FD_SET(s, _pfdset);
#else
		if (s < _MAX_SOCK_FD) {
			FD_SET(s, _pfdset);
		}
		else {
			CELLLog_Error("CELLFDSet::add socket<%d>, CELL_MAX_FD<%d> ",(int)s, _MAX_SOCK_FD);
		}
#endif
	}
	inline void del(SOCKET s) {
		FD_CLR(s,_pfdset);
	}
	inline void zero() {
#ifdef _WIN32
		FD_ZERO(_pfdset);
#else
		memset(_pfdset, 0, _nfdSize);
#endif
	}
	inline bool has(SOCKET s) {
		return FD_ISSET(s,_pfdset);
	}
	inline fd_set* fdset() {
		return _pfdset;
	}
	void copy(CELLFDSet& set ) {
		memcpy(_pfdset, set._pfdset, set._nfdSize);
		_nfdSize = set._nfdSize;
	}
private:
	fd_set* _pfdset = nullptr;
	size_t _nfdSize = 0;
	int _MAX_SOCK_FD = 0;
};


#endif // !_CELL_THREAD_HPP_
