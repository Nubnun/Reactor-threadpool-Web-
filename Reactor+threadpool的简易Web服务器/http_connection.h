#pragma once

#include<sys/socket.h>
#include<arpa/inet.h>
#include<memory>
#include<unistd.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<string.h>
#include<sys/epoll.h>
#include<sys/stat.h>
#include"http_connecttion_all.h"
using namespace std;


class http_connection
{
public:
	http_connection(int sockfd, const sockaddr_in address);
	~http_connection();
	void init();
	void close_connection();
	void process();
	bool read();
	bool write();
private:
	HTTP_REPLY_CODE process_read();//分析请求内容
	bool process_write();//填充应答
	
	//被process_read调用
	HTTP_REPLY_CODE parse_requestline(char* text);
	HTTP_REPLY_CODE parse_header(char* text);
	HTTP_REPLY_CODE parse_content(char* text);
	LINE_STATE parse_line();
	HTTP_REPLY_CODE do_request();

	//被process_write调用
	void unmap();
	

public:
	typedef shared_ptr<http_connection> ptr;
	static int user_num;
	static int epoll_fd;

private:
	int connect_fd;
	sockaddr_in m_address;
	struct read_buffer read_Buffer;//读缓冲区
	struct write_buffer write_Buffer;//写缓冲区
	struct analysy_info analysy_result;//分析请求的结果
	CHECK_STATE check_state;
	METHOD method;
	char* file_address;//请求的文件在内存映射区的位置
	struct stat file_stat;// 请求目标文件状态
	struct iovec iv[2];//两块内存块
	int iv_num;//内存块数量
};

