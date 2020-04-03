#pragma once
#include"global.h"


class SceneManager
{
private:
	class TCPClient* m_client;

	class TextureManager* s_Resource;
	class ChessPieces* Chess_King;
	PACKET m_players;
	int Chess_Board = -1;

	GLdouble WorldRotate[16]
		= { 1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1 };

	KeyInput m_key;

	bool isPressedKey;

public:
	SceneManager();
	~SceneManager();

public:
	void Initialize();
	void Update(float time);
	void Render(float time);
	void KeyDown(unsigned char key, int x, int y);
	void KeyUp(unsigned char key, int x, int y);
	void SpecialKeyDown(int key, int x, int y);
	void SpecialKeyUp(int key, int x, int y);

};
