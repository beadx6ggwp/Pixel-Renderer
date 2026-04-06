#pragma once

#include <stdint.h>
#include "types.h" // 拿到 FramebufferConfig

class RenderDevice {
   public:
    RenderDevice(const FramebufferConfig& config);
    ~RenderDevice();

    void Clear(uint32_t color);
    void SetPixel(int x, int y, uint32_t color);
    // add z-buffer Brasterization...

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

   private:
    unsigned char* frame_buffer;
    int pitch;
    int width;
    int height;
};
