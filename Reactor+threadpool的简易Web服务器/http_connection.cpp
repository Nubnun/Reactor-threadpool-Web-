#include "http_connection.h"

http_connection::http_connection(int sockfd, const sockaddr_in address)
{
	
}

http_connection::~http_connection()
{

}

void http_connection::init()
{

}

bool http_connection::close_connection()
{
	return false;
}

void http_connection::process()
{

}

bool http_connection::read()
{
	return false;
}

bool http_connection::write()
{
	return false;
}

HTTP_REPLY_CODE http_connection::process_read()
{
	return HTTP_REPLY_CODE();
}

bool http_connection::process_write()
{
	return false;
}

HTTP_REPLY_CODE http_connection::parse_requestline(string& text)
{
	return HTTP_REPLY_CODE();
}

HTTP_REPLY_CODE http_connection::parse_header(string& text)
{
	return HTTP_REPLY_CODE();
}

HTTP_REPLY_CODE http_connection::parse_content(string& text)
{
	return HTTP_REPLY_CODE();
}

LINE_STATE http_connection::parse_line()
{
	return LINE_STATE();
}

HTTP_REPLY_CODE http_connection::do_request()
{
	return HTTP_REPLY_CODE();
}
