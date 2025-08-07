#include "application.h"
#include <stdio.h>

Application::Application(int w, int h, const wchar_t* t) {
    running = screen.Init(w, h, t);
    if (running) device = new RenderDevice(&screen);
}

Application::~Application() {
    delete device;
    screen.Close();
}

void Application::Run() {
    if (!running) return;

    OnInit();

    LARGE_INTEGER frequency, last_time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&last_time);
    double target_frame_time = 1000.0 / 60.0; // 60 FPS

    while (running && !screen.IsKeyDown(VK_ESCAPE)) {
        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);
        double delta_time = (current_time.QuadPart - last_time.QuadPart) * 1000.0 / frequency.QuadPart;

        if (delta_time >= target_frame_time) {
            screen.DispatchEvents();
            OnUpdate(static_cast<float>(delta_time / 1000.0)); // delta in seconds
            OnRender();
            screen.UpdateScreen();
            last_time = current_time;
            printf("Frame time: %.2f ms\n", delta_time); // Debug
            wchar_t strBuffer[100];
            /*
            swprintf(strBuffer, 100,
                L"Pixel ver0.1, %dx%d, FPS:%4d, dt:%2dms",
                screen.GetWidth(), screen.GetHeight(), 1000.0/ delta_time, delta_time);*/
        }
    }

    OnClose();
}