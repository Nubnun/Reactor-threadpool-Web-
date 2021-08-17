#include "http_connection.h"


const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your retquest has bad syntax ot is inherently impossible to satisfy.\n";
const char* error_403_title = "forbidden";
const char* error_403_form = "you do not have permission to get file from this server.\n";
const char* error_404_title = "Not Fount";
const char* error_404_form = "The request file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the request file.\n";

int set_nonblocking(int fd) {
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

int http_connection::epoll_fd = -1;
int http_connection::user_num = 0;

void addfd(int epoll_fd, int fd, bool one_shot) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLHUP;
	if (one_shot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	set_nonblocking(fd);
}
	
void rm_fd(int epoll_fd, int fd) {
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}

void mod_fd(int epoll_fd, int fd, int ev) {
	epoll_event event;
	event.data.fd = fd;
	event.events = ev | EPOLLIN | EPOLLET | EPOLLHUP;
	epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

http_connection::http_connection()
{
}



void http_connection::init(int sockfd, const sockaddr_in address)
{
	connect_fd = sockfd;
	m_address = address;
	user_num++;
	addfd(epoll_fd, sockfd, 1);
	init();
}

http_connection::~http_connection()
{

}

void http_connection::init()
{
	read_Buffer.analyse_index = 0;
	read_Buffer.check_index = 0;
	read_Buffer.read_index = 0;
	write_Buffer.write_index = 0;
	check_state = CHECK_STATE_REQUESTLINE;
	analysy_result.contet_length = 0;
	analysy_result.url = 0;
	analysy_result.host = 0;
	analysy_result.version = 0;
	analysy_result.linger = false;
	memset(read_Buffer.buf, '\0', MAX_READ_BUFFER);
	memset(write_Buffer.buf, '\0', MAX_WRITE_BUFFER);
	memset(analysy_result.file_path, '\0', FILE_PATH_LENGTH);
	}

void http_connection::close_connection()
{
	if (connect_fd != -1) {
		rm_fd(epoll_fd, connect_fd);
		connect_fd = -1;
		user_num--;
	}
	return;
}

bool http_connection::read()
{
	if (read_Buffer.read_index >= MAX_READ_BUFFER)
		return false;
	int bytes_read = 0;
	while (1) {
		bytes_read = recv(connect_fd, read_Buffer.buf + read_Buffer.read_index,
			MAX_READ_BUFFER - read_Buffer.read_index, 0);
		if (bytes_read == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			return false;
		}
		else if (bytes_read == 0) {
			return false;
		}
		else {
			read_Buffer.read_index += bytes_read;
		}
	}
	return true;
}

bool http_connection::write()
{
	int temp = 0;
	int byte_to_send = write_Buffer.write_index;
	int byte_have_send = 0;
	if (byte_to_send == 0) {
		mod_fd(epoll_fd, connect_fd, EPOLLIN);
		init();
		return true;
	}
	while (1) {
		temp = writev(connect_fd, iv, iv_num);
		if (temp <= -1) {
			if (errno == EAGAIN) {
				mod_fd(epoll_fd, connect_fd, EPOLLOUT);
				return true;
			}
			unmap();
			return false;
		}
		byte_have_send += temp;
		byte_to_send -= temp;
		if (byte_to_send <= byte_have_send) {
			unmap();
			if (analysy_result.linger) {
				init();
				mod_fd(epoll_fd, connect_fd, EPOLLIN);
				return true;
			}
		}
		else {
			mod_fd(epoll_fd, connect_fd, EPOLLIN);
			return false;
		}
	}
}

HTTP_REPLY_CODE http_connection::process_read()
{
	LINE_STATE line_state = LINE_OK;
	HTTP_REPLY_CODE ret = NO_REQUEST;
	char* text;
	while ((line_state = parse_line()) == LINE_OK || 
		((check_state == CHECK_STATE_CONTENT) && (line_state == LINE_OK))) {
		text = read_Buffer.buf + read_Buffer.analyse_index;
		read_Buffer.analyse_index = read_Buffer.check_index;
		cout << "got one http line : " << text << endl;
		switch (check_state)
		{
		case CHECK_STATE_REQUESTLINE:
			ret = parse_requestline(text);
			if (ret == BAD_REQUEST)
				return BAD_REQUEST;
			break;
		case CHECK_STATE_REQUESTHEADER:
			ret = parse_header(text);
			if (ret == BAD_REQUEST)
				return BAD_REQUEST;
			else if (ret == GET_REQUEST)
				return do_request();
			break;
		case CHECK_STATE_CONTENT:
			ret = parse_content(text);
			if (ret == GET_REQUEST)
				return do_request();
			line_state = LINE_OPEN;
			break;
		default:
			return INTERNAL_ERRO;
			break;
		}
	}
	return NO_REQUEST;
}


HTTP_REPLY_CODE http_connection::parse_requestline(char* text)
{
	analysy_result.url = strpbrk(text, "\t");//再text中找到"\t"第一次出现的位置
	if (!analysy_result.url)
		return BAD_REQUEST;
	*analysy_result.url++ = '\0';
	char* temp = text;
	if (strcasecmp(temp, "GET") == 0)//比较两个字符串是否相等，相等返回零；
		method = GET;
	else
		return BAD_REQUEST;
	analysy_result.url += strspn(analysy_result.url, "\t");//跳过\t
	analysy_result.version = strpbrk(analysy_result.url, "\t");
	if (!analysy_result.url)
		return BAD_REQUEST;
	analysy_result.url += strspn(analysy_result.url, "\t");
	*analysy_result.version++ = '\0';
	analysy_result.version += strspn(analysy_result.version, "\t");
	if (strcasecmp(analysy_result.version, "http/1.1") != 0)
		return BAD_REQUEST;
	if (strncasecmp(analysy_result.url, "http://", 7) == 0) {
		analysy_result.url += 7;
		analysy_result.url = strchr(analysy_result.url, '/');
	}
	if (!analysy_result.url || analysy_result.url[0] != '/')
		return BAD_REQUEST;
	check_state = CHECK_STATE_REQUESTHEADER;
	return NO_REQUEST;
}

HTTP_REPLY_CODE http_connection::parse_header(char* text)
{
	if (text[0] == '\0') {
		if (analysy_result.contet_length != 0) {
			check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	else if (strncasecmp(text, "Connection:", 11) == 0) {
		text += 11;
		text += strspn(text, "\t");
		if (strcasecmp(text, "keep_alive") == 0)
			analysy_result.linger = true;
	}
	else if (strncasecmp(text, "Content-Length:", 15) == 0) {
			text += 15;
			text += strspn(text, "\t");
			analysy_result.contet_length = atol(text);
	}
	else if (strncasecmp(text, "Host:", 5) == 0) {
			text += 5;
			text += strspn(text, "\t");
			analysy_result.host = text;
	}
	else
		cout << "unknow header" << text << endl;
	return NO_REQUEST;
}

HTTP_REPLY_CODE http_connection::parse_content(char* text)
{
	if (read_Buffer.read_index >= read_Buffer.check_index + analysy_result.contet_length) {
		text[analysy_result.contet_length] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

LINE_STATE http_connection::parse_line()
{
	char temp;
	int check_index = read_Buffer.check_index;
	int read_index = read_Buffer.read_index;
	for (; check_index < read_index; check_index++) {
		temp = read_Buffer.buf[check_index];
		if (temp == '\r') {
			if (read_Buffer.buf[check_index + 1] == read_index) {
				read_Buffer.check_index = check_index;
				return LINE_OPEN;
			}
			else if (read_Buffer.buf[check_index + 1] == '\n') {
				read_Buffer.buf[check_index++] = '\0';
				read_Buffer.buf[check_index++] = '\0';
				read_Buffer.check_index = check_index;
				return LINE_OK;
			}
			return LINE_FAULT;
		}
		else if (temp == '\n') {
			if (check_index > 1 && read_Buffer.buf[check_index - 1] == '\r') {
				read_Buffer.buf[check_index - 1] = '\0';
				read_Buffer.buf[check_index++] = '\0';
				read_Buffer.check_index = check_index;
				return LINE_OK;
			}
			return LINE_FAULT;
		}
	}
	return LINE_OPEN;
}

HTTP_REPLY_CODE http_connection::do_request()
{
	strcpy(analysy_result.file_path, root);
	int len = strlen(root);
	strncpy(analysy_result.file_path + len, analysy_result.url, FILE_PATH_LENGTH - len - 1);
	if (stat(analysy_result.file_path, &file_stat) < 0)
		return NO_RESOURESE;
	if (!(file_stat.st_mode & S_IROTH))
		return FORBIDDEN_REQUEST;
	if (S_ISDIR(file_stat.st_mode))
		return BAD_REQUEST;
	int fd = open(analysy_result.file_path, O_RDONLY);
	file_address = (char*)mmap(0, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	return GET_REQUEST;
}

void http_connection::unmap()
{
	if (file_address) {
		munmap(file_address, file_stat.st_size);
		file_address = 0;
	}
}

//主要逻辑是根据解析请求报文的结果，来选择相应的回复报文。
bool http_connection::process_write(HTTP_REPLY_CODE ret)
{
	switch (ret)
	{
	case GET_REQUEST:
	{
		add_status_line(200, ok_200_title);
		if (file_stat.st_size != 0) {
			add_headers(file_stat.st_size);
			iv[0].iov_base = write_Buffer.buf;
			iv[0].iov_len = write_Buffer.write_index;
			iv[1].iov_base = file_address;
			iv[1].iov_len = file_stat.st_size;
			iv_num = 2;
			return true;
		}
		else {
			const char* ok_200_string = "<html><body></body></html>";
			add_headers(strlen(ok_200_string));
			if (!add_content(ok_200_string))
				return false;
			return true;
		}
		break;
	}
	case BAD_REQUEST:
	{
		add_status_line(400, error_400_title);
		add_headers(strlen(error_400_title));
		if (!add_content(error_400_form))
			return false;
		break;
	}
	case NO_RESOURESE:
	{
		add_status_line(404, error_404_title);
		add_headers(strlen(error_404_form));
		if (!add_content(error_404_form))
			return false;
		break;
	}
	case FORBIDDEN_REQUEST:
	{
		add_status_line(403, error_403_title);
		add_headers(strlen(error_403_form));
		if (!add_content(error_403_form))
			return false;
		break;
	}
	case INTERNAL_ERRO:
	{
		add_status_line(500, error_500_title);
		add_headers(strlen(error_500_form));
		if (!add_content(error_500_form))
			return false;
		break;
	}
	default:
		return false;
	}
	iv[0].iov_base = write_Buffer.buf;
	iv[0].iov_len = write_Buffer.write_index;
	iv_num = 1;
	return true;
}

//将回复报文的内容写到用户写缓冲区；
bool http_connection::add_response(const char* format, ...)
{
	if (write_Buffer.write_index >= MAX_WRITE_BUFFER)
		return false;
	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(write_Buffer.buf + write_Buffer.write_index, MAX_WRITE_BUFFER - 1 - write_Buffer.write_index, format, arg_list);
	if (len > MAX_WRITE_BUFFER - 1 - write_Buffer.write_index)
		return false;
	write_Buffer.write_index += len;
	va_end(arg_list);
	return true;
}
//回复报文内容
bool http_connection::add_content(const char* content)
{
	return add_response("%s", content);
}
//状态行
bool http_connection::add_status_line(int status, const char* title)
{
	return add_response("%s%d%s\r\n", "HTTP/1.1", status, title);
}

//依次添加状态行、回复报文首部、回复报文内容
bool http_connection::add_headers(int content_length)
{
	add_content_length(content_length);
	add_linger();
	add_blank_line();
}

bool http_connection::add_content_length(int content_length)
{
	return add_response("Content_length: %d\r\n", content_length);
}

bool http_connection::add_linger()
{
	return add_response("Connection: %s\r\n", (analysy_result.linger == true ? "keep_alive" : "close"));
}

bool http_connection::add_blank_line()
{
	return add_response("%s", "\r\n");
}

//线程池调用的函数，先进行HTTP请求报文的解析，然后将回复报文的内容写到用户的写缓冲区，并向epoll事件表注册可写时间；
void http_connection::process()
{
	HTTP_REPLY_CODE read_res = process_read();
	//收到的报文不完全，线程直接退出事件处理，并向内核事件表注册可读事件，期待下次收到完整的HTTP请求报文；
	if (read_res == NO_REQUEST) {
		mod_fd(epoll_fd, connect_fd, EPOLLIN);
		return;
	}
	bool write_res = process_write(read_res);
	if (!write_res) {
		close_connection();
	}
	mod_fd(epoll_fd, connect_fd, EPOLLOUT);
}