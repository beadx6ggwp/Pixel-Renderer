#include "screen_manager.h"
#include <stdio.h>

ScreenManager::ScreenManager() {}
ScreenManager::~ScreenManager() { Close(); }

bool ScreenManager::Init(int w, int h, const wchar_t* title) {
    Close(); // clear resources

    const wchar_t* class_name = L"RenderWindowClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = class_name;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (!RegisterClass(&wc)) {
        printf("RegisterClass failed\n");
        return false;
    }

    RECT rect = { 0, 0, w, h };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    int wx = rect.right - rect.left;
    int wy = rect.bottom - rect.top;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - wx) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wy) / 2;

    window_handle = CreateWindow(class_name, title, WS_OVERLAPPEDWINDOW, sx, sy, wx, wy, NULL, NULL, wc.hInstance, NULL);
    if (!window_handle) {
        printf("CreateWindow failed\n");
        return false;
    }

    // Set this pointer for use in WindowProc
    SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)this);

    ShowWindow(window_handle, SW_SHOW);

    window_dc = GetDC(window_handle);
    mem_dc = CreateCompatibleDC(window_dc);

    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h; // Top-down DIB
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* ptr = NULL;
    dib_bitmap = CreateDIBSection(mem_dc, &bi, DIB_RGB_COLORS, &ptr, NULL, 0);
    if (!dib_bitmap) {
        printf("CreateDIBSection failed\n");
        return false;
    }

    old_bitmap = (HBITMAP)SelectObject(mem_dc, dib_bitmap);
    frame_buffer = (unsigned char*)ptr;
    width = w;
    height = h;
    pitch = w * 4;

    memset(frame_buffer, 0, w * h * 4);
    keys.reset();
    printf("ScreenManager initialized: %dx%d\n", w, h);
    return true;
}

void ScreenManager::Close() {
    if (mem_dc) {
        if (old_bitmap) SelectObject(mem_dc, old_bitmap);
        DeleteDC(mem_dc);
    }
    if (dib_bitmap) DeleteObject(dib_bitmap);
    if (window_dc) ReleaseDC(window_handle, window_dc);
    if (window_handle) DestroyWindow(window_handle);

    window_handle = NULL;
    mem_dc = NULL;
    dib_bitmap = NULL;
    old_bitmap = NULL;
    window_dc = NULL;
    frame_buffer = NULL;
    printf("ScreenManager closed\n");
}

void ScreenManager::DispatchEvents() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void ScreenManager::UpdateScreen() {
    BitBlt(window_dc, 0, 0, width, height, mem_dc, 0, 0, SRCCOPY);
}

bool ScreenManager::IsKeyDown(int key) const {
    return keys[key];
}

int ScreenManager::GetMouseX() const { return mouse_x; }
int ScreenManager::GetMouseY() const { return mouse_y; }
bool ScreenManager::IsMouseButtonDown(int button) const {
    return mouse_buttons[button];
}

unsigned char* ScreenManager::GetFrameBuffer() const { return frame_buffer; }
int ScreenManager::GetPitch() const { return pitch; }
int ScreenManager::GetWidth() const { return width; }
int ScreenManager::GetHeight() const { return height; }

LRESULT CALLBACK ScreenManager::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ScreenManager* self = (ScreenManager*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (self == nullptr) return DefWindowProc(hwnd, msg, wParam, lParam); // Check for nullptr

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam < 256) self->keys[wParam] = true;
        return 0;
    case WM_KEYUP:
        if (wParam < 256) self->keys[wParam] = false;
        return 0;
    case WM_MOUSEMOVE:
        self->mouse_x = LOWORD(lParam);
        self->mouse_y = HIWORD(lParam);
        return 0;
    case WM_LBUTTONDOWN:
        self->mouse_buttons[0] = true;
        return 0;
    case WM_LBUTTONUP:
        self->mouse_buttons[0] = false;
        return 0;
    case WM_RBUTTONDOWN:
        self->mouse_buttons[1] = true;
        return 0;
    case WM_RBUTTONUP:
        self->mouse_buttons[1] = false;
        return 0;
    case WM_MBUTTONDOWN:
        self->mouse_buttons[2] = true;
        return 0;
    case WM_MBUTTONUP:
        self->mouse_buttons[2] = false;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}