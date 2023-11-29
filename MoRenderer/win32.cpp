#include "win32.h"

#include <cassert>
#include <cstdio>

window_t* window = nullptr;

static LRESULT CALLBACK msg_callback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		window->is_close = TRUE;
		break;
	case WM_KEYDOWN:
		window->keys[wParam & 511] = 1;
		break;
	case WM_KEYUP:
		window->keys[wParam & 511] = 0;
		break;
	case WM_LBUTTONDOWN:
		window->mouse_info.mouse_position = GetMousePosition();
		window->mouseButtons[0] = 1;
		break;
	case WM_LBUTTONUP:
		window->mouseButtons[0] = 0;
		break;
	case WM_RBUTTONDOWN:
		window->mouse_info.mouse_position = GetMousePosition();
		window->mouseButtons[1] = 1;
		break;
	case WM_RBUTTONUP:
		window->mouseButtons[1] = 0;
		break;
	case WM_MOUSEWHEEL:
		window->mouseButtons[2] = 1;
		window->mouse_info.mouse_wheel_delta = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		break;

	default: return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
	UINT        style;
	WNDPROC     lpfnWndProc;
	int         cbClsExtra;
	int         cbWndExtra;
	HINSTANCE   hInstance;
	HICON       hIcon;
	HCURSOR     hCursor;
	HBRUSH      hbrBackground;
	LPCSTR      lpszMenuName;
	LPCSTR      lpszClassName;
*/
static void register_window_class(const char* title)
{
	ATOM atom;
	//初始化结构体
	WNDCLASS wc;
	wc.style = CS_BYTEALIGNCLIENT;							//窗口风格
	wc.lpfnWndProc = (WNDPROC)msg_callback;					//回调函数
	wc.cbClsExtra = 0;										//紧跟在窗口类尾部的一块额外空间，不用则设为0
	wc.cbWndExtra = 0;										//紧跟在窗口实例尾部的一块额外空间，不用则设为0
	wc.hInstance = GetModuleHandle(NULL);					//当前实例句柄
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);				//任务栏图标
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);				//光标样式
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	//背景样式
	wc.lpszMenuName = NULL;									//菜单
	wc.lpszClassName = title;					//该窗口类的名字

	atom = RegisterClass(&wc); //注册窗口类
	assert(atom != 0);
}

/*
		DWORD      biSize;
		LONG       biWidth;
		LONG       biHeight;
		WORD       biPlanes;
		WORD       biBitCount;
		DWORD      biCompression;
		DWORD      biSizeImage;
		LONG       biXPelsPerMeter;
		LONG       biYPelsPerMeter;
		DWORD      biClrUsed;
		DWORD      biClrImportant;
*/
static void init_bm_header(BITMAPINFOHEADER& bitmap, int width, int height)
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

int window_init(int width, int height, const char* title)
{
	window = (window_t*)malloc(sizeof(window_t));
	memset(window, 0, sizeof(window_t));
	window->is_close = false;

	RECT rect = { 0, 0, width, height };	//一个矩形范围 左上右下
	int wx, wy, sx, sy;
	LPVOID ptr;								//就是void *
	HDC hDC;								//设备环境，h代表句柄，handle
	BITMAPINFOHEADER bi;

	//注册窗口类
	register_window_class(title);

	//创建窗口
	window->h_window = CreateWindow(title, title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
	assert(window->h_window != NULL);

	//初始化位图头格式
	init_bm_header(bi, width, height);

	//获得兼容性DC
	hDC = GetDC(window->h_window);
	window->mem_dc = CreateCompatibleDC(hDC);
	ReleaseDC(window->h_window, hDC);

	//创建位图
	window->bm_dib = CreateDIBSection(window->mem_dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &ptr, 0, 0); //创建设备无关句柄
	assert(window->bm_dib != NULL);

	window->bm_old = (HBITMAP)SelectObject(window->mem_dc, window->bm_dib);//把新创建的位图句柄写入mem_dc
	window->window_fb = (unsigned char*)ptr;

	window->width = width;
	window->height = height;


	AdjustWindowRect(&rect, GetWindowLong(window->h_window, GWL_STYLE), 0);//调整窗口大小
	wx = rect.right - rect.left;
	wy = rect.bottom - rect.top;
	sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2; // GetSystemMetrics(SM_CXSCREEN)获取你屏幕的分片率
	sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2; // 计算出中心位置
	if (sy < 0) sy = 0;

	SetWindowPos(window->h_window, NULL, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(window->h_window);
	ShowWindow(window->h_window, SW_NORMAL);

	//消息分派
	msg_dispatch();

	//初始化keys, window_fb全为0
	memset(window->window_fb, 0, width * height * 4);
	memset(window->keys, 0, sizeof(char) * 512);
	return 0;
}

int window_destroy()
{
	if (window->mem_dc)
	{
		if (window->bm_old)
		{
			SelectObject(window->mem_dc, window->bm_old); // 写入原来的bitmap，才能释放DC！
			window->bm_old = NULL;
		}
		DeleteDC(window->mem_dc);
		window->mem_dc = NULL;
	}
	if (window->bm_dib)
	{
		DeleteObject(window->bm_dib);
		window->bm_dib = NULL;
	}
	if (window->h_window)
	{
		CloseWindow(window->h_window);
		window->h_window = NULL;
	}

	free(window);
	return 0;
}

void msg_dispatch()
{
	MSG msg;
	while (1)
	{
		// Peek不阻塞，Get会阻塞，PM_NOREMOVE表示如果有消息不处理（留给接下来的Get处理）
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) break; //没消息就溜，确定有消息再用Get
		if (!GetMessage(&msg, NULL, 0, 0)) break;

		TranslateMessage(&msg);	 //转换消息 虚拟按键->字符
		DispatchMessage(&msg); //传送消息给回调
	}
}

static void window_display(std::string logMessage)
{
	//显示Log信息
	if (!logMessage.empty()) {
		LOGFONT logfont;								//改变输出字体
		ZeroMemory(&logfont, sizeof(LOGFONT));
		logfont.lfCharSet = ANSI_CHARSET;
		logfont.lfHeight = 20;							//设置字体的大小
		HFONT hFont = CreateFontIndirect(&logfont);

		//目标矩形的左上角(x,y), 宽度，高度，上下文指针
		SelectObject(window->mem_dc, hFont);
		SetTextColor(window->mem_dc, RGB(190, 190, 190));
		SetBkColor(window->mem_dc, RGB(80, 80, 80));

		TextOut(window->mem_dc, 20, 20,
			logMessage.c_str(),
			strlen(logMessage.c_str()));
	}

	//绘制framebuffer
	HDC hDC = GetDC(window->h_window);

	// 把兼容性DC的数据传到真正的DC上
	BitBlt(hDC, 0, 0, window->width, window->height, window->mem_dc, 0, 0, SRCCOPY);
	ReleaseDC(window->h_window, hDC);
}

void window_draw(unsigned char* framebuffer, std::string logMessage)
{
	memcpy(window->window_fb, framebuffer, window->width * window->height * 4);

	window_display(logMessage);
}

Vec2f GetMousePosition()
{
	POINT mousePoint;
	GetCursorPos(&mousePoint);

	// 将鼠标位置从屏幕空间转到窗口空间
	ScreenToClient(window->h_window, &mousePoint);
	return Vec2f((float)mousePoint.x, (float)mousePoint.y);
}

/* misc platform functions */
static double get_native_time(void) {
	static double period = -1;
	LARGE_INTEGER counter;
	if (period < 0) {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		period = 1 / (double)frequency.QuadPart;
	}
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * period;
}

float platform_get_time(void) {
	static double initial = -1;
	if (initial < 0) {
		initial = get_native_time();
	}
	return (float)(get_native_time() - initial);
}
