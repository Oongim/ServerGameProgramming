#include "global.h"

map <SOCKET, SOCKETINFO> clients;	//���� ����ϴ°� �ٶ������� ����, ���� ��ȣ�� �߱��������� �ü���� ���ϹǷ�,������ 0,1,2,3 �ε����� ����� � �÷��̾�� ���������� �� �� �ְ� ���� ���̵� ���뵵 �Ǹ� �ȵǴµ� ������ ���� �� �� �ִ�.
PACKET sendPack;

void CALLBACK Key_recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK Stat_send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void Update(KeyInput& key, CharacterStatus& stat, int id);

void CALLBACK Key_recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{//� recv�� �Ϸ�Ǿ���. �״�� �ݻ��ؾ��ϴ� � �������� �˾Ƴ��� overlapped�� hEvent
	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);
	if (dataBytes == 0)				//������ close�ߴٴ� �ǹ��̴� ���� close ����, error�ڵ� ó���� �߰� ����� �� ���߿�
	{
		sendPack.stats[clients[client_s].player_id].position = { 0.f,0.f,0.f };
		sendPack.NumOfClient--;

		closesocket(clients[client_s].socket);
		clients.erase(client_s);		//������ ��
		return;
	}  // Ŭ���̾�Ʈ�� closesocket�� ���� ���


	//cout << "From client : " << clients[client_s].key.Up<< clients[client_s].key.Down<< clients[client_s].key.Right<<clients[client_s].key.Left << " (" << dataBytes << ") bytes)\n"; //�� �޼����� �Դ�.
	cout << "Client Num:" << sendPack.NumOfClient<< " (" << dataBytes << ") bytes\n";
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));	//0���� �ʱ�ȭ�ؼ� ����
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;				//�̰ŵ� �ʱ�ȭ �Ǵ� �ٽ� ���� ����

	Update(clients[client_s].key, sendPack.stats[clients[client_s].player_id], clients[client_s].player_id);

	WSASend(client_s, &(clients[client_s].dataBuffer[1]), 1, NULL, 0, &(clients[client_s].overlapped), Stat_send_callback); //send
}

void CALLBACK Stat_send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{//send�� ��������� �ٽ� recv����� ��
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	if (dataBytes == 0) {
		sendPack.stats[clients[client_s].player_id].position = { 0.f,0.f,0.f };
		sendPack.NumOfClient--;

		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		return;
	}  // Ŭ���̾�Ʈ�� closesocket�� ���� ���

	//cout << "TRACE - Send message : " << sendPack.stats[clients[client_s].player_id].position.x << "," << sendPack.stats[clients[client_s].player_id].position.y << " (" << dataBytes << " bytes)\n";	//�̰� ����
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));				//�ʱ�ȭ
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;							//���� �־���

	WSARecv(client_s, &clients[client_s].dataBuffer[0], 1, 0, &flags, &(clients[client_s].overlapped), Key_recv_callback);//recv
}

int main()
{
	int retval;

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
		return 1;
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//���� ���� WSA_FLAG_OVERLAPPED��������� �� overlapped IO
	if (listenSocket == INVALID_SOCKET) err_quit("socket()");

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	retval = ::bind(listenSocket, (struct sockaddr*) & serverAddr, sizeof(SOCKADDR_IN));  //bind
	if (retval == SOCKET_ERROR) err_quit("bind()");
	cout << "Bind() ����\n";

	retval = listen(listenSocket, SOMAXCONN);													//listen  ��뷮�̸� �ƽ�_Ŀ�ؼ�.
	if (retval == SOCKET_ERROR) err_quit("listen()");
	cout << "listen() ����\n";

	BOOL optval = TRUE;
	setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)& optval, sizeof(optval));
	//���̱� �˰��� ����, �� ���� �� ������ ��� ��� �����̴� ���� �߻�

	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);

	
	while (true) {														//��� accept�ؾ���
		if (sendPack.NumOfClient == 10)continue;

		cout << "accept() ��\n";
		SOCKET clientSocket = accept(listenSocket, (struct sockaddr*) & clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			err_display("accept()");
			memset(&clients[clientSocket].overlapped, 0, sizeof(WSAOVERLAPPED));
			continue;
		}
		clients[clientSocket] = SOCKETINFO{};							//������ �ε����� �ϴ� ��ü�� ���� �ʱ�ȭ
		clients[clientSocket].socket = clientSocket;

		clients[clientSocket].dataBuffer[0].len = sizeof(KeyInput);
		clients[clientSocket].dataBuffer[0].buf = (char*)& clients[clientSocket].key;
		clients[clientSocket].dataBuffer[1].len = sizeof(PACKET);
		clients[clientSocket].dataBuffer[1].buf = (char*)& sendPack;

		for (int i = 0; i < MAX_PLAYER; ++i)
		{
			if (sendPack.stats[i].position.x == 0.f)
			{
				clients[clientSocket].player_id = i;
				sendPack.stats[i].position = { 0.5,0.5,0.f };
				sendPack.stats[i].whoseControlNum = i;

				sendPack.NumOfClient = i;

				sendPack.NumOfClient++;
				break;
			}
		}
		

		memset(&clients[clientSocket].overlapped, 0, sizeof(WSAOVERLAPPED));			//���� ���� 0���� �ʱ�ȭ����� ��
		clients[clientSocket].overlapped.hEvent = (HANDLE)clients[clientSocket].socket;
		DWORD flags = 0;
		WSARecv(clients[clientSocket].socket, &clients[clientSocket].dataBuffer[0], 1, NULL,//null�� ���� ������ recv�ϸ� ��� ���ۿ� �� �� �ִ�. �״�� �Ǵµ� ��������� ����
			&flags, &(clients[clientSocket].overlapped)/*�������� ����ü�� �ּҰ� �ݹ� �Լ��� �Ѿ �ȿ� ������ �ü���� �˾Ƽ� ����*/
			, Key_recv_callback);	//�ݹ� �Լ�
	}
	closesocket(listenSocket);
	WSACleanup();
}

void Update(KeyInput& key, CharacterStatus& stat,int id)
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