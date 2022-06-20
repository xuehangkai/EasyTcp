#include"EasyTcpClient.hpp"
#include"CELLTimestamp.hpp"
#include<thread>
#include<atomic>

class MyClient:public EasyTcpClient
{
public:
	//响应网络消息
	virtual void OnNetMsg(netmsg_DataHeader* header) {
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

};

bool g_bRun = true;
void cmdThread() {
	while (g_bRun) {
		char cmdBuf[256] = {};
#ifdef _WIN32
		scanf_s("%s", cmdBuf, 256);
#else
		scanf("%s", cmdBuf);
#endif
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			CELLLog_Info("退出cmdThread线程\n");
			break;
		}
		else
		{
			CELLLog_Info("不支持的命令\n");
		}
	}
}

//客户端数量
const int cCount = 10000;
//发送线程数量
const int tCount = 1;
//客户端数组
EasyTcpClient* client[cCount];

std::atomic_int sendCount = 0;
std::atomic_int readyCount = 0;

void recvThread(int begin,int end) {
	CELLTimestamp t;
	while (g_bRun) {
		//client.OnSelect();
		//client.SendData(&login);
		for (int n = begin; n < end; n++) {
			//if (t.getElapsedSecond() > 3.0 && n == begin)
				//continue;
			client[n]->OnSelect();
		}
	}
}

void sendThread(int id) {
	CELLLog_Info("thread<%d>, start!\n", id);
	int c= cCount / tCount;
	int begin = (id-1)*c;
	int end = id* c;
	for (int n = begin; n < end; n++) {
		client[n] = new MyClient();
	}
	for (int n = begin; n < end; n++) {
		client[n]->Connect("192.168.31.247", 4567);
		//client[n]->Connect("127.0.0.1", 4567);
		//CELLLog_Info("thread<%d>, Connect=%d\n",id, n);
	}

	//EasyTcpClient client;
	//client.initSocket();
	//client.Connect("127.0.0.1",4567);
	CELLLog_Info("thread<%d>, Connect<begin=%d, end=%d>\n", id, begin,end);

	readyCount++;
	while (readyCount<tCount) {
		//等待其他线程准备好发送数据
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	//
	std::thread t1(recvThread,begin,end);
	t1.detach();

	//
	netmsg_Login login[100];
	for (int n = 0; n <100  ;n++) {
#ifdef _WIN32
		strcpy_s(login[n].userName, "xhk");
		strcpy_s(login[n].passWord, "xhk123");
#else
		strcpy(login[n].userName, "xhk");
		strcpy(login[n].passWord, "xhk123");
#endif
	}
	const int nLen = sizeof(login);
	while (g_bRun) {
		//client.OnSelect();
		//client.SendData(&login);
		for (int n = begin; n < end; n++) {
			if (SOCKET_ERROR !=client[n]->SendData(login)) {
				sendCount++; 
			}
		}
		std::chrono::milliseconds t(99);
		std::this_thread::sleep_for(t);
	}
	//client.Close();
	for (int n = 0; n < cCount; n++) {
		client[n]->Close();
		delete client[n];
	}
	CELLLog_Info("thread<%d>, exit!\n", id);
}

int main() {
	CELLLog::Instance().setLogPath("clientLog", "w");
	//启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	//启动发送线程
	for (int n = 0; n < tCount; n++) {
		std::thread t1(sendThread,n+1);
		t1.detach();
	}
	CELLTimestamp tTime;
	while (g_bRun) {
		auto t = tTime.getElapsedSecond();
		if (t>=1.0) {
			CELLLog_Info("thread<%d>, client<%d>, time<%lf>, send<%d>\n",tCount, cCount,t,(int)(sendCount/t));
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}
	CELLLog_Info("已退出。\n");
	return 0;
}