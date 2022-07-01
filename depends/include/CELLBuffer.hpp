#ifndef _CELL_BUFFER_HPP_
#define _CELL_BUFFER_HPP_

#include"Cell.hpp"

#ifdef CELL_USE_IOCP
	#include"CELLIOCP.hpp"
#endif

class CELLBuffer
{
public:
	CELLBuffer(int nSize=8192) {
		_nSize = nSize;
		_pBuff = new char[_nSize];
	}
	~CELLBuffer() {
		if (_pBuff) {
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}
	//
	char* data() {
		return _pBuff;
	}
	bool push(const char* pData,int nLen){

		////写入大量数据不一定要放到内存中
		////也可以存储到数据库或者磁盘等存储器中
		//if (_nlast + nLen > _nSize) {
		//	//需要写入的数据大于可用空间
		//	int n = (_nlast + nLen)-_nSize;
		//	//拓展buff大小
		//	if (n<8192) {
		//		n = 8192;
		//	}
		//	char* buff = new char[_nSize + n];
		//	memcpy(buff, _pBuff, _nlast);
		//	delete[] _pBuff;
		//	_pBuff = buff;
		//}

		if (_nlast + nLen <= _nSize)
		{
			//将要发送的数据拷贝到发送缓冲区尾部
			memcpy(_pBuff + _nlast, pData, nLen);
			//计算数据尾部位置
			_nlast += nLen;
			if (_nlast == _nSize) {
				++_fullCount;
			}
			return true;
		}
		else {
			++_fullCount;
		}
		return false;
	}
	//
	void pop(int nLen) {
		int n = _nlast - nLen;
		if (n>0) {
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		_nlast = n;
		if (_fullCount>0) {
			--_fullCount;
		}
	}
	//立即将缓冲区的数据发送给客户端
	int write2socket(SOCKET sockfd) {
		int ret = 0;
		if (_nlast > 0 && INVALID_SOCKET != sockfd) {
			//发送数据
			ret = send(sockfd, _pBuff, _nlast, 0);

			if (ret<=0) {
				CELLLog_PError("write2socket:sockfd<%d> nsize<%d> nlast<%d> ret<%d>\n",sockfd,_nSize,_nlast,ret);
				return SOCKET_ERROR;
			}
			if (ret==_nlast) {
				_nlast = 0;
			}
			else {
				_nlast -= ret;
				memcpy(_pBuff, _pBuff + ret, _nlast);
			}
			//
			_fullCount = 0;
		}
		return ret;
	}
	//
	int read4socket(SOCKET sockfd) {
		if (_nSize - _nlast>0) {
			//接收客户端数据
			char* szRecv = _pBuff + _nlast;
			int nLen = (int)recv(sockfd, szRecv, _nSize - _nlast, 0);

			if (nLen <= 0) {
				CELLLog_PError("read4socket:sockfd<%d> nsize<%d> nlast<%d> nlen<%d>\n", sockfd, _nSize, _nlast, nLen);
				return SOCKET_ERROR;
			}
			//消息缓冲区的数据尾部位置后移
			_nlast += nLen;
			return nLen;
		}
		return 0;
	}
	bool hasMsg() {
		if (_nlast >= sizeof(netmsg_DataHeader)) {
			//这时就可以知道当前消息的长度
			netmsg_DataHeader* header = (netmsg_DataHeader*)_pBuff;
			//判断消息缓冲区的数据长度大于消息长度
			return _nlast >= header->dataLength;
		}
		return false;
	}
	bool needWrite() {
		return _nlast>0;
	}

#ifdef CELL_USE_IOCP
	IO_DATA_BASE* makeRecvIoData(SOCKET sockfd)
	{
		int nLen = _nSize - _nlast;
		if (nLen > 0)
		{
			_ioData.wsabuff.buf = _pBuff + _nlast;
			_ioData.wsabuff.len = nLen;
			_ioData.sockfd = sockfd;
			return &_ioData;
		}
		return nullptr;
	}

	IO_DATA_BASE* makeSendIoData(SOCKET sockfd)
	{
		if (_nlast > 0)
		{
			_ioData.wsabuff.buf = _pBuff;
			_ioData.wsabuff.len = _nlast;
			_ioData.sockfd = sockfd;
			return &_ioData;
		}
		return nullptr;
	}

	bool read4iocp(int nRecv)
	{
		if (nRecv > 0 && _nSize - _nlast >= nRecv)
		{
			_nlast += nRecv;
			return true;
		}
		CELLLog_Error("read4iocp:sockfd<%d> nSize<%d> nLast<%d> nRecv<%d>\n", _ioData.sockfd, _nSize, _nlast, nRecv);
		return false;
	}

	bool write2iocp(int nSend)
	{
		if (_nlast < nSend)
		{
			CELLLog_Error("write2iocp:sockfd<%d> nSize<%d> nLast<%d> nSend<%d>\n", _ioData.sockfd, _nSize, _nlast, nSend);
			return false;
		}
		if (_nlast == nSend)
		{//_nlast=2000 实际发送nSend=2000
		 //数据尾部位置清零
			_nlast = 0;
		}
		else {
			//_nlast=2000 实际发送ret=1000
			_nlast -= nSend;
			memcpy(_pBuff, _pBuff + nSend, _nlast);
		}
		_fullCount = 0;
		return true;
	}
#endif // CELL_USE_IOCP
private:
	//第二缓冲区 发送缓冲区
	char * _pBuff=nullptr;
	////可以使用链表和队列来管理缓冲数据块
	//list<char*> _pBuffList;
	//消息缓冲区的数据尾部位置
	int _nlast=0;
	//缓冲区总的空间大小，字节长度
	int _nSize=0;
	//缓冲区写次数满计数
	int _fullCount = 0;
#ifdef CELL_USE_IOCP
	IO_DATA_BASE _ioData = {};
#endif // CELL_USE_IOCP
};


#endif // !1
