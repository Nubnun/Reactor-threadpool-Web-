#pragma once
#include<list>
#include<mutex>
#include<pthread.h>
#include<condition_variable>


using namespace std;

template<class T>
class threadpool {

public:
	threadpool(int max_thread, int max_task);
	bool add_task(T* new_task);
	~threadpool();
private:
	static void* work(void* arg);

private:
	int max_thread;
	int max_task;
	list<T*> task_q;
	pthread_t* workthread;
	mutex thread_mutex;
	condition_variable_any m_notEmpty;
	condition_variable_any m_notFull;
	bool server_stop;
};
