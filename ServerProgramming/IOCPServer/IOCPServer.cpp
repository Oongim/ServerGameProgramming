#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "mswsock.lib")
#pragma warning(disable:4996)

using namespace std;

#include "protocol.h"
constexpr auto MAX_PACKET_SIZE = 255;
constexpr auto MAX_BUF_SIZE = 1024;
constexpr auto MAX_USER = 10;


enum ENUMOP { OP_RECV, OP_SEND, OP_ACCEPT };

struct EXOVER {
    WSAOVERLAPPED   over;
    ENUMOP         op;
    char         io_buf[MAX_BUF_SIZE];
    WSABUF         wsabuf;
};

struct CLIENT {
    SOCKET m_s;
    int m_id;
    EXOVER m_recv_over;
    int m_prev_size;
    char m_packet_buf[MAX_PACKET_SIZE];

    short x, y;
    char name[MAX_ID_LEN + 1];
};

CLIENT g_clients[MAX_USER];
HANDLE g_iocp;

void send_Packet(int user_id, void* p)
{
    char* buf = reinterpret_cast<char*>(p);

    CLIENT& u = g_clients[user_id];

    EXOVER* exover = new EXOVER;
    exover->op = OP_SEND;
    ZeroMemory(&exover->over, sizeof(exover->over));
    exover->wsabuf.buf = exover->io_buf;
    exover->wsabuf.len = buf[0];
    memcpy(exover->io_buf, buf, buf[0]);
    WSASend(u.m_s, &exover->wsabuf, 1, NULL, 0, &exover->over, NULL);

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

    send_Packet(user_id, &p);
}


void send_move_packet(int user_id)
{
    sc_packet_move p;
    p.id = user_id;
    p.size = sizeof(p);
    p.type = S2C_LOGIN_OK;
    p.x = g_clients[user_id].x;
    p.y = g_clients[user_id].y;

    send_Packet(user_id, &p);
}

void do_move(int user_id, int direction)
{
    CLIENT& u = g_clients[user_id];
    int x = u.x;
    int y = u.y;
    switch (direction) {
    case D_UP: if (y > 0) y--; break;
    case D_DOWN: if (y < (WORLD_HEIGHT - 1)) y++; break;
    case D_LEFT: if (x > 0) x--; break;
    case D_RIGHT: if (x < (WORLD_WIDTH - 1)) x++; break;
    default:
        cout << "Unknown Direction from Client move packet! \n";
        DebugBreak();
        exit(-1);
    }
    u.x = x;
    u.y = y;
    send_move_packet(user_id);
}

void process_packet(int user_id, char* buf)
{
    switch (buf[1]) {
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

int main()
{
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData);

    SOCKET l_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    SOCKADDR_IN s_address;
    memset(&s_address, 0, sizeof(s_address));
    s_address.sin_family = AF_INET;
    s_address.sin_port = htons(SERVER_PORT);
    s_address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    ::bind(l_socket, reinterpret_cast<sockaddr*>(&s_address), sizeof(s_address));

    listen(l_socket, SOMAXCONN);
    int g_current_user_id = 0;

    g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

    CreateIoCompletionPort(reinterpret_cast<HANDLE>(l_socket), g_iocp, 999, 0);
    SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    EXOVER accept_over;
    ZeroMemory(&accept_over.over, sizeof(accept_over.over));
    accept_over.op = OP_ACCEPT;

    AcceptEx(l_socket, c_socket, accept_over.io_buf, NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &accept_over.over);

    while (true) {

        DWORD io_byte;
        ULONG_PTR key;
        WSAOVERLAPPED* over;

        GetQueuedCompletionStatus(g_iocp, &io_byte, &key, &over, INFINITE);

        EXOVER* exover = reinterpret_cast<EXOVER*>(over);
        int user_id = static_cast<int>(key);
        CLIENT& cl = g_clients[user_id];

        switch (exover->op)
        {
        case OP_RECV: {
            process_packet(user_id, exover->io_buf);
            ZeroMemory(&cl.m_recv_over.over, sizeof(cl.m_recv_over.over));
            DWORD flags = 0;
            WSARecv(cl.m_s, &cl.m_recv_over.wsabuf, 1, NULL, &flags, &cl.m_recv_over.over, NULL);
        }
                    break;
        case OP_ACCEPT: {

            int user_id = g_current_user_id++;

            CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_iocp, user_id, 0);

            g_current_user_id = g_current_user_id % MAX_USER;
            CLIENT& nc = g_clients[user_id];
            nc.m_id = user_id;
            nc.m_prev_size = 0;
            nc.m_recv_over.op = OP_RECV;
            ZeroMemory(&nc.m_recv_over.over, sizeof(nc.m_recv_over.over));
            nc.m_recv_over.wsabuf.buf = nc.m_recv_over.io_buf;
            nc.m_recv_over.wsabuf.len = MAX_BUF_SIZE;
            nc.m_s = c_socket;
            nc.x = rand() % WORLD_WIDTH;
            nc.x = rand() % WORLD_HEIGHT;
            DWORD flags = 0;

            WSARecv(c_socket, &nc.m_recv_over.wsabuf, 1, NULL, &flags, &nc.m_recv_over.over, NULL);

            c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
            ZeroMemory(&accept_over.over, sizeof(accept_over.over));

            AcceptEx(l_socket, c_socket, accept_over.io_buf, NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &accept_over.over);

        }

                      break;
        case OP_SEND:
            delete exover;
            break;

        }
    }
}