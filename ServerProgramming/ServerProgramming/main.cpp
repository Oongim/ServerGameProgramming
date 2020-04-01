#include"global.h"
#include"SceneManager.h"

SceneManager* g_ScnMgr;
TextureManager* TextureManager::s_instance = nullptr;
// ������ ��� �Լ�
GLvoid RenderScene(int temp)
{
	static int PrevTime{ 0 };
	int CurrTime = glutGet(GLUT_ELAPSED_TIME);
	int ElapsedTime = CurrTime - PrevTime;
	PrevTime = CurrTime;

	float ElapsedTimeInSec = (float)ElapsedTime / 1000.f;

	g_ScnMgr->Update(ElapsedTimeInSec);
	g_ScnMgr->Render(ElapsedTimeInSec);

	glutSwapBuffers();
	glutTimerFunc(10, RenderScene, 0);

}
GLvoid drawScene(GLvoid)
{
}
GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, w, h);
	gluPerspective(60.0f, w / h, 1.0, 2000.0);
	glTranslatef(0.0, 0.0, -500.0);
	glMatrixMode(GL_MODELVIEW);
}

void Keyboard(unsigned char key, int x, int y)
{
	g_ScnMgr->KeyDown(key, x, y);
}
void KeyboardUp(unsigned char key, int x, int y)
{
	g_ScnMgr->KeyUp(key, x, y);
}

void SpecialKeyDownInput(int key, int x, int y)
{
	g_ScnMgr->SpecialKeyDown(key, x, y);
}

void SpecialKeyUpInput(int key, int x, int y)
{
	g_ScnMgr->SpecialKeyUp(key, x, y);
}

int main(int argc, char* argv[])
{
	//�ʱ�ȭ �Լ���
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH); // ���÷��� ��� ����
	glutInitWindowPosition(100, 100); // �������� ��ġ����
	glutInitWindowSize(800, 800); // �������� ũ�� ����
	glutCreateWindow("GameServerProgramming"); // ������ ���� (������ �̸�)
	/////////////////////////////////////////////////////////////////////

	g_ScnMgr = new SceneManager;

	/////////////////////////////////////////////////////////////////////
	glutDisplayFunc(drawScene); 

	glutKeyboardFunc(Keyboard);                  // Ű���� �Է� �ݹ� �Լ�
	glutKeyboardUpFunc(KeyboardUp);                  // Ű���� �Է� �ݹ� �Լ�
	glutSpecialFunc(SpecialKeyDownInput);
	glutSpecialUpFunc(SpecialKeyUpInput);


	glutTimerFunc(10, RenderScene, 1);
	glutReshapeFunc(Reshape); // �ٽ� �׸��� �Լ��� ����
	

	glutMainLoop();

	// �Ҹ��ڴ� �̰���
	delete g_ScnMgr;
}
