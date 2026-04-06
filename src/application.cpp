#include "application.h"

#include <stdio.h>

Application::Application(int w, int h, const wchar_t* t) {
    // Initialize ScreenManager first, then create RenderDevice and Rasterizer
    // screen is the lowest level, it will manage window and framebuffer
    // RenderDevice depends on ScreenManager for framebuffer access
    // Rasterizer depends on RenderDevice for drawing operations
    running = screen.Init(w, h, t);
    if (running) {
        device = new RenderDevice(&screen);
        rasterizer = new Rasterizer(device);

        if (!device || !rasterizer) {
            running = false;
            printf("Failed to initialize RenderDevice or Rasterizer\n");
        }

        printf("Application initialized\n");
    }
}

Application::~Application() {
    delete rasterizer;
    delete device;
    screen.Close();
}

void Application::Run() {
    if (!running) return;

    OnInit();

    LARGE_INTEGER frequency, last_time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&last_time);
    float target_frame_time = 1.0 / 60.0;  // 60 FPS

    while (running && !screen.IsKeyDown(VK_ESCAPE)) {
        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);
        // delta_time in seconds
        float delta_time = (current_time.QuadPart - last_time.QuadPart) /
                           (float)frequency.QuadPart;

        if (delta_time >= target_frame_time) {
            screen.DispatchEvents();
            OnUpdate(delta_time);  // delta in seconds
            OnRender();
            screen.UpdateScreen();
            last_time = current_time;
            total_time += delta_time;
            frame_count++;

            wchar_t strBuffer[128];
            swprintf(strBuffer, 128,
                     L"Pixel Renderer, %dx%d | FPS:%.1f | dt:%fs | t:%.2fs | "
                     L"frame:%llu",
                     screen.GetWidth(), screen.GetHeight(), 1.0 / delta_time,
                     delta_time, total_time, frame_count);
            screen.SetWindowTitle(strBuffer);
        }
    }

    OnClose();
}