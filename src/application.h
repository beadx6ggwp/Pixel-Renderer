#pragma once

#include "render/rasterizer.h"
#include "render_device.h"
#include "screen_manager.h"

class Application {
   public:
    Application(int width, int height, const wchar_t* title);
    virtual ~Application();

    void Run();  // game loop

   protected:
    virtual void OnInit() {}
    virtual void OnUpdate(float delta_time) {}
    virtual void OnRender() {}
    virtual void OnClose() {}

    ScreenManager screen;
    RenderDevice* device = nullptr;
    Rasterizer* rasterizer = nullptr;

    bool running = false;
    double total_time = 0.0;   // Total elapsed time in seconds
    uint64_t frame_count = 0;  // Total frames rendered
};