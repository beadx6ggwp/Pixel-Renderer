#include "application.h"

#include <stdio.h>

Application::Application(int w, int h, const wchar_t* t) {
    // Initialize ScreenManager first, then create RenderDevice and Rasterizer
    // screen is the lowest level, it will manage window and framebuffer
    // RenderDevice depends on ScreenManager for framebuffer access
    // Rasterizer depends on RenderDevice for drawing operations
    running = screen.Init(w, h, t);
    if (running) {
        
        // 1. 從 screen 獲取數據並打包成 struct
        FramebufferConfig config;
        config.buffer = screen.GetFrameBuffer();
        config.width  = screen.GetWidth();
        config.height = screen.GetHeight();
        config.pitch  = screen.GetPitch();

        // 2. 注入給 RenderDevice
        device = new RenderDevice(config);
        
        // 3. 建立 Rasterizer
        rasterizer = new Rasterizer(device);

        printf("Application initialized: Data-driven HAL ready.\n");
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