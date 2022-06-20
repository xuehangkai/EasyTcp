#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_

#include<thread>
#include<mutex>
#include<list>
#include <functional>

#include"CELLThread.hpp"
//#include"CELLLog.hpp"

//ִ������ķ�������
class CellTaskServer {
public:
	int serverId = -1;
private:
	typedef std::function<void()> CellTask;
private:
	//��������
	std::list<CellTask> _tasks;
	//�������ݻ�����
	std::list<CellTask> _tasksBuf;
	//�ı����ݻ�������Ҫ����
	std::mutex _mutex;
	//
	CELLThread _thread;
public:
	//�������
	void addTask(CellTask task) {
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}
	//���������߳�
	void Start() {
		_thread.Start(nullptr,[this](CELLThread* pThread)->void{OnRun(pThread);}, nullptr);
	}
	void Close() {
		//CELLLog_Info("CellTaskServer<%d>.Close begin\n", serverId);
		_thread.Close();
		//CELLLog_Info("CellTaskServer<%d>.Close end\n", serverId);
	}
protected:
	//��������
	void OnRun(CELLThread* pThread) {
		while (pThread->isRun())
		{
			//�ӻ�������ȡ������
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf) {
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}
			//���û������
			if (_tasks.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//��������
			for (auto pTask : _tasks) {
				pTask();
			}
			//�������
			_tasks.clear();
		}

		//���������������
		for (auto pTask:_tasksBuf) {
			pTask();
		}

		//CELLLog_Info("CellTaskServer<%d>.OnRun exit\n", serverId);
	}
};

#endif // !_CELL_TASK_H_