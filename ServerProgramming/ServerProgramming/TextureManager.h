#pragma once
#include "global.h"
#include "LoadPng.h"

#define MAX_TEXTURES 1000

class TextureManager
{
private:

	GLubyte* pBytes; // 데이터를 가리킬 포인터
	BITMAPINFO* info; // 비트맵 헤더 저장할 변수
	int m_Textures[MAX_TEXTURES];

	static TextureManager* s_instance;

public:
	TextureManager();
	~TextureManager();

public:
	GLubyte* LoadDIBitmap(const char* filename, BITMAPINFO** info);
	int GenPngTexture(char* filePath, GLuint sampling = GL_NEAREST);
	int GetTexture(int index) const;

	static TextureManager* Getinstance();

};


