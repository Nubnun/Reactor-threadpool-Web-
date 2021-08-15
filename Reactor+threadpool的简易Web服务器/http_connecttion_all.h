#pragma once
const int MAX_READ_BUFFER = 2048;//���������Ĵ�С
const int MAX_WRITE_BUFFER = 2048;//д�������Ĵ�С
const int FILE_PATH_LENGTH = 100;//�����ļ�·��
const string root{ "/var/www/html" };//��վ��Ŀ¼

//��������ʱ��������״̬
enum CHECK_STATE {
	CHECK_STATE_REQUESTLINE = 0,//���ڷ���������
	CHECK_STATE_REQUESTHEADER,//���ڷ�������ͷ��
	CHECK_STATE_CONTENT//���ڷ���������
};
//���������ܷ��ص�״̬��
enum HTTP_REPLY_CODE {
	NO_REQUEST,//��������
	GET_REQUEST,//����ɹ�
	BAD_REQUEST,//�����﷨����
	NO_RESOURESE,//��������û���ҵ��������Դ
	FORBIDDEN_REQUEST,//û�з���Ȩ��
	INTERNAL_REQUEST,//�������ڲ����� 
	CLOSED_CONNECTTION//���ӹر�
};
//�еĶ�ȡ״̬
enum LINE_STATE {
	LINE_OK,//�����ȡ�ɹ�
	LINE_LINE_FAULT,//��������
	LINE_OPEN//����������Ҫ���Ȼ����ʣ�µ�����
};

struct read_buffer {
	string buf[MAX_READ_BUFFER];
	int read_index;
	int check_index;
	int analyse_index;
};

struct write_buffer {
	string buf[MAX_WRITE_BUFFER];
	int write_index;
};

struct analysy_info {
	string file_path[FILE_PATH_LENGTH];// �ļ�·��
	string url;//�ļ���
	string version;//httpЭ��汾
	string host;//������
	int contet_length;//��Ϣ�峤��
	bool linger��//�Ƿ񱣳�����
};
