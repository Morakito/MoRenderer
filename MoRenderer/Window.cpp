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

	// ע�ᴰ����
	RegisterWindowClass(title);

	// ��������
	hwnd_ = CreateWindow(title, title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

	// ��ʼ��λͼͷ��ʽ
	InitBitmapHeader(bitmap_info_header, width, height);

	// ��ü�����DC
	const HDC hdc = GetDC(hwnd_);
	memory_dc_ = CreateCompatibleDC(hdc);
	ReleaseDC(hwnd_, hdc);

	// ����λͼ
	bitmap_dib_ = CreateDIBSection(memory_dc_, reinterpret_cast<BITMAPINFO*>(&bitmap_info_header),
		DIB_RGB_COLORS, &frame_buffer_ptr, nullptr, 0); //�����豸�޹ؾ��

	bitmap_old_ = static_cast<HBITMAP>(SelectObject(memory_dc_, bitmap_dib_));//���´�����λͼ���д��memory_dc_
	frame_buffer_ = static_cast<unsigned char*>(frame_buffer_ptr);

	width_ = width;
	height_ = height;

	// һ�����η�Χ ��������
	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, GetWindowLong(hwnd_, GWL_STYLE), 0);//�������ڴ�С
	const int wx = rect.right - rect.left;
	const int wy = rect.bottom - rect.top;
	const int sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2;	// GetSystemMetrics(SM_CXSCREEN)��ȡ����Ļ�ķֱ���
	int sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2;		// ���������λ��
	if (sy < 0) sy = 0;

	SetWindowPos(hwnd_, nullptr, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(hwnd_);
	ShowWindow(hwnd_, SW_NORMAL);

	// ��Ϣ����
	MessageDispatch();

	// ��ʼ��keys, window_fbȫΪ0
	memset(frame_buffer_, 0, width_ * height_ * 4);
	memset(keys_, 0, sizeof(char) * 512);


	// ��ʼ��LOG��Ϣ
	num_frames_per_second_ = 0;
	current_frame_time_ = PlatformGetTime();
	can_press_keyboard_ = false;
}

void Window::WindowDestroy()
{
	if (memory_dc_)
	{
		if (bitmap_old_)
		{
			SelectObject(memory_dc_, bitmap_old_); // д��ԭ����bitmap�������ͷ�DC��
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
	MessageDispatch();
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
	//��ʼ���ṹ��
	WNDCLASS wc;
	wc.style = CS_BYTEALIGNCLIENT;											//���ڷ��
	wc.lpfnWndProc = static_cast<WNDPROC>(MessageCallback);					//�ص�����
	wc.cbClsExtra = 0;														//�����ڴ�����β����һ�����ռ䣬��������Ϊ0
	wc.cbWndExtra = 0;														//�����ڴ���ʵ��β����һ�����ռ䣬��������Ϊ0
	wc.hInstance = GetModuleHandle(nullptr);								//��ǰʵ�����
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);					//������ͼ��
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);					//�����ʽ
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));	//������ʽ
	wc.lpszMenuName = nullptr;												//�˵�
	wc.lpszClassName = title;												//�ô����������

	ATOM atom = RegisterClass(&wc);											//ע�ᴰ����
	assert(atom != 0);
}


void Window::InitBitmapHeader(BITMAPINFOHEADER& bitmap, const int width, const int height)
{
	memset(&bitmap, 0, sizeof(BITMAPINFOHEADER));
	bitmap.biSize = sizeof(BITMAPINFOHEADER);		//���ṹ��ռ�õ��ֽ���
	bitmap.biWidth = width;							//bitmap���
	bitmap.biHeight = height;						//bitmap�߶ȣ�ԭ��λ�����½�
	bitmap.biPlanes = 1;							//Ŀ���豸����
	bitmap.biBitCount = 32;							//bitmap��һ����ɫ��ռ�ݵ�bit��
	bitmap.biCompression = BI_RGB;					//�Ƿ�ѹ����BI_RGBΪ��ѹ��
	bitmap.biSizeImage = width * height * 4;		//����bitmap��ռ�ݵ��ֽ���
}


void Window::MessageDispatch()
{
	MSG message;
	while (true) {
		// Peek��������Get��������PM_NOREMOVE��ʾ�������Ϣ������������������Get����
		if (!PeekMessage(&message, nullptr, 0, 0, PM_NOREMOVE)) break;
		if (!GetMessage(&message, nullptr, 0, 0)) break;

		TranslateMessage(&message);	 //ת����Ϣ ���ⰴ��->�ַ�
		DispatchMessage(&message); //������Ϣ���ص�
	}
}

void Window::WindowDrawFrame(const uint8_t* frame_buffer) const
{
	memcpy(frame_buffer_, frame_buffer, width_ * height_ * 4);

	//��ʾLog��Ϣ
	if (!log_messages_.empty()) {
		LOGFONT log_font;								//�ı��������
		ZeroMemory(&log_font, sizeof(LOGFONT));
		log_font.lfCharSet = ANSI_CHARSET;
		log_font.lfHeight = 20;							//��������Ĵ�С
		const HFONT h_font = CreateFontIndirect(&log_font);

		//Ŀ����ε����Ͻ�(x,y), ��ȣ��߶ȣ�������ָ��
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

	//����frame buffer
	const HDC hdc = GetDC(hwnd_);
	// �Ѽ�����DC�����ݴ���������DC��
	BitBlt(hdc, 0, 0, width_, height_, memory_dc_, 0, 0, SRCCOPY);
	ReleaseDC(hwnd_, hdc);
}

Vec2f Window::GetMousePosition() const
{
	POINT mouse_point;
	GetCursorPos(&mouse_point);

	// �����λ�ô���Ļ�ռ�ת�����ڿռ�
	ScreenToClient(hwnd_, &mouse_point);
	auto mouse_position = Vec2f(static_cast<float>(mouse_point.x), static_cast<float>(mouse_point.y));
	return mouse_position;
}

void Window::SetLogMessage(const std::string& log_type, const std::string& log_content)
{
	log_messages_[log_type] = log_content;
}

void Window::RemoveLogMessage(const std::string& log_type)
{
	log_messages_.erase(log_type);
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

		can_press_keyboard_ = true;
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

