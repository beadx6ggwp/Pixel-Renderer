#include "render_device.h"

#include <string.h>

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
