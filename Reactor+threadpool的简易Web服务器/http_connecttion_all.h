#pragma once
#include <string>
const int MAX_READ_BUFFER = 2048;//读缓冲区的大小
const int MAX_WRITE_BUFFER = 2048;//写缓冲区的大小
const int FILE_PATH_LENGTH = 100;//请求文件路径
const char* root = "/val/www/html";//网站根目录

//解析请求时，解析的状态
enum CHECK_STATE {
	CHECK_STATE_REQUESTLINE = 0,//正在分析请求行
	CHECK_STATE_REQUESTHEADER,//正在分析请求头部
	CHECK_STATE_CONTENT//正在分析请求体
};
//服务器可能返回的状态码
enum HTTP_REPLY_CODE {
	NO_REQUEST,//请求不完整
	GET_REQUEST,//请求成功
	BAD_REQUEST,//请求语法错误
	NO_RESOURESE,//服务器上没有找到请求的资源
	FORBIDDEN_REQUEST,//没有访问权限
	INTERNAL_ERRO,//服务器内部错误 
	CLOSED_CONNECTTION//连接关闭
};
//行的读取状态
enum LINE_STATE {
	LINE_OK,//请求读取成功
	LINE_FAULT,//请求有误
	LINE_OPEN//请求不完整，要求既然接受剩下的内容
};

enum METHOD {
	GET = 0, POST, PUSH, HEAD, DELETE, TRACE,
	OPTIONS, CONNECT, PATCH
};

struct read_buffer {
	char buf[MAX_READ_BUFFER];
	int read_index;
	int check_index;
	int analyse_index;
};

struct write_buffer {
	char buf[MAX_WRITE_BUFFER];
	int write_index;
};

struct analysy_info {
	char file_path[FILE_PATH_LENGTH];// 文件路径
	char* url;//文件名
	char* version;//http协议版本
	char* host;//主机名
	int contet_length;//消息体长度
	bool linger;//是否保持连接
};
