#pragma once

#include <stdint.h>


// 定義純粹的數據結構，不需要包含 screen_manager.h
struct FramebufferConfig {
    unsigned char* buffer;
    int width;
    int height;
    int pitch;
};

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
