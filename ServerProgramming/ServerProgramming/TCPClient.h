#pragma once

#include <stdlib.h>
#include "global.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996) 



class TCPClient
{
private:
	WSADATA wsa;
	SOCKET sock;

public:
	TCPClient();							//���� �� �����ʱ�ȭ�� ���� ����, connect ����
	~TCPClient();							//�Ҹ� �� closesocket�� �������� ����

	//���� ���� ��� �Լ�
	void err_quit(char* msg);
	void err_display(char* msg);

	int recvn(SOCKET s, char* buf, int len, int flags);

public:
	//int TitleSceneSendData(unsigned char msg);	

	//int TitleSceneRecvData(unsigned char& msg);

	int PlaySceneSendData(KeyInput&);

	int PlaySceneRecvData(PACKET& data);

};