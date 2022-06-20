﻿#include "EasyTcpServer.hpp"
#include"CELLMsgStream.hpp"
#include"CELLConfig.hpp"
//bool g_bRun = true;
//void cmdThread() {
//	while (g_bRun) {
//		char cmdBuf[256] = {};
//#ifdef _WIN32
//		scanf_s("%s", cmdBuf, 256);
//#else
//		scanf("%s", cmdBuf);
//#endif
//		if (0 == strcmp(cmdBuf, "exit")) {
//			g_bRun = false;
//			CELLLog_Info("退出cmdThread线程\n");
//			break;
//		}
//		else
//		{
//			CELLLog_Info("不支持的命令\n");
//		}
//	}
//}

class MyServer:public EasyTcpServer {
public: 
	//只会被一个线程调用 安全
	virtual void OnNetJoin(CellClient* pClient) {
		EasyTcpServer::OnNetJoin(pClient);
		//CELLLog_Info("client<%d> join\n", pClient->getSockfd());
	}

	//cellserver 4 多个线程触发不安全
	virtual void OnNetLeave(CellClient* pClient) {
		EasyTcpServer::OnNetLeave(pClient);
		//CELLLog_Info("client<%d> leave\n", pClient->getSockfd());
	}

	//cellserver 4 多个线程触发不安全
	virtual void OnNetMsg(CellServer* pCellServer,CellClient* pClient, netmsg_DataHeader* header) {
		EasyTcpServer::OnNetMsg(pCellServer,pClient,header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			pClient->resetDTHeart();
			netmsg_Login* login = (netmsg_Login*)header;
			//CELLLog_Info("收到客户端<Socket=%d>请求:CMD_LOGIN,数据长度：%d,username: %s,password: %s\n", (int)cSocket, login->dataLength, login->userName, login->passWord);
			netmsg_LoginR ret ;
			if (SOCKET_ERROR==pClient->SendData(&ret)) {

				//发送缓冲区满了，消息没发出去
				CELLLog_Info("recv <socker=%d> Send Full \n", pClient->getSockfd()); 
			}
			//netmsg_LoginR* ret=new netmsg_LoginR();

			//pCellServer->addSendTask(pClient,ret);

		}
		break;
		case CMD_LOGINOUT:
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

			///
			CELLSendStream s;
			s.setNetCmd(CMD_LOGINOUT_RESULT);
			s.WriteInt8(1);
			s.WriteInt16(2);
			s.WriteInt32(3);
			s.WriteFloat(4.5f);
			s.WriteDouble(6.7);
			const char* str = "server";
			s.WriteArray(str, strlen(str));
			char a[] = "ahah";
			s.WriteArray(a, strlen(a));
			int b[] = { 1,2,3,4,5 };
			s.WriteArray(b, 5);
			s.finsh();
			pClient->SendData(s.data(),s.length());
			//netmsg_Loginout* loginout = (netmsg_Loginout*)header;
			////CELLLog_Info("收到客户端<Socket=%d>请求:CMD_LOGINOUT,数据长度：%d,username: %s\n", (int)cSocket, loginout->dataLength, loginout->userName);
			//netmsg_LoginoutR oret;
			////SendData(cSocket, &oret);
		}
		break;
		case CMD_C2S_HEART: {

			pClient->resetDTHeart();
			netmsg_s2c_Heart ret;
			pClient->SendData(&ret);
		}
		default:
		{
			CELLLog_Info("收到<cSocket=%d>未定义消息，数据长度：%d\n", (int)pClient->getSockfd(), header->dataLength);
			netmsg_DataHeader header;
			//SendData(cSocket,&header);
		}
		break;
		}
	}

	virtual void OnNetRecv(CellClient* pClient) {
		EasyTcpServer::OnNetRecv(pClient);
	}

private:
};

int main(int argc,char * args[]) {

	CELLLog::Instance().setLogPath("serverLog", "w");

	CELLConfig::Instance().Init(argc,args);
	const char* strIP = CELLConfig::Instance().getStr("strIP", "any");
	uint16_t nPort = CELLConfig::Instance().getInt("nPort", 4567);
	int nThread = CELLConfig::Instance().getInt("nThread", 1);
	int nClient = CELLConfig::Instance().getInt("nClient", 1);

	if (CELLConfig::Instance().hasKey("-p")) {
		CELLLog_Info("hasKey -p\n");
	}

	if (strcmp(strIP,"any")==0) {
		strIP = nullptr;
	}

	MyServer server;
	server.iniSocket();
	server.Bind(strIP, nPort);
	server.Listen(64);
	server.Start(nThread);

	//启动UI线程
	//std::thread t1(cmdThread);
	//t1.detach();

	while (true) {
		char cmdBuf[256] = {};
#ifdef _WIN32
		scanf_s("%s", cmdBuf, 256);
#else
		scanf("%s", cmdBuf);
#endif
		if (0 == strcmp(cmdBuf, "exit")) {
			server.Close();
			CELLLog_Info("退出cmdThread线程\n");
			break;
		}
		else
		{
			CELLLog_Info("不支持的命令\n");
		}
	}

	/*CellTaskServer task;
	task.Start();
		Sleep(100);
	task.Close();*/

//#ifdef _WIN32
//	while (true)
//		Sleep(10);
//#endif
	return 0;
}