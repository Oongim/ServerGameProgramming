#include "TCPClient.h"
//#define SERVERIP "192.168.43.23"
#define SERVERPORT 9000
#define BUFSIZE 524288
#include<iostream>

void TCPClient::err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)& lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void TCPClient::err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)& lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 사용자 정이 데이터 수신 함수
int TCPClient::recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}
//////////////Title Scene//////////////////////////////////////
//int TCPClient::TitleSceneSendData(unsigned char msg)
//{
//	int retval;
//	retval = send(sock, (char*)& msg, sizeof(msg), 0);
//	if (retval == SOCKET_ERROR) {
//		err_display("send()");
//		return 0;
//	}
//	return 1;
//}
//
//int TCPClient::TitleSceneRecvData(unsigned char& msg)
//{
//	int retval;
//	retval = recv(sock, (char*)& msg, sizeof(msg), 0);
//	if (retval == SOCKET_ERROR) {
//		err_display("recv()");
//		return 0;
//	}
//	printf("%d\n", msg);
//
//	return 1;
//}
///////////////////////////////////////////////////////////////

////////////Play Scene///////////////////////////////////////
int TCPClient::PlaySceneSendData(KeyInput& data)
{
	int retval;
	//std::cout << data->Up << data->Down << data->Left << data->Right << data->Interact1 << data->Interact2 << data->Interact3 << data->Interact4 << std::endl;
	retval = send(sock, (char*)& data, sizeof(data), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
		return 0;
	}
	return 1;
}
int TCPClient::PlaySceneRecvData(std::vector<CharacterStatus>& data)
{
	int retval;
	char clientNum;
	retval = recvn(sock, (char*)& clientNum, sizeof(char), 0);
	if (retval == SOCKET_ERROR) {
		err_display("recv()");
		return 0;
	}
	else
	{
		retval = 1;
	}
	std::cout << "clientNum : " << (int)clientNum << std::endl;

	data.clear();
	data.reserve(clientNum);

	CharacterStatus recvData;
	for (int i = 0; i < clientNum; ++i) {
		recv(sock, (char*)& recvData, sizeof(recvData), 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			return 0;
		}
		else
		{
			retval = 1;
		}

		data.emplace_back(recvData);
	}

	return retval;
}
//////////////////////////////////////////////////////////////

TCPClient::TCPClient()
{
	int retval;

	//윈속 초기화
	wsa;
	if (WSAStartup(MAKEWORD(2, 0), &wsa) != 0)
		err_quit("WSAStartup()");

	// socket()
	sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	char SERVERIP[256];

	std::cout << std::endl << "서버 IP 입력: ";
	std::cin >> SERVERIP;

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(SOCKADDR_IN));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	retval = connect(sock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");
	else { printf("서버 접속 성공\n"); }
}


TCPClient::~TCPClient()
{
	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
}