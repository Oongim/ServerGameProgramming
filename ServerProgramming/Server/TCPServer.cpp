#include "TCPServer.h"

TCPServer::TCPServer()
{
	Init();
	std::cout << "Server 실행" << std::endl;
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		err_quit("WSAStartup()");

	// socket()			대기용 소켓
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);		//주소체계(IPv4), 소켓타입(TCP), 프로토콜(0) 
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	Bind();

	Listen();

	Accept();
}

TCPServer::~TCPServer()
{
	//closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
}

void TCPServer::Init()
{
	stat.position = { 0.5,0.5,0.f };
}

void TCPServer::Bind()
{
	std::cout << "bind 진행" << std::endl;
	// bind()
	SOCKADDR_IN serveraddr;				//16바이트			
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;		//주소체계(IPv4)
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);	//IP주소, u_long (INADDR_ANY), host to network(빅앤디안 방식, 네트워크 바이트)
	serveraddr.sin_port = htons(SERVERPORT);		//포트 번호, 부호없는 16비트 정수값(u_short), host to network
	retval = bind(listen_sock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));		//		대기 소켓에다가 주소 값 전달 및 형변환, 크기 전달(16바이트)
	if (retval == SOCKET_ERROR) err_quit("bind()");
}

void TCPServer::Listen()
{
	std::cout << "listen 소켓 생성" << std::endl;
	// listen()
	retval = listen(listen_sock, SOMAXCONN);			// 업 앤 러닝 상태에서의 클라이언트 연락 대기
	if (retval == SOCKET_ERROR) err_quit("listen()");

	BOOL optval = TRUE;
	setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (char*)& optval, sizeof(optval));
	//네이글 알고리즘 설정, 미 설정 시 실제로 띄엄 띄엄 움직이는 문제 발생
}

void TCPServer::Accept()
{
	
	int addrlen;

	while (1)
	{
		std::cout << "대기 중" << std::endl;
		// accept()
		addrlen = sizeof(clientaddr);		//주소 길이
		client_sock = accept(listen_sock, (SOCKADDR*)& clientaddr, &addrlen);		//받을 때도 형변환, 길이 획득, listen_sock이 client_sock에게 정보 넘겨주기
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accpet()");
			break;
		}

		//접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접소: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// 클라이언트와 데이터 통신
		while (1)
		{
			recvData();

			Update();

			sendData();
		}
	}
	//closesocket()
	closesocket(client_sock);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
}

void TCPServer::Update()
{
	static float speed = 1.f;
	if (key.Up)
	{
		stat.position += { 0.f, speed, 0.f };
	}
	if (key.Down)
	{
		stat.position += { 0.f, -speed, 0.f };
	}
	if (key.Left)
	{
		stat.position += { -speed, 0.f, 0.f };
	}
	if (key.Right)
	{
		stat.position += { speed, 0.f, 0.f };
	}

	stat.position.x = clamp(stat.position.x, (float)-NUM_OF_CHESSBOARDLINE / 2 + 0.5f, (float)NUM_OF_CHESSBOARDLINE / 2 - 0.5f);
	stat.position.y = clamp(stat.position.y, (float)-NUM_OF_CHESSBOARDLINE / 2 + 0.5f, (float)NUM_OF_CHESSBOARDLINE / 2 - 0.5f);
	stat.position.z = clamp(stat.position.z, (float)-NUM_OF_CHESSBOARDLINE / 2 + 0.5f, (float)NUM_OF_CHESSBOARDLINE / 2 - 0.5f);
}

void TCPServer::recvData()
{
	//데이터 받기
	retval = recv(client_sock, (char*)& key, sizeof(key), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv()");
		return;
	}
	else if (retval == 0)
		return;

	// 받은 데이터 출력
	printf("[TCP/%s:%d] \n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	std::cout << "Up:"<<key.Up << "  Down:" << key.Down << "  Left:" << key.Left << "  Right:" << key.Right << std::endl;

}

void TCPServer::sendData()
{
	// 데이터 보내기
	retval = send(client_sock, (char*)& stat, sizeof(stat), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return;
	}
	std::cout << "전송 완료" << std::endl;
}

void TCPServer::err_quit(char* msg)
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

void TCPServer::err_display(char* msg)
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

//DWORD WINAPI ClientThread(LPVOID arg)
//{
//	SOCKET* s = (SOCKET*)(arg);
//
//	SOCKET client_sock= 
//
//	return 0;
//}
