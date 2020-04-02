#pragma once

#include <map>	 //������ Ŭ���̾�Ʈ�� ����Ǿ���ϴ� �����̳ʰ� �ʿ�, ���͵� �ǰ����� ���ð��� ������ �ʿ������ ���̴� ����� ��
#include <WS2tcpip.h> //�ϳ��� ����ϰ� �� �ذ��

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <math.h>
#include <assert.h>
using namespace std;

#pragma comment(lib,"winmm.lib")
#pragma comment(lib, "Ws2_32.lib")//�Ӽ� ������Ƽ�� �־������ �ξ� ���
#pragma warning(disable:4996)
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS

//���� ����
#define SERVER_PORT 9000
#define MAX_BUFFER 1024

#define MAX_PLAYER 3

#define SIZE_OF_ONE_CHESS_SQUARE 70.f
#define NUM_OF_CHESSBOARDLINE 8.f
/********************************************************************************************/
#pragma pack(1)
struct Position {
	float x, y, z;

	Position& operator =(const Position& a)
	{
		x = a.x;
		y = a.y;
		z = a.z;
		return *this;
	}

	Position& operator+(const Position& p1) {
		x += p1.x;
		y += p1.y;
		z += p1.z;
		return *this;
	}
	Position& operator+=(const Position& p1) {
		*this = *this + p1;
		return *this;
	}
	Position operator-(Position p1) {
		Position ret;
		ret.x = +p1.x;
		ret.y = +p1.y;
		return ret;
	}

};
#pragma pack()
#pragma pack(1)
struct KeyInput
{
	bool Up;
	bool Down;
	bool Left;
	bool Right;
	bool Attack;
};
#pragma pack()
#pragma pack(1)
struct FixedData
{
	char NumOfClient;
};
#pragma pack()
#pragma pack(1)
struct CharacterStatus
{
	Position position;
	char whoseControlNum;
};
#pragma pack()

template<class T>
inline const T& clamp(const T& var, const T& low, const T& high)
{
	assert(high > low);
	if (var <= low) return low;
	else if (var >= high)return high;
	else return var;
}

//////////////////////////////////////
struct SOCKETINFO         //Ŭ���̾�Ʈ ���� ���� Ŭ��� ��Ʈ��ũ�� �����͸� �ְ� ������ �ʿ��� �ΰ��ڷ�� 
{
	WSAOVERLAPPED overlapped;	//recv �������� ���̰� ���� ����µ� send, recv ���� �����̴�. ���� �� ��ġ�� ���� ����
	WSABUF dataBuffer[3];			//wasbuf ���� ������ ���۰� �ƴ϶� ���۵��� �����ϴ� ����, �迭�� �Ǿ�� ������ ������ �ϳ��� ���Ŵ�
	SOCKET socket;

	KeyInput key;
	char NumOfClient;
	CharacterStatus stat;
};
///////////////////////////////////
void err_quit(char* msg)
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

// ���� �Լ� ���� ���
void err_display(char* msg)
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