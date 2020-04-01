#pragma once
#include	"global.h"

class ChessPieces
{
private:
	class TextureManager* s_Resource;

	Position m_pos;
	int m_texture;

public:
	ChessPieces(Position pos = {0,0,0}, int texture = -1);
	~ChessPieces();

private:
	void ClampPosition();

public:
	void Update();
	void Draw();

	Position GetPos() const;
	void SetPos(const Position pos);
	void AddPos(const Position pos);

	int GetTexture() const;
	void SetTexture(const int texture);

};

