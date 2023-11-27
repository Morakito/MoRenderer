#ifndef WIN32_H
#define WIN32_H

#include <Windows.h>

#include "math.h"

typedef struct mouse
{
	Vec2f mousePosition;			// ��굱ǰλ��
	Vec2f mouseDelta;				// ��굱ǰλ�õı仯��
	float mouseWheelDelta;			// �����ֵı仯��
}mouse_t;

typedef struct window
{
	HWND h_window;
	HDC mem_dc;
	HBITMAP bm_old;
	HBITMAP bm_dib;
	unsigned char* window_fb;
	int width;
	int height;
	char keys[512];

	// 0-��������1-����Ҽ���2-������
	char mouseButtons[3];	//left button��0�� right button��1
	bool is_close;
	mouse_t mouse_info;
}window_t;

extern window_t* window;

int window_init(int width, int height, const char* title);
int window_destroy();
void window_draw(unsigned char* framebuffer, std::string logMessage);
void msg_dispatch();
Vec2f GetMousePosition();
float platform_get_time(void);


#endif // !WIN32_H
