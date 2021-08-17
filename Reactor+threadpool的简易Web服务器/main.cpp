#include<signal.h>
#include<memory>
#include <cassert>


#include"http_connection.h"
#include"threadpool.h"

const int MAX_USER = 65536;
const int MAX_EVENT_NUM = 10000;


using namespace std;

extern int addfd(int epoll_fd, int fd, bool one_shot);
extern int rm_fd(int epoll_fd, int fd);

void add_sig(int sig, void(handle)(int), bool restart = true) {
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handle;
	if (restart)
		sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);

}

void show_error(int connect_fd, const char* info) {
	cout << info << endl;
	send(connect_fd, info, strlen(info), 0);
	close(connect_fd);
}


int main(int argc, char* argv[]) {
	if (argc <= 2) {
		printf("usage: %s ip_address port_number\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	
	//忽略sigpipe信号
	add_sig(SIGPIPE, SIG_IGN);

	//创建线程池
	threadpool<http_connection>* pool = NULL;
	try 
	{
		pool = new threadpool<http_connection>;
	}
	catch (...)
	{
		return 1;
	}
	http_connection* user = new http_connection[MAX_USER];
	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listen_fd >= 0);
	struct linger tmp = { 1, 0 };
	//设置描述符so_linger选项，连接关闭前，发送完缓冲区的数据；
	setsockopt(listen_fd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

	struct sockaddr_in address;
	memset(&address, '\0', sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int ret = bind(listen_fd, (struct sockaddr*)&address, sizeof(address));
	assert(ret >= 0);

	ret = listen(listen_fd, 5);
	assert(ret >= 0);

	epoll_event event[MAX_EVENT_NUM];

	int epollfd = epoll_create(1);
	assert(epollfd >= 0);
	addfd(epollfd, listen_fd, false);
	http_connection::epoll_fd = epollfd;

	while (1) {
		int num = epoll_wait(epollfd, event, MAX_EVENT_NUM, -1);
		if (num < 0 && errno != EINTR) {
			cout << "epoll failure" << endl;
			break;
		}
		for (int i = 0; i < num; i++) {
			int sock_fd = event[i].data.fd;
			if (sock_fd == listen_fd) {
				struct sockaddr_in client_address;
				socklen_t n = sizeof(client_address);
				int connect_fd = accept(listen_fd, (struct sockaddr*)&client_address, &n);
				if (http_connection::user_num >= MAX_USER) {
					show_error(connect_fd, "Internal server busy");
					continue;
				}
				user->init(sock_fd, client_address);
			}
			else if (event[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
				user->close_connection();
			}
			else if (event[i].events & EPOLLIN) {
				if (user[sock_fd].read())
					pool->add_task(user + sock_fd);
				else
					user[sock_fd].close_connection();
			}
			else if (event[i].events & EPOLLOUT) {
				if (!user[sock_fd].write())
					user[sock_fd].close_connection();
			}
		}
	}
	close(epollfd);
	close(listen_fd);
	delete pool;
	delete user;
	return 0;
}