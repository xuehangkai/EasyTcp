#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_

#include"CELLLog.hpp"

#include<cstdint>
#include<string>

//�ֽ���BYTE
class CELLStream
{
public:
	CELLStream(char * pData,int nSize,bool bDelete=false) {
		_nSize = nSize;
		_pBuff = pData;
		_bDelete = bDelete;
	}
	CELLStream(int nSize = 1024) {
		_nSize = nSize;
		_pBuff = new char[_nSize];
		_bDelete = true;
	}
	virtual ~CELLStream() {
		if (_bDelete&&_pBuff) {
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}

public:

	char* data() {
		return _pBuff;
	}
	int length() {
		return _nWritePos;
	}

	//��������
	//���ܶ���n�ֽڵ�������
	inline bool canRead(int n) {
		return _nSize - _nReadPos >=n;
	}
	//����д��n�ֽڵ�������
	inline bool canWrite(int n) {
		return _nSize - _nWritePos >= n;
	}
	//��д��λ�ã��������n�ֽڳ���
	inline void push(int n) {
		_nWritePos += n;
	}
	//�Ѷ�ȡλ�ã��������n�ֽڳ���
	inline void pop(int n) {
		_nReadPos += n;
	}
	inline void setWritePos(int n) {
		_nWritePos = n;
	}
	inline int  getWritePos() {
		return _nWritePos;
	}

//////read
	template<typename T>
	bool Read(T& n,bool bOffset=true) {
		//����Ҫ��ȡ���ݵ��ֽڳ���
		auto nLen = sizeof(T);
		//�ж��ܲ��ܶ�
		if (canRead(nLen)) {
			//��Ҫ��ȡ������ ��������
			memcpy(&n, _pBuff + _nReadPos, nLen);
			//�����Ѷ�����λ��
			if(bOffset)
				pop(nLen);
			return true;
		}
		//����assert
		//������־
		CELLLog_Error("CELLStream::Read faild\n");
		return false;
	}
	template<typename T>
	bool onlyRead(T &n) {
		return Read(n,false);
	}
	template<typename T>
	uint32_t ReadArray(T* pArr,uint32_t len) {
		
		uint32_t len1 = 0;
		//��ȡ����Ԫ�ظ���������ƫ�ƶ�ȡλ��
		Read(len1,false);
		//�жϻ��������ܷ�ŵ���
		if (len1<=len) {
			//����������ֽڳ���
			auto nLen = len1 * sizeof(T);
			//�ж��ܲ��ܶ���
			if (canRead(nLen+sizeof(uint32_t))) {
				//�����Ѷ�λ��+���鳤����ռ�пռ�
				pop(sizeof(uint32_t));
				//��Ҫ��ȡ������ ��������
				memcpy(pArr, _pBuff + _nReadPos, nLen);
				//�����Ѷ�����λ��
				pop(nLen);
				return len1;
			}
		}
		_nReadPos -= sizeof(uint32_t);
		CELLLog_Error("CELLStream::ReadArray faild");
		return 0;

	}

	int8_t ReadInt8(int8_t def=0) {
		Read(def);
		return def;
	}
	int16_t ReadInt16(int16_t n = 0) {
		Read(n);
		return n;
	}
	int32_t ReadInt32(int32_t n = 0) {
		Read(n);
		return n;
	}
	float ReadFloat(float n = 0.0f) {
		Read(n);
		return n;
	}
	double ReadDouble(double n = 0.0) {
		Read(n);
		return n;
	}
 
////write
	template<typename T>
	bool Write(T n) {
		//����Ҫд�����ݵĴ�С
		auto nLen = sizeof(T);
		//�ж��ܲ���д��
		if (canWrite( nLen))
		{
			//��Ҫд������� ������������β��
			memcpy(_pBuff + _nWritePos, &n, nLen);
			//��������β��λ��
			push(nLen);
			return true;
		}
		CELLLog_Error("CELLStream::Write faild");
		return false;
	}
	template<typename T>
	bool WriteArray(T  * pData,uint32_t len) {
		//����Ҫд��������ֽڳ���
		auto nLen = sizeof(T)*len;
		//�ж��ܲ���д��
		if ( canWrite( nLen+sizeof(uint32_t)))
		{
			//д������ĳ���
			WriteInt32(len);
			//��Ҫд������� ������������β��
			memcpy(_pBuff + _nWritePos, pData, nLen);
			//��������β��λ��
			push(nLen);
			return true;
		}
		CELLLog_Error("error, CELLStream::WriteArray faild");
		return false;
	}

	bool WriteInt8(int8_t n) {
		return Write(n);
	}
	bool WriteInt16(int16_t n) {
		return Write(n);
	}
	bool WriteInt32(int32_t n) {
		return Write(n);
	}
	bool WriteFloat(float n) {
		return Write(n);
	}
	bool WriteDouble(double n) {
		return Write(n);
	}
private:
	//���ݻ�����
	char* _pBuff = nullptr;
	//�������ܵĿռ��С���ֽڳ���
	int _nSize = 0;

	//��д�����ݵ�β��λ��
	int _nWritePos = 0;
	//�Ѷ�ȡ���ݵ�β��λ��
	int _nReadPos = 0;

	//_pBuff���ⲿ���������ʱ�Ƿ�Ӧ�ñ��ͷ�
	bool _bDelete = true;

};

#endif // !_CELL_TASK_H_