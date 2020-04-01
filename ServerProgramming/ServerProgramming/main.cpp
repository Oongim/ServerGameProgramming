#include"global.h"
#include"SceneManager.h"

SceneManager* g_ScnMgr;
TextureManager* TextureManager::s_instance = nullptr;
// 윈도우 출력 함수
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
	//초기화 함수들
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	glutInitWindowPosition(100, 100); // 윈도우의 위치지정
	glutInitWindowSize(800, 800); // 윈도우의 크기 지정
	glutCreateWindow("GameServerProgramming"); // 윈도우 생성 (윈도우 이름)
	/////////////////////////////////////////////////////////////////////

	g_ScnMgr = new SceneManager;

	/////////////////////////////////////////////////////////////////////
	glutDisplayFunc(drawScene); 

	glutKeyboardFunc(Keyboard);                  // 키보드 입력 콜백 함수
	glutKeyboardUpFunc(KeyboardUp);                  // 키보드 입력 콜백 함수
	glutSpecialFunc(SpecialKeyDownInput);
	glutSpecialUpFunc(SpecialKeyUpInput);


	glutTimerFunc(10, RenderScene, 1);
	glutReshapeFunc(Reshape); // 다시 그리기 함수의 지정
	

	glutMainLoop();

	// 소멸자는 이곳에
	delete g_ScnMgr;
}
