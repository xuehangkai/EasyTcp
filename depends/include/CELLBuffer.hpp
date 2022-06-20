#ifndef _CELL_BUFFER_HPP_
#define _CELL_BUFFER_HPP_

#include"Cell.hpp"

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
			//数据尾部置清零
			_nlast = 0;
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
				//CELLLog_Info("客户端<Socket=%d>退出,任务结束\n", (int)pClient->getSockfd());
				CELLLog_Info("read4socket, nLen=%d\n",nLen);
				return nLen;
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
};


#endif // !1
