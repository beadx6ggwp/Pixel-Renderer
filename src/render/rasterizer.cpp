#include "rasterizer.h"

#include <algorithm>

#include "../render_device.h"

void Rasterizer::DrawLine(int x1, int y1, int x2, int y2, uint32_t color) {
    // Bresenham simple
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        device->SetPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void Rasterizer::DrawTriangle(const Vertex& v1, const Vertex& v2,
                              const Vertex& v3, uint32_t color) {
    // need finish rasterization
    // 1. find bounding box of the triangle
    int min_y = (int)(std::min)({(int)v1.y, (int)v2.y, (int)v3.y});
    int max_y = (int)(std::max)({(int)v1.y, (int)v2.y, (int)v3.y});
    int min_x = (int)(std::min)({(int)v1.x, (int)v2.x, (int)v3.x});
    int max_x = (int)(std::max)({(int)v1.x, (int)v2.x, (int)v3.x});

    // clip to screen
    min_x = std::max<int>(min_x, 0);
    min_y = std::max<int>(min_y, 0);
    max_x = std::min<int>(max_x, device->GetWidth() - 1);
    max_y = std::min<int>(max_y, device->GetHeight() - 1);

    // scan through the bounding box and check if each pixel is inside the
    // triangle using barycentric coordinates
    //
    // TODO: Refactor to Edge Function for production use
    // ===================================================
    // Current implementation: Barycentric coordinates (educational version)
    // - Easier to understand and implement
    // - Directly provides interpolation coefficients (w1, w2, w3)
    // - Required for color/UV/depth interpolation
    // - Involves division (potential floating-point precision issue)
    //
    // Future implementation: Edge Function (production standard)
    // - Tests which side of each edge the point lies on
    // - Pure linear operations: add/subtract/multiply only (no division)
    // - Avoids floating-point precision issues
    // - Ideal for rasterization: hardware can parallelize efficiently
    // - Industry standard in modern graphics engines
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            // check if (x,y) is inside the triangle using barycentric
            // coordinates
            float w1 =
                ((v2.y - v3.y) * (x - v3.x) + (v3.x - v2.x) * (y - v3.y)) /
                ((v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y));
            float w2 =
                ((v3.y - v1.y) * (x - v3.x) + (v1.x - v3.x) * (y - v3.y)) /
                ((v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y));
            float w3 = 1.0f - w1 - w2;
            bool is_inside = (w1 >= 0 && w2 >= 0 && w3 >= 0);
            if (is_inside) {
                device->SetPixel(x, y, color);
            }
        }
    }
}