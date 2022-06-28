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
#define CELLLog_PError(...) CELLLog::pError(__VA_ARGS__)
	

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
	void setLogPath(const char* logName,const char* mode,bool hasDate) {
		if (_LogFile) {
			Info("CELLLog::setLogPath _LogFile !=nullptr fclose\n");
			fclose(_LogFile);
			_LogFile = nullptr;
		}
		static char logPath[256] = {};
		if (_LogFile) {
			auto t = system_clock::now();
			auto  tNow = system_clock::to_time_t(t);
			//fprintf(pLog->_LogFile, "%s", ctime(&tNow));
			std::tm* now = std::localtime(&tNow);
			sprintf(logPath, "%s[%d-%d-%d_%d-%d-%d].txt", logName, now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
		}
		else {
			sprintf(logPath, "%s.txt", logName);
		}
		_LogFile = fopen(logPath, mode);
		if (_LogFile) {
			Info("CELLLog::setLogPath success,<%s,%s>\n", logPath,mode);
		}
		else {
			Info("CELLLog::setLogPath failed,<%s,%s>\n", logPath, mode);
		}
	}

	static void  pError(const char* pStr) {
		pError("%s", pStr);
	}

	template<typename ...Args>
	static void  pError(const char* pformat, Args...args) {
#ifdef _WIN32
		auto errCode = GetLastError();
		Instance()._taskServer.addTask([=]() {
			char text[256] = {};
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				errCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&text,
				256,
				NULL
			);
			EchoReal(true,"pError ", pformat,args...);
			EchoReal(false,"pError ", "errno=%d,errmsg=%s", errCode, text);
		});
#else
		auto errCode = errno;
		Instance()._taskServer.addTask([=]() {
			EchoReal(true, "pError ", pformat, args...);
			EchoReal(true, "pError ", "errno=%d,errmsg=%s\n", errCode, strerror(errCode));
		});
#endif
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
			EchoReal(true,type,pformat,args...);
		});
	}

	template<typename ...Args>
	static void  EchoReal(bool br,const char* type, const char* pformat, Args...args) {
		CELLLog* pLog = &Instance();
		if (pLog->_LogFile) {
			auto t = system_clock::now();
			auto  tNow = system_clock::to_time_t(t);
			//fprintf(pLog->_LogFile, "%s", ctime(&tNow));
			std::tm* now = std::localtime(&tNow);
			if (type) {
				fprintf(pLog->_LogFile, "%s", type);
			}
			fprintf(pLog->_LogFile, "[%d-%d-%d %d:%d:%d] ", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
			fprintf(pLog->_LogFile, pformat, args...);
			if (br) {

			}
			fflush(pLog->_LogFile);
		}
		if (type) {
		
		}
		printf(pformat, args...);
		if (br) {

		}
	}
private:
	FILE* _LogFile = nullptr;
	CellTaskServer _taskServer;
};



#endif // !_CELL_LOG_HPP_