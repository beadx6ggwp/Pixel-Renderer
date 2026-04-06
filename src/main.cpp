#include <stdio.h>

#include "application.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

class MyApp : public Application {
   public:
    MyApp() : Application(WINDOW_WIDTH, WINDOW_HEIGHT, L"PixelRenderer") {}

    void OnInit() override { printf("App initialized\n"); }

    void OnUpdate(float delta_time) override {
        if (screen.IsKeyDown('A')) {
            printf("A key pressed, total_time: %f\n", total_time);
        }
    }

    void OnRender() override {

        device->Clear(0x000000);
        RenderFrame(frame_count);
        rasterizer->DrawLine(100, 100, 700, 500, 0x00FF00);
    }

    void RenderFrame(int frame) {
        for (int y = 0; y < WINDOW_HEIGHT; y++) {
            for (int x = 0; x < WINDOW_WIDTH; x++) {
                uint32_t color =
                    (x + frame) % 256 | ((y + frame) % 256 << 8) | (128 << 16);
                device->SetPixel(x, y, color);
            }
        }
        rasterizer->DrawTriangle({400, 100}, {500, 300}, {300, 300}, 0xFF0000);
    }

    void OnClose() override { printf("App closed\n"); }
};

int main() {
    MyApp app;
    app.Run();
    return 0;
}