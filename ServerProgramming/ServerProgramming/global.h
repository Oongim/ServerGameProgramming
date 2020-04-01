#pragma once
#include <GL/freeglut.h>
#include <vector>
#include <cstdlib>
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <map>
#include <WS2tcpip.h>
#include "Renderer.h"
#include "TextureManager.h"


/********************************************************************************************/
#define SIZE_OF_ONE_CHESS_SQUARE 70.f
#define NUM_OF_CHESSBOARDLINE 8.f

#define MAX_PLAYER 3
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