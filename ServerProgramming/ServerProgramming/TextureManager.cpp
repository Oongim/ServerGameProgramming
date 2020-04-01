#include "TextureManager.h"

TextureManager::TextureManager()
{
	//Initialize Texture arrays
	for (int i = 0; i < MAX_TEXTURES; i++)
	{
		m_Textures[i] = -1;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 670, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, pBytes);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GL_MODULATE);
}

TextureManager::~TextureManager()
{
}

GLubyte* TextureManager::LoadDIBitmap(const char* filename, BITMAPINFO** info)
{
	FILE* fp;
	GLubyte* bits;
	int bitsize, infosize;
	BITMAPFILEHEADER header;
	// 바이너리 읽기 모드로 파일을 연다
		
	fopen_s(&fp, filename, "rb");
	if (fp == NULL) {
		std::cout << "not" << std::endl;
		return NULL;
	}
	// 비트맵 파일 헤더를 읽는다.
	if (fread(&header, sizeof(BITMAPFILEHEADER), 1, fp) < 1) {
		fclose(fp);
		return NULL;
	}
	// 파일이 BMP 파일인지 확인한다.
	if (header.bfType != 'MB') {
		fclose(fp);
		return NULL;
	}
	// BITMAPINFOHEADER 위치로 간다.
	infosize = header.bfOffBits - sizeof(BITMAPFILEHEADER);
	// 비트맵 이미지 데이터를 넣을 메모리 할당을 한다.
	if ((*info = (BITMAPINFO*)malloc(infosize)) == NULL) {
		fclose(fp);
		exit(0);
		return NULL;
	}
	// 비트맵 인포 헤더를 읽는다.
	if (fread(*info, 1, infosize, fp) < (unsigned int)infosize) {
		free(*info);
		fclose(fp);
		return NULL;
	}
	// 비트맵의 크기 설정
	if ((bitsize = (*info)->bmiHeader.biSizeImage) == 0)
		bitsize = ((*info)->bmiHeader.biWidth * (*info)->bmiHeader.biBitCount + 7) / 8.0 * abs((*info)->bmiHeader.biHeight);
	// 비트맵의 크기만큼 메모리를 할당한다.
	if ((bits = (unsigned char*)malloc(bitsize)) == NULL) {
		free(*info);
		fclose(fp);
		return NULL;
	}
	// 비트맵 데이터를 bit(GLubyte 타입)에 저장한다.
	if (fread(bits, 1, bitsize, fp) < (unsigned int)bitsize) {
		free(*info); free(bits);
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	return bits;
}

int TextureManager::GenPngTexture(char* filePath, GLuint sampling)
{
	//find empty slot
	int idx = -1;
	for (int i = 0; i < MAX_TEXTURES; i++)
	{
		if (m_Textures[i] == -1)
		{
			idx = i;
			break;
		}
	}
	if (idx == -1)
	{
		std::cout << "Can't gen more textures." << std::endl;
		return -1;
	}

	//Load Pngs
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filePath);
	if (error != 0)
	{
		std::cout << "PNG Image Loading Failed : " << filePath << std::endl;
		assert(0);
	}

	GLuint temp;
	glGenTextures(1, &temp);

	if (temp < 0)
	{
		std::cout << "PNG Texture Creation Failed : " << filePath << std::endl;
		assert(0);
	}

	glBindTexture(GL_TEXTURE_2D, temp);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampling);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampling);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);

	m_Textures[idx] = temp;

	return idx;
}

int TextureManager::GetTexture(int index) const
{
	if (m_Textures[index] != NULL) {
		return m_Textures[index];
	}
	else return -1;
}

TextureManager* TextureManager::Getinstance()
{
	if (s_instance == nullptr)
	{
		s_instance = new TextureManager();
	}
	return s_instance;
	
}
