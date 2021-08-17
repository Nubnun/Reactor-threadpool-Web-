#include"threadpool.h"

template<class T>
threadpool<T>::threadpool(int max_thread, int max_task)
{
	this->max_thread = max_thread;
	this->max_task = max_task;
	if (this->max_thread <= 0 || this->max_task <= 0) {
		throw exception();
	}
	server_stop = false;
	workthread = new pthread_t[this->max_thread];
	if (!workthread)
		throw exception();
	for (int i = 0; i < max_thread; i++) {
		if (pthread_create(&workthread[i], NULL, work, this) != 0){
			delete[]workthread;
			throw exception();
		}
		if (pthread_detach(workthread[i]) != 0) {
			delete[]workthread;
			throw exception();
		}
	}
}

template<class T>
bool threadpool<T>::add_task(T* new_task)
{
	lock_guard<mutex> locker(thread_mutex);
	if (task_q.size() > max_task) {
		return false;
	}
	task_q.push_back(new_task);
	m_notEmpty.notify_one();
	return true;
}

template<class T>
threadpool<T>::~threadpool()
{
	server_stop = true;
	delete[]workthread;
}

template<class T>
void* threadpool<T>::work(void* arg)
{
	threadpool* pool = static_cast<threadpool*>(arg);
	while (!pool -> server_stop) {
		{
			lock_guard<mutex> locker(pool->thread_mutex);
			pool->m_notEmpty.wait(pool->thread_mutex);
			T* request = pool->task_q.front();
			if (request)
				request.process();
		}
	}
	return nullptr;
}
