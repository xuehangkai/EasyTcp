#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGINOUT,
	CMD_LOGINOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR,
	CMD_C2S_HEART,
	CMD_S2C_HEART
};

struct netmsg_DataHeader
{
	netmsg_DataHeader() {
		dataLength = sizeof(netmsg_DataHeader);
		cmd = CMD_ERROR;
	}
	unsigned short dataLength;
	unsigned short cmd;
};

struct netmsg_Login : public netmsg_DataHeader
{
	netmsg_Login() {
		dataLength = sizeof(netmsg_Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char passWord[32];
	char data[28];
	int msgID;
};

struct netmsg_LoginR : public netmsg_DataHeader
{
	netmsg_LoginR() {
		dataLength = sizeof(netmsg_LoginR);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
	char data[88];
	int msgID;
};

struct netmsg_Loginout : public netmsg_DataHeader
{
	netmsg_Loginout() {
		dataLength = sizeof(netmsg_Loginout);
		cmd = CMD_LOGINOUT;
	}
	char userName[32];
};

struct netmsg_LoginoutR : public netmsg_DataHeader
{
	netmsg_LoginoutR() {
		dataLength = sizeof(netmsg_LoginoutR);
		cmd = CMD_LOGINOUT_RESULT;
		result = 0;
	}
	int result;
};

struct netmsg_NewUserJoin : public netmsg_DataHeader
{
	netmsg_NewUserJoin() {
		dataLength = sizeof(netmsg_NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

struct netmsg_c2s_Heart : public netmsg_DataHeader
{
	netmsg_c2s_Heart() {
		dataLength = sizeof(netmsg_c2s_Heart);
		cmd = CMD_C2S_HEART;
	}
};

struct netmsg_s2c_Heart : public netmsg_DataHeader
{
	netmsg_s2c_Heart() {
		dataLength = sizeof(netmsg_s2c_Heart);
		cmd = CMD_S2C_HEART;
	}
};
#endif