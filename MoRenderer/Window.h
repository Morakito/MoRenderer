#ifndef WINDOW_H
#define WINDOW_H

#include <map>
#include <Windows.h>
#include "math.h"

struct Mouse
{
	Vec2f mouse_position;			// 鼠标当前位置
	Vec2f mouse_delta;				// 鼠标当前位置的变化量
	float mouse_wheel_delta;		// 鼠标滚轮的变化量
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
	int width_;							// 窗口宽度
	int height_;						// 窗口高度

	char keys_[512];					// 键盘按键
	char mouse_buttons_[3];				// 0-鼠标左键，1-鼠标右键，2-鼠标滚轮	
	bool is_close_;						// 窗口是否关闭
	Mouse mouse_info_;					// 鼠标信息

	bool can_press_keyboard_;

private:
	HWND hwnd_;
	HDC memory_dc_;
	HBITMAP bitmap_old_;
	HBITMAP bitmap_dib_;
	uint8_t* frame_buffer_;				// 输出内容

	std::map<std::string, std::string> log_messages_;	// 日志信息数据
	int num_frames_per_second_ = 0;						// 一秒内的帧数
	float last_frame_time_;								// 一帧开始渲染的时刻
	float current_frame_time_;							// 一帧结束渲染的时刻

	static Window* window_;
};


#endif // !WINDOW_H
