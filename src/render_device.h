#pragma once

#include <stdint.h>

#include "screen_manager.h"

class RenderDevice {
   public:
    RenderDevice(ScreenManager* screen);
    ~RenderDevice();

    void Clear(uint32_t color);
    void SetPixel(int x, int y, uint32_t color);
    // add z-buffer Brasterization...

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

   private:
    ScreenManager* screen;
    unsigned char* frame_buffer;
    int pitch;
    int width;
    int height;
};
