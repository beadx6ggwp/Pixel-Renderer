#include "render_device.h"
#include <string.h>
#include <algorithm>

RenderDevice::RenderDevice(ScreenManager* s) : screen(s) {
    frame_buffer = screen->GetFrameBuffer();
    pitch = screen->GetPitch();
    width = screen->GetWidth();
    height = screen->GetHeight();
}

RenderDevice::~RenderDevice() {}

void RenderDevice::Clear(uint32_t color) {
    for (int y = 0; y < height; y++) {
        uint32_t* row = (uint32_t*)(frame_buffer + y * pitch);
        std::fill(row, row + width, color);
    }
}

void RenderDevice::SetPixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        *(uint32_t*)(frame_buffer + y * pitch + x * 4) = color;
    }
}

void RenderDevice::DrawLine(int x1, int y1, int x2, int y2, uint32_t color) {
    // Bresenham simple
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        SetPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void RenderDevice::DrawTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3, uint32_t color) {
    // need finish rasterization
    int min_y = (int)(std::min)({ (int)v1.y, (int)v2.y, (int)v3.y });
    int max_y = (int)(std::max)({ (int)v1.y, (int)v2.y, (int)v3.y });
    for (int y = min_y; y <= max_y; y++) {
        // test
        DrawLine(100, y, 200, y, color);
    }
}