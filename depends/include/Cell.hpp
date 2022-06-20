#ifndef _CELL_HPP_
#define _CELL_HPP_

//SOCKET
#ifdef _WIN32
	#define FD_SETSIZE	10240
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS 
	#include<Windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h>// uni std
	#include<arpa/inet.h>
	#include<string.h>
	#include<signal.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR		(-1)
#endif


//�Զ���
#include"MessageHeader.hpp"	
#include"CELLTimestamp.hpp"
#include"CELLTask.hpp"
#include"CELLLog.hpp"

//
#include<stdio.h>

//��������С��Ԫ��С
#ifndef RECV_BUFF_SZIE
	#define RECV_BUFF_SZIE 8192
	#define SEND_BUFF_SZIE 10240
#endif
#endif // !_Cell_hpp_


