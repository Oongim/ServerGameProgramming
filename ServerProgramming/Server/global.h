#pragma once

#include <map>	 //여러개 클라이언트가 저장되어야하니 컨테이너가 필요, 벡터도 되겠지만 스택같은 구조가 필요없으니 맵이더 깔끔할 것
#include <WS2tcpip.h> //하나로 깔끔하게 다 해결됨

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <math.h>
#include <assert.h>
using namespace std;

#pragma comment(lib,"winmm.lib")
#pragma comment(lib, "Ws2_32.lib")//속성 프로퍼티에 넣어도되지만 훨씬 깔끔
#pragma warning(disable:4996)
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS

//전역 변수
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
struct SOCKETINFO         //클라이언트 들의 정보 클라와 네트워크로 데이터를 주고 받을때 필요한 부가자료들 
{
	WSAOVERLAPPED overlapped;	//recv 전용으로 쓰이게 많이 만드는데 send, recv 같이 쓸것이다. 서로 안 겹치니 재사용 가능
	WSABUF dataBuffer[3];			//wasbuf 실제 데이터 버퍼가 아니라 버퍼들을 관리하는 버퍼, 배열이 되어야 맞지만 예제니 하나만 쓸거니
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

// 소켓 함수 오류 출력
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