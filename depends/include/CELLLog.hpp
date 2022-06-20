#ifndef _CELL_LOG_HPP_
#define _CELL_LOG_HPP_

//#include"Cell.hpp"
#include"CELLTask.hpp"
#include<ctime>
class CELLLog
{
#if _DEBUG
	#ifndef CELLLog_Debug
		#define CELLLog_Debug(...) CELLLog::Debug(__VA_ARGS__)
	#endif
#else
	#ifndef CELLLog_Debug
		#define CELLLog_Debug(...)
	#endif
#endif

#define CELLLog_Info(...) CELLLog::Info(__VA_ARGS__)
#define CELLLog_Warring(...) CELLLog::Warring(__VA_ARGS__)
#define CELLLog_Error(...) CELLLog::Error(__VA_ARGS__)

	

private:
	CELLLog() {
		_taskServer.Start();
	}
	~CELLLog() {
		_taskServer.Close();
		if (_LogFile) {
			Info("CELLLog  fclose(_LogFile)\n");
			fclose(_LogFile);
			_LogFile = nullptr;
		}
	}

public:
	static CELLLog& Instance() {
		static CELLLog sLog;
		return sLog;
	}
	void setLogPath(const char* logName,const char* mode) {
		if (_LogFile) {
			Info("CELLLog::setLogPath _LogFile !=nullptr fclose\n");
			fclose(_LogFile);
			_LogFile = nullptr;
		}
		static char logPath[256] = {};

		auto t = system_clock::now();
		auto  tNow = system_clock::to_time_t(t);
		//fprintf(pLog->_LogFile, "%s", ctime(&tNow));
		std::tm* now = std::localtime(&tNow);

		sprintf(logPath,"%s[%d-%d-%d_%d-%d-%d].txt",logName, now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
		_LogFile = fopen(logPath, mode);
		if (_LogFile) {
			Info("CELLLog::setLogPath success,<%s,%s>\n", logPath,mode);
		}
		else {
			Info("CELLLog::setLogPath failed,<%s,%s>\n", logPath, mode);
		}
	}
	//Info
	//Debug
	//Warring
	//Error
	static void  Error(const char * pStr) {
		Error("%s", pStr);
	}
	static void  Warring(const char* pStr) {
		Warring("%s", pStr);
	}
	static void  Debug(const char* pStr) {
		Debug("%s", pStr);
	}
	static void  Info(const char* pStr) {
		Info("%s", pStr);
	}

	template<typename ...Args>
	static void  Info(const char* pformat, Args...args) {

		Echo("Info ", pformat, args...);
	}
	template<typename ...Args>
	static void  Debug(const char* pformat, Args...args) {

		Echo("Debug ", pformat, args...);
	}
	template<typename ...Args>
	static void  Warring(const char* pformat, Args...args) {

		Echo("Warring ", pformat, args...);
	}
	template<typename ...Args>
	static void  Error(const char* pformat, Args...args) {

		Echo("Error ", pformat, args...);
	}

	template<typename ...Args>
	static void  Echo(const char* type, const char* pformat,Args...args) {
		CELLLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_LogFile) {
				auto t = system_clock::now();
				auto  tNow = system_clock::to_time_t(t);
				//fprintf(pLog->_LogFile, "%s", ctime(&tNow));
				std::tm* now = std::localtime(&tNow);
				fprintf(pLog->_LogFile, "%s", type);
				fprintf(pLog->_LogFile, "[%d-%d-%d %d:%d:%d] ", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour , now->tm_min, now->tm_sec);
				fprintf(pLog->_LogFile, pformat, args...);
				fflush(pLog->_LogFile);
			}
			printf(pformat, args...);
		});
	}

private:
	FILE* _LogFile = nullptr;
	CellTaskServer _taskServer;
};



#endif // !_CELL_LOG_HPP_