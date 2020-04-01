#include "TCPServer.h"

TCPServer::TCPServer()
{
	Init();
	std::cout << "Server ����" << std::endl;
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		err_quit("WSAStartup()");

	// socket()			���� ����
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);		//�ּ�ü��(IPv4), ����Ÿ��(TCP), ��������(0) 
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	Bind();

	Listen();

	Accept();
}

TCPServer::~TCPServer()
{
	//closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
}

void TCPServer::Init()
{
	stat.position = { 0.5,0.5,0.f };
}

void TCPServer::Bind()
{
	std::cout << "bind ����" << std::endl;
	// bind()
	SOCKADDR_IN serveraddr;				//16����Ʈ			
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;		//�ּ�ü��(IPv4)
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);	//IP�ּ�, u_long (INADDR_ANY), host to network(��ص�� ���, ��Ʈ��ũ ����Ʈ)
	serveraddr.sin_port = htons(SERVERPORT);		//��Ʈ ��ȣ, ��ȣ���� 16��Ʈ ������(u_short), host to network
	retval = bind(listen_sock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));		//		��� ���Ͽ��ٰ� �ּ� �� ���� �� ����ȯ, ũ�� ����(16����Ʈ)
	if (retval == SOCKET_ERROR) err_quit("bind()");
}

void TCPServer::Listen()
{
	std::cout << "listen ���� ����" << std::endl;
	// listen()
	retval = listen(listen_sock, SOMAXCONN);			// �� �� ���� ���¿����� Ŭ���̾�Ʈ ���� ���
	if (retval == SOCKET_ERROR) err_quit("listen()");

	BOOL optval = TRUE;
	setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (char*)& optval, sizeof(optval));
	//���̱� �˰��� ����, �� ���� �� ������ ��� ��� �����̴� ���� �߻�
}

void TCPServer::Accept()
{
	
	int addrlen;

	while (1)
	{
		std::cout << "��� ��" << std::endl;
		// accept()
		addrlen = sizeof(clientaddr);		//�ּ� ����
		client_sock = accept(listen_sock, (SOCKADDR*)& clientaddr, &addrlen);		//���� ���� ����ȯ, ���� ȹ��, listen_sock�� client_sock���� ���� �Ѱ��ֱ�
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accpet()");
			break;
		}

		//������ Ŭ���̾�Ʈ ���� ���
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// Ŭ���̾�Ʈ�� ������ ���
		while (1)
		{
			recvData();

			Update();

			sendData();
		}
	}
	//closesocket()
	closesocket(client_sock);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
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
	//������ �ޱ�
	retval = recv(client_sock, (char*)& key, sizeof(key), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv()");
		return;
	}
	else if (retval == 0)
		return;

	// ���� ������ ���
	printf("[TCP/%s:%d] \n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	std::cout << "Up:"<<key.Up << "  Down:" << key.Down << "  Left:" << key.Left << "  Right:" << key.Right << std::endl;

}

void TCPServer::sendData()
{
	// ������ ������
	retval = send(client_sock, (char*)& stat, sizeof(stat), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return;
	}
	std::cout << "���� �Ϸ�" << std::endl;
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
