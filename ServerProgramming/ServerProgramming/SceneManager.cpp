#include "SceneManager.h"
#include "ChessPieces.h"
#include "TCPClient.h"

SceneManager::SceneManager()
{
	Initialize();
}

SceneManager::~SceneManager()
{
}

void SceneManager::Initialize()
{
	s_Resource = TextureManager::Getinstance();
	Chess_King = new ChessPieces({ 0.5,0.5,0.f });
	Chess_Board = s_Resource->GenPngTexture("resource/Chessboard.png");

	m_client = new TCPClient();
}

void SceneManager::Update(float time)
{
	m_client->PlaySceneSendData(m_key);
	m_client->PlaySceneRecvData(m_players);

	isPressedKey = false;
	m_key.Up = false;
	m_key.Down = false;
	m_key.Right = false;
	m_key.Left = false;
}

void SceneManager::Render(float time)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // 바탕색을 'blue' 로 지정
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 설정된 색으로 전체를 칠하기
	glEnable(GL_DEPTH_TEST | GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPushMatrix();//=========================

	glLoadIdentity();
	gluLookAt(
		0, 0, 100,
		0.0, 0.0, 0.0,
		0.0, 1, 0.0);
	glMultMatrixd(WorldRotate);


	DrawTexture_Plane(600, s_Resource->GetTexture(Chess_Board),
		1.0, 1.0, 1.0, 1.0);
	glPushMatrix();//=========================
	{
		for (auto player : m_players)
		{
			Chess_King->SetPos(player.position);
			Chess_King->Draw();
		}
	}
	glPopMatrix();//=========================
	glPopMatrix();//=========================
	glFlush(); // 화면에 출력하기
	glutSwapBuffers(); // 화면에 출력하기
}

void SceneManager::KeyDown(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':
		glPushMatrix();
		glRotatef(5, 1.0, 0.0, 0.0);
		glMultMatrixd(WorldRotate);
		glGetDoublev(GL_MODELVIEW_MATRIX, WorldRotate);
		glPopMatrix();
		break;
	case 'Q':
		glPushMatrix();
		glRotatef(-5, 1.0, 0.0, 0.0);
		glMultMatrixd(WorldRotate);
		glGetDoublev(GL_MODELVIEW_MATRIX, WorldRotate);
		glPopMatrix();
		break;
	case 'e':
		glPushMatrix();
		glRotatef(-5, 0.0, 1.0, 0.0);
		glMultMatrixd(WorldRotate);
		glGetDoublev(GL_MODELVIEW_MATRIX, WorldRotate);
		glPopMatrix();
		break;
	case 'E':
		glPushMatrix();
		glRotatef(5, 0.0, 1.0, 0.0);
		glMultMatrixd(WorldRotate);
		glGetDoublev(GL_MODELVIEW_MATRIX, WorldRotate);
		glPopMatrix();
		break;
	}
}

void SceneManager::KeyUp(unsigned char key, int x, int y)
{
	switch (key) {
	}
}

void SceneManager::SpecialKeyDown(int key, int x, int y)
{
	static float speed = 1.f;

	if (!isPressedKey) {
		switch (key) {
		case GLUT_KEY_UP:
			m_key.Up = true;
			break;
		case GLUT_KEY_DOWN:
			m_key.Down = true;
			break;
		case GLUT_KEY_LEFT:
			m_key.Left = true;
			break;
		case GLUT_KEY_RIGHT:
			m_key.Right = true;
			break;
		}
		isPressedKey = true;
	}
}

void SceneManager::SpecialKeyUp(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_UP:
		//m_key.Up = false;
		break;
	case GLUT_KEY_DOWN:
		//m_key.Down = false;
		break;
	case GLUT_KEY_LEFT:
		//m_key.Left = false;
		break;
	case GLUT_KEY_RIGHT:
		//m_key.Right = false;
		break;
	}
}
