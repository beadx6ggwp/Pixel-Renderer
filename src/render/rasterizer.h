#pragma once

#include <algorithm>

#include "../render_device.h"

struct Vertex {
    float x, y, z;
    uint32_t color;
};
struct Vec3f {
    float x, y, z;
    Vec3f(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

class Rasterizer {
   public:
    Rasterizer(RenderDevice* _device) : device(_device) {}

    void DrawLine(int x1, int y1, int x2, int y2, uint32_t color);
    void DrawTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3,
                      uint32_t color);

   private:
    RenderDevice* device;  // 依賴 Core 層的接口
};