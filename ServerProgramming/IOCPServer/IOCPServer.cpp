#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "mswsock.lib")
#pragma warning(disable:4996)
using namespace std;

#include "protocol.h"

constexpr auto MAX_PACKET_SIZE = 255;  //char라 최대크기 255
constexpr auto MAX_BUF_SIZE = 1024;  //char라 최대크기 255
constexpr auto MAX_USER = 10;  //char라 최대크기 255

enum ENUMOP { OP_RECV, OP_SEND, OP_ACCEPT }; //op= 오퍼레이션


struct EXOVER {					//확장 오버랩드 구조
	WSAOVERLAPPED	over;		//오버랩드 구조체가 맨위에 있어야하고 아래에 데이터들을 추가함
	ENUMOP			op;
	char			io_buf[MAX_BUF_SIZE];		//버퍼의 위치를 포인터를 넣을바에 그냥 버퍼를 넣어버림
	WSABUF			wasbuf;
};												//전부다 재사용

//클라의 정보를 보내야한다.
struct CLIENT {
	SOCKET		m_s;
	int			m_id;
	EXOVER		m_recv_over;
	int			m_prev_size;
	char		m_packet_buf[MAX_PACKET_SIZE];

	short x, y;  //점점 커질건데  char는 256이라 너무 적어서 short로 함 int는 너무 큼
	char name[MAX_ID_LEN + 1];
};

CLIENT g_clients[MAX_USER];
int g_curr_user_id = 0;
HANDLE g_iocp;

void send_packet(int user_id, void* p)
{
	char* buf = reinterpret_cast<char*>(p);

	CLIENT& u = g_clients[user_id];

	EXOVER* exover = new EXOVER; //날라가니 어딘가에 할당해야한다. u.recv는 쓰면 안되고
	exover->op = OP_SEND;
	ZeroMemory(&exover->over, sizeof(exover->over));
	exover->wasbuf.buf = exover->io_buf;
	exover->wasbuf.len = buf[0];
	memcpy(exover->io_buf, buf, buf[0]);
	WSASend(u.m_s, &exover->wasbuf, 1, NULL, 0, &exover->over, NULL);
}


void send_login_ok_packet(int user_id)
{
	sc_packet_login_ok p;
	p.exp = 0;
	p.hp = 0;
	p.id = user_id;
	p.level = 0;
	p.size = sizeof(p);
	p.type = S2C_LOGIN_OK;
	p.x = g_clients[user_id].x;
	p.y = g_clients[user_id].y;

	send_packet(user_id, &p);
}

void send_move_packet(int user_id)
{
	sc_packet_move p;
	p.id = user_id;
	p.size = 0;
	p.type = S2C_LOGIN_OK;
	p.x = g_clients[user_id].x;
	p.y = g_clients[user_id].y;

	send_packet(user_id, &p);
}
void do_move(int user_id, int direction)
{
	CLIENT& u = g_clients[user_id];
	int	x = u.x;
	int	y = u.y;
	switch (direction) {
	case D_UP:		if( y>0)					y--; break;
	case D_DOWN:	if (y < (WORLD_HEIGHT - 1)) y++; break;
	case D_LEFT:	if (x > 0)					x--; break;
	case D_RIGHT:	if (x < (WORLD_WIDTH - 1))  x++; break;
	default:
		cout << "Unknown Direction from Client move packet!\n";
		DebugBreak();
		exit(-1);
	}
	u.x = x;
	u.y = y;
	send_move_packet(user_id);
}

void process_packet(int user_id, char* buf)
{
	switch (buf[1]) {  //size?
	case C2S_LOGIN: {
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(buf);
		strcpy_s(g_clients[user_id].name, packet->name);
		g_clients[user_id].name[MAX_ID_LEN] = NULL;
		send_login_ok_packet(user_id);
	}
		break;
	case C2S_MOVE: {
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(buf);
		do_move(user_id, packet->direction);
	}
		break;
	default:
		cout << "Unknown Packet Type Error!\n";
		DebugBreak();
		exit(-1);
	}
}

int  main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	SOCKET l_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//오버랩드 플래그 해야됨

	SOCKADDR_IN s_address;
	memset(&s_address, 0, sizeof(s_address));
	s_address.sin_family = AF_INET;
	s_address.sin_port = htons(SERVER_PORT);
	s_address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(l_socket, reinterpret_cast<sockaddr*>(&s_address), sizeof(s_address));
	//그냥 bind로 쓰면 c++11에 있는 바인드 키보드인가로 연결됨 원래 운영체제에 있는 bind쓰도록 ::붙인다

	listen(l_socket, SOMAXCONN);

	SOCKADDR_IN c_address;
	memset(&c_address, 0, sizeof(c_address));
	int c_addr_size = sizeof(c_address);

	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);	//iocp 핸들 만들기

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(l_socket), g_iocp, 999/*안쓸 키*/, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	EXOVER accept_over;
	ZeroMemory(&accept_over.over, sizeof(accept_over.over));
	accept_over.op = OP_ACCEPT;

	AcceptEx(l_socket, c_socket, accept_over.io_buf, NULL, sizeof(sockaddr_in) + 16/*바인드 할떄 썼죠? 근데 그대로 쓰면안되고 +16해줌 여유있게*/
		, sizeof(sockaddr_in) + 16, NULL/*비동기로 미리 받아서 처리하지 않을것이다*/
		, &accept_over.over);	//WS2tcpip에 없음 확장에 있어서 MSWSock
			//소켓을 만들어서 리턴하지 않음 , 미리 만들어놓고 만들어논 소켓에 클라이언트를 연결해놈 c_socket

	while (true) {
		DWORD io_byte;
		ULONG_PTR key;
		WSAOVERLAPPED* over;
		GetQueuedCompletionStatus(g_iocp, &io_byte, &key, &over, INFINITE);  //이걸로 받아야겠죠?
		//accept가 멈춰있으면 이걸 못받고 이거에서 멈춰있으면 accept를 못받네? 그렇다면 acceptex를 써야한다

		EXOVER* exover = reinterpret_cast<EXOVER*>(over);
		int user_id = static_cast<int>(key);
		CLIENT& cl = g_clients[user_id];

		switch (exover->op) {
		case OP_RECV: {
			process_packet(user_id, exover->io_buf);
			ZeroMemory(&cl.m_recv_over.over, sizeof(cl.m_recv_over.over));
			DWORD flags = 0;
			WSARecv(cl.m_s, &cl.m_recv_over.wasbuf, 1, NULL, &flags, &cl.m_recv_over.over, NULL);
		}
			break;
		case OP_SEND:		//오버랩드 구조체를 할당받아서 하는거니까
			delete exover;
			break;
		case OP_ACCEPT: {
			//억셉트를 받고 해야됨 이전의 과정들을
			int user_id = g_curr_user_id++;

			CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_iocp, user_id, 0);

			g_curr_user_id = g_curr_user_id % MAX_USER;		//원래는 빈자리를 찾아야됨 재사용 위함, 나중에
			CLIENT& nc = g_clients[user_id];
			nc.m_id = user_id;
			nc.m_prev_size = 0;
			nc.m_recv_over.op = OP_RECV;
			ZeroMemory(&nc.m_recv_over.over, sizeof(nc.m_recv_over.over));
			nc.m_recv_over.wasbuf.buf = nc.m_recv_over.io_buf;
			nc.m_recv_over.wasbuf.len = MAX_BUF_SIZE;
			nc.m_s = c_socket;
			nc.x = rand() % WORLD_WIDTH;
			nc.y = rand() % WORLD_HEIGHT;
			DWORD flags = 0;
			WSARecv(c_socket, &nc.m_recv_over.wasbuf, 1, NULL, &flags, &nc.m_recv_over.over, NULL);

			c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			ZeroMemory(&accept_over.over, sizeof(accept_over.over));	//재사용
			AcceptEx(l_socket, c_socket, accept_over.io_buf, NULL, sizeof(sockaddr_in) + 16/*바인드 할떄 썼죠? 근데 그대로 쓰면안되고 +16해줌 여유있게*/
				, sizeof(sockaddr_in) + 16, NULL/*비동기로 미리 받아서 처리하지 않을것이다*/
				, &accept_over.over);	//WS2tcpip에 없음 확장에 있어서 MSWSock
			//다시 억셉트를 받지않으면 다중접속을 할 수가 없다. 계속 억셉트를 유지해줘야함
		}
			break;
		}
	}
}