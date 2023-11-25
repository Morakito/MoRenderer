#pragma once
#include <Windows.h>

#include "math.h"

typedef struct mouse
{
	// for camera orbit
	Vec2f orbit_pos;
	Vec2f orbit_delta;
	// for first-person view (diabled now)
	Vec2f fv_pos;
	Vec2f fv_delta;
	// for mouse wheel
	float wheel_delta;
}mouse_t;

typedef struct window 
{
	HWND h_window;
	HDC mem_dc;
	HBITMAP bm_old;
	HBITMAP bm_dib;
	unsigned char *window_fb;
	int width;
	int height;
	char keys[512];
	char buttons[2];	//left button¡ª0£¬ right button¡ª1
	int is_close;
	mouse_t mouse_info;
}window_t;

extern window_t* window;

int window_init(int width, int height, const char *title);
int window_destroy();
void window_draw(unsigned char *framebuffer, std::string logMessage);
void msg_dispatch();
Vec2f get_mouse_pos();
float platform_get_time(void);
