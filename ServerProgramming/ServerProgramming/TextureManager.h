#pragma once
#include "global.h"
#include "LoadPng.h"

#define MAX_TEXTURES 1000

class TextureManager
{
private:

	GLubyte* pBytes; // �����͸� ����ų ������
	BITMAPINFO* info; // ��Ʈ�� ��� ������ ����
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


