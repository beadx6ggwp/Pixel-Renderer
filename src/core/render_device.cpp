#include "render_device.h"

#include <string.h>
#include <algorithm>

RenderDevice::RenderDevice(const FramebufferConfig& config) {
    frame_buffer = config.buffer;
    pitch = config.pitch;
    width = config.width;
    height = config.height;
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
