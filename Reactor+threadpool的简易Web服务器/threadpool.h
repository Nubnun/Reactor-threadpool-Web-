#pragma once
#ifndef THREADPOOL_H
#define THREADPOOL_H


#include<list>
#include<mutex>
#include<pthread.h>
#include<condition_variable>


using namespace std;

template<class T>
class threadpool {

public:
	threadpool(int max_thread = 8, int max_task = 10000);
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

#endif