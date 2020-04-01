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

	// ������ ��ſ� ����� ����
	map <SOCKET, SOCKETINFO> clients;	//���� ����ϴ°� �ٶ������� ����, ���� ��ȣ�� �߱��������� �ü���� ���ϹǷ�,������ 0,1,2,3 �ε����� ����� � �÷��̾�� ���������� �� �� �ְ� ���� ���̵� ���뵵 �Ǹ� �ȵǴµ� ������ ���� �� �� �ִ�.

	//HANDLE hClientCommunicationThread;	// ���Ӽ���������

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

