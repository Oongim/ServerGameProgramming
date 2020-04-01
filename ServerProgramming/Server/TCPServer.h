#pragma once
#include "global.h"

//DWORD WINAPI ClientThread(LPVOID arg);

void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);

class TCPServer
{
private:
	WSADATA wsa;

	SOCKET listen_sock;
	int retval;

	// 데이터 통신에 사용할 변수
	map <SOCKET, SOCKETINFO> clients;	//소켓 사용하는거 바람직하지 않음, 소켓 번호가 중구난방으로 운영체제가 정하므로,원래는 0,1,2,3 인덱스를 써야함 어떤 플레이어다 직관적으로 알 수 있게 소켓 아이디가 재사용도 되면 안되는데 소켓은 재사용 될 수 있다.

	//HANDLE hClientCommunicationThread;	// 게임서버스레드

	KeyInput key;
	CharacterStatus stat;

public:
	TCPServer();
	~TCPServer();

private:
	void err_quit(char* msg);
	void err_display(char* msg);

	void Bind();
	void Listen();

	void Accept();

	void Update();

	void Init();
public:
	void recvData();
	void sendData();

	
};

