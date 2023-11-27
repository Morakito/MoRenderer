#ifndef WIN32_H
#define WIN32_H

#include <Windows.h>

#include "math.h"

typedef struct mouse
{
	Vec2f mousePosition;			// 鼠标当前位置
	Vec2f mouseDelta;				// 鼠标当前位置的变化量
	float mouseWheelDelta;			// 鼠标滚轮的变化量
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

	// 0-鼠标左键，1-鼠标右键，2-鼠标滚轮
	char mouseButtons[3];	//left button—0， right button—1
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
