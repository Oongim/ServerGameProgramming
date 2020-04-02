#include "global.h"

map <SOCKET, SOCKETINFO> clients;	//소켓 사용하는거 바람직하지 않음, 소켓 번호가 중구난방으로 운영체제가 정하므로,원래는 0,1,2,3 인덱스를 써야함 어떤 플레이어다 직관적으로 알 수 있게 소켓 아이디가 재사용도 되면 안되는데 소켓은 재사용 될 수 있다.

void CALLBACK Key_recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK ClientNum_send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void CALLBACK Stat_send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags);
void Update(KeyInput& key, CharacterStatus& stat, char& clientNum);

void CALLBACK Key_recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{//어떤 recv가 완료되었다. 그대로 반사해야하니 어떤 소켓인지 알아내자 overlapped의 hEvent
	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	if (dataBytes == 0)				//소켓이 close했다는 의미이니 나도 close 하자, error코드 처리도 추가 해줘야 함 나중에
	{
		closesocket(clients[client_s].socket);
		clients.erase(client_s);		//지워서 끝
		return;
	}  // 클라이언트가 closesocket을 했을 경우


	cout << "From client : " << clients[client_s].key.Down << " (" << dataBytes << ") bytes)\n"; //이 메세지가 왔다.
	//clients[client_s].dataBuffer[0].len = dataBytes;	//데이타 버퍼 길이 세팅 보내온 양 만큼 그대로 보내야 함, 버퍼 사이즈로 하면 너무 많이 보냄
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));	//0으로 초기화해서 재사용
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;				//이거도 초기화 되니 다시 소켓 설정

	Update(clients[client_s].key, clients[client_s].stat, clients[client_s].NumOfClient);

	WSASend(client_s, &(clients[client_s].dataBuffer[1]), 1, NULL, 0, &(clients[client_s].overlapped), ClientNum_send_callback);
	//send 길이만 맞춰서 보냄 
}

void CALLBACK ClientNum_send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{//send가 종료됬으니 다시 recv해줘야 함
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	if (dataBytes == 0) {
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		return;
	}  // 클라이언트가 closesocket을 했을 경우
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));				//초기화
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;							//소켓 넣어줌

	cout << "TRACE - Send message : " << (int)clients[client_s].NumOfClient << " (" << dataBytes << " bytes)\n";	//이거 보냄
	//for (auto stat : clients) {
		WSASend(client_s, &(clients[client_s].dataBuffer[2]), 1, NULL, 0, &(clients[client_s].overlapped), Stat_send_callback); //send
	//}

	
}

void CALLBACK Stat_send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{//send가 종료됬으니 다시 recv해줘야 함
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	if (dataBytes == 0) {
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		return;
	}  // 클라이언트가 closesocket을 했을 경우

	cout << "TRACE - Send message : " << clients[client_s].stat.position.x << "," << clients[client_s].stat.position.y << " (" << dataBytes << " bytes)\n";	//이거 보냄
	memset(&(clients[client_s].overlapped), 0, sizeof(WSAOVERLAPPED));				//초기화
	clients[client_s].overlapped.hEvent = (HANDLE)client_s;							//소켓 넣어줌

	WSARecv(client_s, &clients[client_s].dataBuffer[0], 1, 0, &flags, &(clients[client_s].overlapped), Key_recv_callback);//recv
}

int main()
{
	int retval;

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
		return 1;
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//리슨 소켓 WSA_FLAG_OVERLAPPED설정해줘야 함 overlapped IO
	if (listenSocket == INVALID_SOCKET) err_quit("socket()");

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	retval = ::bind(listenSocket, (struct sockaddr*) & serverAddr, sizeof(SOCKADDR_IN));  //bind
	if (retval == SOCKET_ERROR) err_quit("bind()");
	cout << "Bind() 성공\n";

	retval = listen(listenSocket, SOMAXCONN);													//listen  대용량이면 맥스_커넥션.
	if (retval == SOCKET_ERROR) err_quit("listen()");
	cout << "listen() 성공\n";

	BOOL optval = TRUE;
	setsockopt(listenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)& optval, sizeof(optval));
	//네이글 알고리즘 설정, 미 설정 시 실제로 띄엄 띄엄 움직이는 문제 발생

	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);

	while (true) {														//계속 accept해야함
		cout << "accept() 중\n";
		SOCKET clientSocket = accept(listenSocket, (struct sockaddr*) & clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			err_display("accept()");
			memset(&clients[clientSocket].overlapped, 0, sizeof(WSAOVERLAPPED));
			continue;
		}
		clients[clientSocket] = SOCKETINFO{};							//소켓을 인덱스로 하는 객체를 생성 초기화
		clients[clientSocket].socket = clientSocket;

		clients[clientSocket].dataBuffer[0].len = sizeof(KeyInput);
		clients[clientSocket].dataBuffer[0].buf = (char*)& clients[clientSocket].key;
		clients[clientSocket].dataBuffer[1].len = sizeof(char);
		clients[clientSocket].dataBuffer[1].buf = (char*)& clients[clientSocket].NumOfClient;
		clients[clientSocket].dataBuffer[2].len = sizeof(CharacterStatus);
		clients[clientSocket].dataBuffer[2].buf = (char*)&clients[clientSocket].stat;
		clients[clientSocket].stat.position = { 0.5,0.5,0.f };

		memset(&clients[clientSocket].overlapped, 0, sizeof(WSAOVERLAPPED));			//쓰기 전에 0으로 초기화해줘야 함
		clients[clientSocket].overlapped.hEvent = (HANDLE)clients[clientSocket].socket;
		//원래 이렇게 소켓 느라고 만들진 않음, 꼼수 recv부터 해야 하는데 callback함수가 호출됨, callback함수는 정보가 필요하다 어떤 소켓의 recv가 종료되었냐 
		//콜백 함수는 하나만 두고 여러 소켓이 공유를 한다 어떤 소켓인지 알기 위해 나머지는 세팅할 수가 없고 오버랩드 구조체에 포함할 수 밖에 없다.
		//오버랩드 구조체에서 나머지는 건들 수 없고 hEvent가 콜백함수니 필요가 없다 무시가 되버리니 여기에 저장하자
		DWORD flags = 0;
		WSARecv(clients[clientSocket].socket, &clients[clientSocket].dataBuffer[0], 1, NULL,//null로 하지 않으면 recv하면 즉시 버퍼에 들어갈 수 있다. 그대로 되는데 깔끔해지지 않음
			&flags, &(clients[clientSocket].overlapped)/*오버랩드 구조체의 주소가 콜백 함수에 넘어감 안에 내용은 운영체제가 알아서 변경*/
			, Key_recv_callback);	//콜백 함수
	}
	closesocket(listenSocket);
	WSACleanup();
}

void Update(KeyInput& key, CharacterStatus& stat, char& clientNum)
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

	clientNum = clients.size();
}