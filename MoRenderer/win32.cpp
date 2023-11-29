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
	//��ʼ���ṹ��
	WNDCLASS wc;
	wc.style = CS_BYTEALIGNCLIENT;							//���ڷ��
	wc.lpfnWndProc = (WNDPROC)msg_callback;					//�ص�����
	wc.cbClsExtra = 0;										//�����ڴ�����β����һ�����ռ䣬��������Ϊ0
	wc.cbWndExtra = 0;										//�����ڴ���ʵ��β����һ�����ռ䣬��������Ϊ0
	wc.hInstance = GetModuleHandle(NULL);					//��ǰʵ�����
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);				//������ͼ��
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);				//�����ʽ
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	//������ʽ
	wc.lpszMenuName = NULL;									//�˵�
	wc.lpszClassName = title;					//�ô����������

	atom = RegisterClass(&wc); //ע�ᴰ����
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
	bitmap.biSize = sizeof(BITMAPINFOHEADER);		//���ṹ��ռ�õ��ֽ���
	bitmap.biWidth = width;							//bitmap���
	bitmap.biHeight = height;						//bitmap�߶ȣ�ԭ��λ�����½�
	bitmap.biPlanes = 1;							//Ŀ���豸����
	bitmap.biBitCount = 32;							//bitmap��һ����ɫ��ռ�ݵ�bit��
	bitmap.biCompression = BI_RGB;					//�Ƿ�ѹ����BI_RGBΪ��ѹ��
	bitmap.biSizeImage = width * height * 4;		//����bitmap��ռ�ݵ��ֽ���
}

int window_init(int width, int height, const char* title)
{
	window = (window_t*)malloc(sizeof(window_t));
	memset(window, 0, sizeof(window_t));
	window->is_close = false;

	RECT rect = { 0, 0, width, height };	//һ�����η�Χ ��������
	int wx, wy, sx, sy;
	LPVOID ptr;								//����void *
	HDC hDC;								//�豸������h��������handle
	BITMAPINFOHEADER bi;

	//ע�ᴰ����
	register_window_class(title);

	//��������
	window->h_window = CreateWindow(title, title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
	assert(window->h_window != NULL);

	//��ʼ��λͼͷ��ʽ
	init_bm_header(bi, width, height);

	//��ü�����DC
	hDC = GetDC(window->h_window);
	window->mem_dc = CreateCompatibleDC(hDC);
	ReleaseDC(window->h_window, hDC);

	//����λͼ
	window->bm_dib = CreateDIBSection(window->mem_dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &ptr, 0, 0); //�����豸�޹ؾ��
	assert(window->bm_dib != NULL);

	window->bm_old = (HBITMAP)SelectObject(window->mem_dc, window->bm_dib);//���´�����λͼ���д��mem_dc
	window->window_fb = (unsigned char*)ptr;

	window->width = width;
	window->height = height;


	AdjustWindowRect(&rect, GetWindowLong(window->h_window, GWL_STYLE), 0);//�������ڴ�С
	wx = rect.right - rect.left;
	wy = rect.bottom - rect.top;
	sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2; // GetSystemMetrics(SM_CXSCREEN)��ȡ����Ļ�ķ�Ƭ��
	sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2; // ���������λ��
	if (sy < 0) sy = 0;

	SetWindowPos(window->h_window, NULL, sx, sy, wx, wy, (SWP_NOCOPYBITS | SWP_NOZORDER | SWP_SHOWWINDOW));
	SetForegroundWindow(window->h_window);
	ShowWindow(window->h_window, SW_NORMAL);

	//��Ϣ����
	msg_dispatch();

	//��ʼ��keys, window_fbȫΪ0
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
			SelectObject(window->mem_dc, window->bm_old); // д��ԭ����bitmap�������ͷ�DC��
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
		// Peek��������Get��������PM_NOREMOVE��ʾ�������Ϣ������������������Get����
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) break; //û��Ϣ���ȷ������Ϣ����Get
		if (!GetMessage(&msg, NULL, 0, 0)) break;

		TranslateMessage(&msg);	 //ת����Ϣ ���ⰴ��->�ַ�
		DispatchMessage(&msg); //������Ϣ���ص�
	}
}

static void window_display(std::string logMessage)
{
	//��ʾLog��Ϣ
	if (!logMessage.empty()) {
		LOGFONT logfont;								//�ı��������
		ZeroMemory(&logfont, sizeof(LOGFONT));
		logfont.lfCharSet = ANSI_CHARSET;
		logfont.lfHeight = 20;							//��������Ĵ�С
		HFONT hFont = CreateFontIndirect(&logfont);

		//Ŀ����ε����Ͻ�(x,y), ��ȣ��߶ȣ�������ָ��
		SelectObject(window->mem_dc, hFont);
		SetTextColor(window->mem_dc, RGB(190, 190, 190));
		SetBkColor(window->mem_dc, RGB(80, 80, 80));

		TextOut(window->mem_dc, 20, 20,
			logMessage.c_str(),
			strlen(logMessage.c_str()));
	}

	//����framebuffer
	HDC hDC = GetDC(window->h_window);

	// �Ѽ�����DC�����ݴ���������DC��
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

	// �����λ�ô���Ļ�ռ�ת�����ڿռ�
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
