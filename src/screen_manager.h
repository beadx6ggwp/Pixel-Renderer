#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <bitset>

class ScreenManager {
public:
    ScreenManager();
    ~ScreenManager();

    bool Init(int width, int height, const wchar_t* title);
    void Close();

    void DispatchEvents(); // Dispatch window events
    void UpdateScreen();  

    // Input handling
    bool IsKeyDown(int key) const; // e.g. VK_ESCAPE
    int GetMouseX() const;
    int GetMouseY() const;
    bool IsMouseButtonDown(int button) const; // 0: left, 1: right

    // Framebuffer access (for RenderDevice)
    unsigned char* GetFrameBuffer() const;
    int GetPitch() const;
    int GetWidth() const;
    int GetHeight() const;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND window_handle = NULL;
    HDC window_dc = NULL;
    HDC mem_dc = NULL;
    HBITMAP dib_bitmap = NULL;
    HBITMAP old_bitmap = NULL;
    unsigned char* frame_buffer = NULL;
    int width = 0;
    int height = 0;
    int pitch = 0;

    std::bitset<256> keys; // Keyboard state
    int mouse_x = 0;
    int mouse_y = 0;
    std::bitset<3> mouse_buttons; // 0: left, 1: right, 2: middle
};

#endif