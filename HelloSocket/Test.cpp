#include"EasyTcpClient.hpp"
#include"CELLMsgStream.hpp"
class MyClient :public EasyTcpClient
{
public:
	//响应网络消息
	virtual void OnNetMsg(netmsg_DataHeader* header) {
		switch (header->cmd)
		{
		case CMD_LOGINOUT_RESULT:
		{
			CELLRecvStream r(header);
			auto n1 = r.ReadInt8();
			auto n2 = r.ReadInt16();
			auto n3 = r.ReadInt32();
			auto n4 = r.ReadFloat();
			auto n5 = r.ReadDouble();

			uint32_t n = 0;
			r.onlyRead(n);

			char name[32] = {};
			auto n6 = r.ReadArray(name, 32);
			char pw[32] = {};
			auto n7 = r.ReadArray(pw, 32);
			int data[10] = {};
			auto n8 = r.ReadArray(data, 10);
			netmsg_LoginR* login = (netmsg_LoginR*)header;
			//CELLLog_Info("收到服务器返回消息：CMD_LOGIN_RESULT，数据长度：%d\n", loginResult->dataLength);
		}
		break;
		case CMD_ERROR:
		{
			CELLLog_Info("<socket=%d>收到服务器返回消息：CMD_ERROR，数据长度：%d\n", (int)_pClient->getSockfd(), header->dataLength);
		}
		break;
		default:
		{
			CELLLog_Error("收到<cSocket=%d>未定义消.\n", (int)_pClient->getSockfd());
		}
		}
	}

};

int main(){
	CELLSendStream s;
	s.setNetCmd(CMD_LOGINOUT);
	s.WriteInt8(1);
	s.WriteInt16(2);
	s.WriteInt32(3);
	s.WriteFloat(4.5f);
	s.WriteDouble(6.7);
	s.WriteString("client");
	char a[] = "ahah";
	s.WriteArray(a, strlen(a));
	int b[] = {1,2,3,4,5};
	s.WriteArray(b, 5);
	s.finsh();
	/*auto n1 = s.ReadInt8();
	auto n2 = s.ReadInt16();
	auto n3 = s.ReadInt32();
	auto n4 = s.ReadFloat();
	auto n5 = s.ReadDouble();

	uint32_t n = 0;
	s.onlyRead(n);

	char name[32] = {};
	auto n6 = s.ReadArray(name, 32);
	char pw[32] = {};
	auto n7 = s.ReadArray(pw, 32);
	int data[10] = {};
	auto n8 = s.ReadArray(data, 10);*/

	MyClient client;
	client.Connect("192.168.31.247",4567);
	client.SendData(s.data(),s.length());
	while (client.isRun())
	{
		client.OnRun();
		CELLThread::Sleep(10);
	}

	return 0;
}