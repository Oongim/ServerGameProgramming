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
	TCPClient();							//생성 시 윈속초기화와 소켓 생성, connect 수행
	~TCPClient();							//소멸 시 closesocket과 윈속종료 수행

	//에러 내용 출력 함수
	void err_quit(char* msg);
	void err_display(char* msg);

	int recvn(SOCKET s, char* buf, int len, int flags);

public:
	//int TitleSceneSendData(unsigned char msg);	

	//int TitleSceneRecvData(unsigned char& msg);

	int PlaySceneSendData(KeyInput&);

	int PlaySceneRecvData(PACKET& data);

};