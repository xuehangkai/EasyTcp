#ifndef _CELL_MSG_STREAM_HPP_
#define _CELL_MSG_STREAM_HPP_


#include"MessageHeader.hpp"
#include"CELLStream.hpp"
//消息数据字节流
class CELLRecvStream :public CELLStream
{
public:
	CELLRecvStream(netmsg_DataHeader* header)
		:CELLStream((char*)header, header->dataLength)
	{
		push(header->dataLength);
		ReadInt16();
		getNetCmd();
	}
	uint16_t getNetCmd() {
		uint16_t cmd = CMD_ERROR;
		Read<uint16_t>(cmd);
		return cmd;
	}
};

//消息数据字节流
class CELLSendStream :public CELLStream
{
public:
	CELLSendStream(char* pData, int nSize, bool bDelete = false)
		:CELLStream(pData, nSize, bDelete)
	{
		//预先占领消息长度所需的空间
		Write<uint16_t>(0);
	}
	CELLSendStream(int nSize = 1024)
		:CELLStream(nSize)
	{
		//预先占领消息长度所需的空间
		Write<uint16_t>(0);
	}

	void setNetCmd(uint16_t cmd) {
		Write<uint16_t>(cmd);
	}

	bool WriteString(const char* str,int len) {
		return WriteArray(str, len);
	}
	bool WriteString(const char* str) {
		return WriteArray(str,strlen(str));
	}
	bool WriteString(std::string& str) {
		return WriteArray(str.c_str(), str.length());
	}

	void finsh() {
		int pos = length();
		setWritePos(0);
		Write<uint16_t>(pos);
		setWritePos(pos);
	}
public:

};

#endif // !_CELL_TASK_H_