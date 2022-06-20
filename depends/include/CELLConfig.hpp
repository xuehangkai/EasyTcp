#ifndef _CELL_CONFIG_HPP
#define _CELL_CONFIG_HPP

//专门用于读取配置数据
//目前我们的配置参数主要来源于main函数的args传入

#include<string>
#include<map>

#include"CELLLog.hpp"


class CELLConfig
{
private:
	CELLConfig() {

	}

	~CELLConfig() {

	}

public:
	static CELLConfig& Instance() {
		static CELLConfig obj;
		return obj;
	}
	void Init(int argc,char* args[]) {
		_exePath = args[0];
		for (int n = 1; n < argc;n++) {
			//CELLLog_Debug("arg=%s\n",args[n]);
			madeKV(args[n]);
		}
	}

	const char* getStr(const char* argName, const char* def) {
		auto itr = _kv.find(argName);
		if (itr==_kv.end()) {
			CELLLog_Error("CELLConfig::getStr not find , argNmae=%s\n",argName);
		}
		else {
			def = itr->second.c_str();
		}
		CELLLog_Info("CELLConfig::getStr, %s=%s\n", argName, def);
		return def;
	}

	int getInt(const char* argName, int def) {
		auto itr = _kv.find(argName);
		if (itr == _kv.end()) {
			CELLLog_Error("CELLConfig::getInt not find , argNmae=%s\n", argName);
		}
		else {
			def = atoi(itr->second.c_str());
		}
		CELLLog_Info("CELLConfig::getInt, %s=%d\n", argName, def);
		return def;
	}

	bool hasKey(const char* key) {
		auto itr = _kv.find(key);
		return itr != _kv.end();
	}

	void madeKV(char* cmd) {
		char* val = strchr(cmd, '=');
		if (val) {
			*val = '\0';
			val++;
			_kv[cmd] = val;
			CELLLog_Debug("madeKV k<%s> | v<%s>\n",cmd,val);
		}
		else {
			_kv[cmd] = "";
			CELLLog_Debug("madeKV k<%s>\n", cmd);
		}
	}

private:
	//当前程序的路径
	std::string _exePath;
	//存储传入的key-val型数据
	std::map<std::string, std::string> _kv;
};

#endif // !_CELL_NET_WORK_HPP
