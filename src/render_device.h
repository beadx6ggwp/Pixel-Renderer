#ifndef RENDER_DEVICE_H
#define RENDER_DEVICE_H

#include <stdint.h>

#include "screen_manager.h"

struct Vertex {
    float x, y, z;
    uint32_t color;
};
struct Vec3f {
    float x, y, z;
    Vec3f(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

class RenderDevice {
   public:
    RenderDevice(ScreenManager* screen);
    ~RenderDevice();

    void Clear(uint32_t color);
    void SetPixel(int x, int y, uint32_t color);
    void DrawLine(int x1, int y1, int x2, int y2, uint32_t color);
    void DrawTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3,
                      uint32_t color);

    // add z-buffer Brasterization...

   private:
    ScreenManager* screen;
    unsigned char* frame_buffer;
    int pitch;
    int width;
    int height;
};

#endif