#pragma once
#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H


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
#include <iostream>
#include <cstdarg>

#include"http_connecttion_all.h"

using namespace std;


class http_connection
{
public:
	http_connection();
	void init(int sockfd, const sockaddr_in address);
	~http_connection();
	void init();
	void close_connection();
	void process();
	bool read();
	bool write();
private:
	HTTP_REPLY_CODE process_read();//������������
	bool process_write(HTTP_REPLY_CODE res);//���Ӧ��
	
	//��process_read����
	HTTP_REPLY_CODE parse_requestline(char* text);
	HTTP_REPLY_CODE parse_header(char* text);
	HTTP_REPLY_CODE parse_content(char* text);
	LINE_STATE parse_line();
	HTTP_REPLY_CODE do_request();

	//��process_write����
	void unmap();
	bool add_response(const char* format, ...);
	bool add_content(const char* content);
	bool add_status_line(int status, const char* title);
	bool add_headers(int content_length);
	bool add_content_length(int content_length);
	bool add_linger();
	bool add_blank_line();
	

public:
	static int user_num;
	static int epoll_fd;

private:
	int connect_fd;
	sockaddr_in m_address;
	struct read_buffer read_Buffer;//��������
	struct write_buffer write_Buffer;//д������
	struct analysy_info analysy_result;//��������Ľ��
	CHECK_STATE check_state;
	METHOD method;
	char* file_address;//������ļ����ڴ�ӳ������λ��
	struct stat file_stat;// ����Ŀ���ļ�״̬
	struct iovec iv[2];//�����ڴ��
	int iv_num;//�ڴ������
};
#endif
