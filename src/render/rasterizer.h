#pragma once

#include "types.h"  // 拿到 Vertex, Vec3f 定義

// Forward Declaration: 告訴編譯器 RenderDevice 是一個類別，現在不需要知道細節
class RenderDevice;
class Rasterizer {
   public:
    Rasterizer(RenderDevice* _device) : device(_device) {}

    void DrawLine(int x1, int y1, int x2, int y2, uint32_t color);
    void DrawTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3,
                      uint32_t color);

   private:
    RenderDevice* device;  // 這裡存pointer，編譯器只需要知道 RenderDevice 存在即可
};