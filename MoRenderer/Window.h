#ifndef WINDOW_H
#define WINDOW_H

#include <map>
#include <Windows.h>
#include "math.h"

struct Mouse
{
	Vec2f mouse_position;			// ��굱ǰλ��
	Vec2f mouse_delta;				// ��굱ǰλ�õı仯��
	float mouse_wheel_delta;			// �����ֵı仯��
};

class Window
{
public:
	Window() {}
	~Window() {};
	Window(const Window& window) = delete;
	Window& operator=(const Window& window) = delete;

	static Window* GetInstance();

public:
	void WindowInit(int width, int height, const char* title);
	void WindowDestroy();
	void WindowDisplay(const uint8_t* frame_buffer, const std::map<std::string, std::string>& log_messages) const;

	Vec2f GetMousePosition() const;

public:

	static void RegisterWindowClass(const char* title);
	static float GetNativeTime();
	static float PlatformGetTime();
	static void MessageDispatch();
	static void InitBitmapHeader(BITMAPINFOHEADER& bitmap, const int width, const int height);

public:
	HWND hwnd_;
	HDC memory_dc_;
	HBITMAP bitmap_old_;
	HBITMAP bitmap_dib_;
	uint8_t* frame_buffer_;		// �������
	int width_;							// ���ڿ��
	int height_;						// ���ڸ߶�


	char keys_[512];					// ���̰���
	char mouse_buttons_[3];				// 0-��������1-����Ҽ���2-������	
	bool is_close_;						// �����Ƿ�ر�
	Mouse mouse_info_;					// �����Ϣ

	static Window* window_;
};


#endif // !WINDOW_H
