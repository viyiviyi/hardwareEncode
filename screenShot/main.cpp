#include "screenShot.h"

int main(int argc, char* argv[])
{
	int len;

	VideoDXGICaptor *screenShot = new VideoDXGICaptor();

	screenShot->SetDisPlay(1);

	screenShot->Init();

	while (1)
	{

		screenShot->QueryFrame2(*(&len));

		printf("W = %d , H = %d \n", screenShot->m_iWidth, screenShot->m_iHeight);

		Sleep(30);
	}

	system("pause");

	return 0;
}