#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_

#include<thread>
#include<mutex>
#include<list>
#include <functional>

#include"CELLThread.hpp"
//#include"CELLLog.hpp"

//执行任务的服务类型
class CellTaskServer {
public:
	int serverId = -1;
private:
	typedef std::function<void()> CellTask;
private:
	//任务数据
	std::list<CellTask> _tasks;
	//任务数据缓冲区
	std::list<CellTask> _tasksBuf;
	//改变数据缓冲区需要加锁
	std::mutex _mutex;
	//
	CELLThread _thread;
public:
	//添加任务
	void addTask(CellTask task) {
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}
	//启动工作线程
	void Start() {
		_thread.Start(nullptr,[this](CELLThread* pThread)->void{OnRun(pThread);}, nullptr);
	}
	void Close() {
		//CELLLog_Info("CellTaskServer<%d>.Close begin\n", serverId);
		_thread.Close();
		//CELLLog_Info("CellTaskServer<%d>.Close end\n", serverId);
	}
protected:
	//工作函数
	void OnRun(CELLThread* pThread) {
		while (pThread->isRun())
		{
			//从缓冲区中取出数据
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf) {
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}
			//如果没有任务
			if (_tasks.empty()) {
				CELLThread::Sleep(1);
				continue;
			}
			//处理任务
			for (auto pTask : _tasks) {
				pTask();
			}
			//清空任务
			_tasks.clear();
		}

		//处理缓冲队列中任务
		for (auto pTask:_tasksBuf) {
			pTask();
		}

		//CELLLog_Info("CellTaskServer<%d>.OnRun exit\n", serverId);
	}
};

#endif // !_CELL_TASK_H_