#include "Window.h"

#include <cassert>
#include <cstdio>
#include <ranges>

Window* Window::window_ = nullptr;

Window* Window::GetInstance()
{
	if (window_ == nullptr) {
		window_ = new Window();
	}
	return window_;
}

void Window::WindowInit(const int width, const int height, const char* title)
{
	is_close_ = false;

	LPVOID frame_buffer_ptr;
	BITMAPINFOHEADER bitmap_info_header;

	// 注册窗口类
	RegisterWindowClass(title);

	// 创建窗口
	hwnd_ = CreateWindow(title, title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

	// 初始化位图头格式
	InitBitmapHeader(bitmap_info_header, width, height);

	// 获得兼容性DC
	const HDC hdc = GetDC(hwnd_);
	memory_dc_ = CreateCompatibleDC(hdc);
	ReleaseDC(hwnd_, hdc);

	// 创建位图
	bitmap_dib_ = CreateDIBSection(memory_dc_, reinterpret_cast<BITMAPINFO*>(&bitmap_info_header),
		DIB_RGB_COLORS, &frame_buffer_ptr, nullptr, 0); //创建设备无关句柄

	bitmap_old_ = static_cast<HBITMAP>(SelectObject(memory_dc_, bitmap_dib_));//把新创建的位图句柄写入memory_dc_
	frame_buffer_ = static_cast<unsigned char*>(frame_buffer_ptr);

	width_ = width;
	height_ = height;

	// 一个矩形范围 左上右下
	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, GetWindowLong(hwnd_, GWL_STYLE), 0);//调整窗口大小
	const int wx = rect.right - rect.left;
	const int wy = rect.bottom - rect.top;
	const int sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2;	// GetSystemMetrics(SM_CXSCREEN)获取你屏幕的分辨率
	int sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2;		// 计算出中心位置
	if (sy < 0) sy = 0;

	SetWindowPos(hwnd_, nullptr, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(hwnd_);
	ShowWindow(hwnd_, SW_NORMAL);

	// 消息分派
	MessageDispatch();

	// 初始化keys, window_fb全为0
	memset(frame_buffer_, 0, width_ * height_ * 4);
	memset(keys_, 0, sizeof(char) * 512);


	// 初始化LOG信息
	num_frames_per_second_ = 0;
	current_frame_time_ = PlatformGetTime();
}

void Window::WindowDestroy()
{
	if (memory_dc_)
	{
		if (bitmap_old_)
		{
			SelectObject(memory_dc_, bitmap_old_); // 写入原来的bitmap，才能释放DC！
			bitmap_old_ = nullptr;
		}
		DeleteDC(memory_dc_);
		memory_dc_ = nullptr;
	}
	if (bitmap_dib_)
	{
		DeleteObject(bitmap_dib_);
		bitmap_dib_ = nullptr;
	}
	if (hwnd_)
	{
		CloseWindow(hwnd_);
		hwnd_ = nullptr;
	}

	free(window_);
}

void Window::WindowDisplay(const uint8_t* frame_buffer) 
{
	UpdateFpsData();
	WindowDrawFrame(frame_buffer);
	Window::MessageDispatch();
}

LRESULT MessageCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Window* window = Window::GetInstance();
	switch (msg)
	{
	case WM_CLOSE:
		window->is_close_ = TRUE;
		break;
	case WM_KEYDOWN:
		window->keys_[wParam & 511] = 1;
		break;
	case WM_KEYUP:
		window->keys_[wParam & 511] = 0;
		break;
	case WM_LBUTTONDOWN:
		window->mouse_info_.mouse_position = window->GetMousePosition();
		window->mouse_buttons_[0] = 1;
		break;
	case WM_LBUTTONUP:
		window->mouse_buttons_[0] = 0;
		break;
	case WM_RBUTTONDOWN:
		window->mouse_info_.mouse_position = window->GetMousePosition();
		window->mouse_buttons_[1] = 1;
		break;
	case WM_RBUTTONUP:
		window->mouse_buttons_[1] = 0;
		break;
	case WM_MOUSEWHEEL:
		window->mouse_buttons_[2] = 1;
		window->mouse_info_.mouse_wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA);
		break;

	default: return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}


void Window::RegisterWindowClass(const char* title)
{
	//初始化结构体
	WNDCLASS wc;
	wc.style = CS_BYTEALIGNCLIENT;											//窗口风格
	wc.lpfnWndProc = static_cast<WNDPROC>(MessageCallback);					//回调函数
	wc.cbClsExtra = 0;														//紧跟在窗口类尾部的一块额外空间，不用则设为0
	wc.cbWndExtra = 0;														//紧跟在窗口实例尾部的一块额外空间，不用则设为0
	wc.hInstance = GetModuleHandle(nullptr);								//当前实例句柄
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);					//任务栏图标
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);					//光标样式
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));	//背景样式
	wc.lpszMenuName = nullptr;												//菜单
	wc.lpszClassName = title;												//该窗口类的名字

	ATOM atom = RegisterClass(&wc);											//注册窗口类
	assert(atom != 0);
}


void Window::InitBitmapHeader(BITMAPINFOHEADER& bitmap, const int width, const int height)
{
	memset(&bitmap, 0, sizeof(BITMAPINFOHEADER));
	bitmap.biSize = sizeof(BITMAPINFOHEADER);		//本结构所占用的字节数
	bitmap.biWidth = width;							//bitmap宽度
	bitmap.biHeight = height;						//bitmap高度，原点位于左下角
	bitmap.biPlanes = 1;							//目标设备级别
	bitmap.biBitCount = 32;							//bitmap中一个颜色所占据的bit数
	bitmap.biCompression = BI_RGB;					//是否压缩，BI_RGB为不压缩
	bitmap.biSizeImage = width * height * 4;		//整个bitmap所占据的字节数
}


void Window::MessageDispatch()
{
	MSG message;
	while (true) {
		// Peek不阻塞，Get会阻塞，PM_NOREMOVE表示如果有消息不处理（留给接下来的Get处理）
		if (!PeekMessage(&message, nullptr, 0, 0, PM_NOREMOVE)) break;
		if (!GetMessage(&message, nullptr, 0, 0)) break;

		TranslateMessage(&message);	 //转换消息 虚拟按键->字符
		DispatchMessage(&message); //传送消息给回调
	}
}

void Window::WindowDrawFrame(const uint8_t* frame_buffer) const
{
	memcpy(frame_buffer_, frame_buffer, width_ * height_ * 4);

	//显示Log信息
	if (!log_messages_.empty()) {
		LOGFONT log_font;								//改变输出字体
		ZeroMemory(&log_font, sizeof(LOGFONT));
		log_font.lfCharSet = ANSI_CHARSET;
		log_font.lfHeight = 20;							//设置字体的大小
		const HFONT h_font = CreateFontIndirect(&log_font);

		//目标矩形的左上角(x,y), 宽度，高度，上下文指针
		SelectObject(memory_dc_, h_font);
		SetTextColor(memory_dc_, RGB(190, 190, 190));
		SetBkColor(memory_dc_, RGB(80, 80, 80));

		int log_index = 1;
		for (auto const value : log_messages_ | std::views::values)
		{
			TextOut(memory_dc_, 20, 20 * (log_index++),
				value.c_str(),
				strlen(value.c_str()));
		}
	}

	//绘制frame buffer
	const HDC hdc = GetDC(hwnd_);
	// 把兼容性DC的数据传到真正的DC上
	BitBlt(hdc, 0, 0, width_, height_, memory_dc_, 0, 0, SRCCOPY);
	ReleaseDC(hwnd_, hdc);
}

Vec2f Window::GetMousePosition() const
{
	POINT mouse_point;
	GetCursorPos(&mouse_point);

	// 将鼠标位置从屏幕空间转到窗口空间
	ScreenToClient(hwnd_, &mouse_point);
	auto mouse_position = Vec2f(static_cast<float>(mouse_point.x), static_cast<float>(mouse_point.y));
	return mouse_position;
}

void Window::SetLogMessage(const std::string& log_type, const std::string& log_content)
{
	log_messages_[log_type] = log_content;
}


void Window::UpdateFpsData()
{
	num_frames_per_second_ += 1;
	current_frame_time_ = PlatformGetTime();
	if (current_frame_time_ - last_frame_time_ >= 1) {
		const int frame_time_ms = static_cast<int>((current_frame_time_ - last_frame_time_) * 1000);
		const int average_frame_time_ms = frame_time_ms / num_frames_per_second_;

		const std::string fps_message = "FPS: " + std::to_string(num_frames_per_second_) + " / " + std::to_string(average_frame_time_ms) + " ms";

		SetLogMessage("fps_message", fps_message);
		num_frames_per_second_ = 0;
		last_frame_time_ = current_frame_time_;
	}
}

float Window::GetNativeTime() {
	static float period = -1;
	LARGE_INTEGER counter;
	if (period < 0) {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		period = 1 / static_cast<double>(frequency.QuadPart);
	}
	QueryPerformanceCounter(&counter);
	return period * counter.QuadPart;
}

float Window::PlatformGetTime() {
	static float initial = -1;
	if (initial < 0) {
		initial = GetNativeTime();
	}
	return GetNativeTime() - initial;
}

