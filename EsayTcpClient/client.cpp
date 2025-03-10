﻿//#include"EasySelectClient.hpp"
#include"EasyIOCPClient.hpp"
#include"CELLTimestamp.hpp"
#include"CELLConfig.hpp"

#include<atomic>
#include<list>
#include<vector>

//服务端IP地址
const char* strIP = "127.0.0.1";
//服务端端口
uint16_t nPort = 4567;
//发送线程数量
int nThread = 1;
//客户端数量
int nClient = 1;
/*
::::::数据会先写入发送缓冲区
::::::等待socket可写时才实际发送
::每个客户端在nSendSleep(毫秒)时间内
::最大可写入nMsg条Login消息
::每条消息100字节（Login）
*/
//客户端每次发几条消息
int nMsg = 1;
//写入消息到缓冲区的间隔时间
int nSendSleep = 1;
//工作休眠时间
int nWorkSleep = 1;
//客户端发送缓冲区大小
int nSendBuffSize = SEND_BUFF_SZIE;
//客户端接收缓冲区大小
int nRecvBuffSize = RECV_BUFF_SZIE;
//
bool bChekSendBack = true;

class MyClient:public EasyIOCPClient
{
public:

	MyClient()
	{
		_bCheckMsgID = CELLConfig::Instance().hasKey("-checkMsgID");
	}
	//响应网络消息
	virtual void OnNetMsg(netmsg_DataHeader* header) {
		_bSend = false;
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			netmsg_LoginR* loginResult = (netmsg_LoginR*)header;
			//CELLLog_Info("收到服务器返回消息：CMD_LOGIN_RESULT，数据长度：%d\n", loginResult->dataLength);
		}
		break;
		case CMD_LOGINOUT_RESULT:
		{
			netmsg_LoginoutR* loginoutResult = (netmsg_LoginoutR*)header;
			//CELLLog_Info("收到服务器返回消息：CMD_LOGINOUT_RESULT，数据长度：%d\n", loginoutResult->dataLength);
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			netmsg_NewUserJoin* userJoin = (netmsg_NewUserJoin*)header;
			//CELLLog_Info("收到服务器返回消息：CMD_NEW_USER_JOIN，数据长度：%d\n", userJoin->dataLength);
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

	int SendTest(netmsg_Login* login)
	{
		int ret = 0;
		//如果剩余发送次数大于0
		if (_nSendCount > 0&& !_bSend)
		{
			login->msgID = _nSendMsgID;
			ret = SendData(login);
			if (SOCKET_ERROR != ret)
			{
				_bSend= bChekSendBack;
				++_nSendMsgID;
				//如果剩余发送次数减少一次
				--_nSendCount;
			}
		}
		return ret;
	}

	bool checkSend(time_t dt)
	{
		_tRestTime += dt;
		CELLLog_Info("_tRestTime<%d>\n", _tRestTime);
		CELLLog_Info("nSendSleep<%d>\n", nSendSleep);
		//每经过nSendSleep毫秒
		if (_tRestTime >= nSendSleep)
		{
			//重置计时
			_tRestTime -= nSendSleep;
			//重置发送计数
			_nSendCount = nMsg;
		}
		return _nSendCount > 0;
	}
public:
	//发送时间计数
	time_t _tRestTime = 0;
private:
	//接收消息id计数
	int _nRecvMsgID = 1;
	//发送消息id计数
	int _nSendMsgID = 1;
	//发送条数计数
	int _nSendCount = 0;
	//检查接收到的服务端消息ID是否连续
	bool _bCheckMsgID = false;
	//
	bool _bSend = false;
};

std::atomic_int sendCount(0);
std::atomic_int readyCount(0);
std::atomic_int nConnect(0);

void WorkThread(CELLThread* pThread, int id) {
	//n个线程 id值为 1~n
	CELLLog_Info("thread<%d>,start\n", id);
	//客户端数组
	std::vector<MyClient*> clients(nClient);
	//计算本线程客户端在clients中对应的index
	int m = 0;
	int s= nThread - nClient % nThread;
	if (s== nThread) {
		m = 0;
	}
	else
	{
		if (s<=id) {
			m = 1;
		}
	}

	int begin = 0;
	int end = nClient/nThread+m;
	if (end<1) {
		end = 1;
	}
	int nTemp1 = nSendSleep > 0? nSendSleep:1;
	for (int n = begin; n < end; n++)
	{
		if (!pThread->isRun())
			break;
		clients[n] = new MyClient();
		clients[n]->_tRestTime = n % nTemp1;
		//多线程时让下CPU
		CELLThread::Sleep(0);
	}
	for (int n = begin; n < end; n++)
	{
		if (!pThread->isRun())
			break;
		if (INVALID_SOCKET == clients[n]->InitSocket(nSendBuffSize, nRecvBuffSize))
			break;
		if (SOCKET_ERROR == clients[n]->Connect(strIP, nPort))
			break;
		nConnect++;
		CELLThread::Sleep(0);
	}
	//所有连接完成
	CELLLog_Info("thread<%d>,Connect<begin=%d, end=%d ,nConnect=%d>\n", id, begin, end, (int)nConnect);

	readyCount++;
	while (readyCount < nThread && pThread->isRun())
	{//等待其它线程准备好，再发送数据
		CELLThread::Sleep(10);
	}

	//消息
	netmsg_Login login;
	//给点有意义的值
	strcpy(login.userName, "xhk");
	strcpy(login.passWord, "xhk123");
	//
	//收发数据都是通过onRun线程
	//SendData只是将数据写入发送缓冲区
	//等待select检测可写时才会发送数据
	//旧的时间点
	auto t2 = CELLTime::getNowInMilliSec();
	//新的时间点
	auto t0 = t2;
	//经过的时间
	auto dt = t0;
	CELLTimestamp tTime;
	while (pThread->isRun())
	{
		t0 = CELLTime::getNowInMilliSec();
		dt = t0 - t2;
		t2 = t0;
		//本次while (pThread->isRun())循环主要工作内容
		//代号work
		{
			int count = 0;
			//每轮每个客户端发送nMsg条数据
			for (int m = 0; m < nMsg; m++)
			{
				//每个客户端1条1条的写入消息
				for (int n = begin; n < end; n++)
				{
					if (clients[n]->isRun())
					{
						if (clients[n]->SendTest(&login) > 0)
						{
							++sendCount;
						}
					}
				}
			}
			//sendCount += count;
			for (int n = begin; n < end; n++)
			{
				if (clients[n]->isRun())
				{	//超时设置为0表示select检测状态后立即返回
					if (!clients[n]->OnRun(0))
					{	//连接断开
						nConnect--;
						continue;
					}
					//检测发送计数是否需要重置
					clients[n]->checkSend(dt);
				}
			}
		}
		CELLThread::Sleep(nWorkSleep);
	}
	//--------------------------
	//关闭消息收发线程
	//tRun.Close();
	//关闭客户端
	for (int n = begin; n < end; n++)
	{
		clients[n]->Close();
		delete clients[n];
	}
	CELLLog_Info("thread<%d>,exit", id);
	--readyCount;
}


int main(int argc, char* args[]) {
	CELLLog::Instance().setLogPath("clientLog", "w",false);

	CELLConfig::Instance().Init(argc,args);
	strIP = CELLConfig::Instance().getStr("strIP", "127.0.0.1");
	nPort = CELLConfig::Instance().getInt("nPort", 4567);
	nThread = CELLConfig::Instance().getInt("nThread", 1);
	nClient = CELLConfig::Instance().getInt("nClient", 10000);
	nMsg = CELLConfig::Instance().getInt("nMsg", 10);
	nSendSleep = CELLConfig::Instance().getInt("nSendSleep", 100);
	nWorkSleep = CELLConfig::Instance().getInt("nWorkSleep", 1);
	bChekSendBack= CELLConfig::Instance().hasKey("-chekSendBack");
	nSendBuffSize = CELLConfig::Instance().getInt("nSendBuffSize", SEND_BUFF_SZIE);
	nRecvBuffSize = CELLConfig::Instance().getInt("nRecvBuffSize", RECV_BUFF_SZIE);

	//启动终端命令线程
	//用于接收运行时用户输入的指令
	CELLThread tCmd;
	tCmd.Start(nullptr, [](CELLThread* pThread) {
		while (true)
		{
			char cmdBuf[256] = {};
			scanf("%s", cmdBuf);
			if (0 == strcmp(cmdBuf, "exit"))
			{
				//pThread->Exit();
				CELLLog_Info("退出cmdThread线程\n");
				break;
			}
			else {
				CELLLog_Info("不支持的命令。\n");
			}
		}
	});


	//启动模拟客户端线程
	std::vector<CELLThread*> threads;
	for (int n = 0; n < nThread; n++)
	{
		CELLThread* t = new CELLThread();
		t->Start(nullptr, [n](CELLThread* pThread) {
			WorkThread(pThread, n + 1);
		});
		threads.push_back(t);
	}

	//每秒数据统计
	CELLTimestamp tTime;
	while (tCmd.isRun())
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			CELLLog_Info("thread<%d>,clients<%d>,connect<%d>,time<%lf>,send<%d>\n", nThread, nClient, (int)nConnect, t, (int)sendCount);
			sendCount = 0;
			tTime.update();
		}
		//CELLThread::Sleep(1);
	}

	//
	for (auto t : threads)
	{
		t->Close();
		delete t;
	}

	CELLLog_Info("已退出。\n");
	return 0;
}