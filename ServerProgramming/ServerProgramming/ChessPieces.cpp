#include "ChessPieces.h"

ChessPieces::ChessPieces(Position pos , int texture): m_pos(pos), m_texture(texture)
{
	s_Resource = TextureManager::Getinstance();
	m_texture=s_Resource->GenPngTexture("resource/ChessKing.png");
}

ChessPieces::~ChessPieces()
{
	m_texture = -1;
}

void ChessPieces::ClampPosition()
{
	m_pos.x = clamp(m_pos.x, (float)-NUM_OF_CHESSBOARDLINE/2+0.5f, (float)NUM_OF_CHESSBOARDLINE / 2-0.5f);
	m_pos.y = clamp(m_pos.y, (float)-NUM_OF_CHESSBOARDLINE/2+0.5f, (float)NUM_OF_CHESSBOARDLINE / 2-0.5f);
	m_pos.z = clamp(m_pos.z, (float)-NUM_OF_CHESSBOARDLINE/2+0.5f, (float)NUM_OF_CHESSBOARDLINE / 2-0.5f);
}

void ChessPieces::Update()
{
	ClampPosition();
}

void ChessPieces::Draw()
{
	glTranslatef(m_pos.x * (float)SIZE_OF_ONE_CHESS_SQUARE, m_pos.y * (float)SIZE_OF_ONE_CHESS_SQUARE, m_pos.z);
	DrawTexture_Plane(50, s_Resource->GetTexture(m_texture),
		1.0, 1.0, 1.0, 1.0);
}

Position ChessPieces::GetPos() const
{
	return m_pos;
}

void ChessPieces::SetPos(const Position pos)
{
	m_pos = pos;
}

void ChessPieces::AddPos(const Position pos)
{
	m_pos +=pos;
}

int ChessPieces::GetTexture() const
{
	return m_texture;
}

void ChessPieces::SetTexture(const int texture)
{
	m_texture = texture;
}
