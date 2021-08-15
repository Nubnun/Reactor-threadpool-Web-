#pragma once

#include<sys/socket.h>
#include<arpa/inet.h>
#include<memory>
#include<sys/types.h>
#include<sys/stat.h>
#include"http_connecttion_all.h"
using namespace std;


class http_connection
{
public:
	http_connection(int sockfd, const sockaddr_in address);
	~http_connection();
	void init();
	bool close_connection();
	void process();
	bool read();
	bool write();
private:
	HTTP_REPLY_CODE process_read();//������������
	bool process_write();//���Ӧ��
	
	//��process_read����
	HTTP_REPLY_CODE parse_requestline(string& text);
	HTTP_REPLY_CODE parse_header(string& text);
	HTTP_REPLY_CODE parse_content(string& text);
	LINE_STATE parse_line();
	HTTP_REPLY_CODE do_request();

	//��process_write����


public:
	typedef shared_ptr<http_connection> ptr;
	static int user_num;
	static int epool_fd;

private:
	int connect_fd;
	sockaddr_in m_address;
	struct read_buffer read_Buffer;//��������
	struct write_buffer write_Buffer;//д������
	struct analysy_info analysy_result;//��������Ľ��
	CHECK_STATE check_state;
	string file_address;//������ļ����ڴ�ӳ������λ��
	struct stat file_stat;// ����Ŀ���ļ�״̬
	struct iovec iv[2];//�����ڴ��
	int iv_num;//�ڴ������
};

