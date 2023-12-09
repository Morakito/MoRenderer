#ifndef WINDOW_H
#define WINDOW_H

#include <map>
#include <Windows.h>
#include "math.h"

struct Mouse
{
	Vec2f mouse_position;			// ��굱ǰλ��
	Vec2f mouse_delta;				// ��굱ǰλ�õı仯��
	float mouse_wheel_delta;		// �����ֵı仯��
};

class Window
{
public:
	Window() = default;
	~Window() = default;
	Window(const Window& window) = delete;
	Window& operator=(const Window& window) = delete;
	static Window* GetInstance();


	void WindowInit(int width, int height, const char* title);
	void WindowDestroy();

	void WindowDisplay(const uint8_t* frame_buffer);

	Vec2f GetMousePosition() const;
	void SetLogMessage(const std::string&, const std::string&);
	void RemoveLogMessage(const std::string& log_type);

private:

	void WindowDrawFrame(const uint8_t* frame_buffer) const;

	void UpdateFpsData();
	static void RegisterWindowClass(const char* title);
	static float GetNativeTime();
	static float PlatformGetTime();
	static void MessageDispatch();
	static void InitBitmapHeader(BITMAPINFOHEADER& bitmap, const int width, const int height);



public:
	int width_;							// ���ڿ��
	int height_;						// ���ڸ߶�

	char keys_[512];					// ���̰���
	char mouse_buttons_[3];				// 0-��������1-����Ҽ���2-������	
	bool is_close_;						// �����Ƿ�ر�
	Mouse mouse_info_;					// �����Ϣ

	bool can_press_keyboard_;

private:
	HWND hwnd_;
	HDC memory_dc_;
	HBITMAP bitmap_old_;
	HBITMAP bitmap_dib_;
	uint8_t* frame_buffer_;				// �������

	std::map<std::string, std::string> log_messages_;	// ��־��Ϣ����
	int num_frames_per_second_ = 0;						// һ���ڵ�֡��
	float last_frame_time_;								// һ֡��ʼ��Ⱦ��ʱ��
	float current_frame_time_;							// һ֡������Ⱦ��ʱ��

	static Window* window_;
};


#endif // !WINDOW_H
