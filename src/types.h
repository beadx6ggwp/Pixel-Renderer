#pragma once
#include <stdint.h>

// 定義純粹的數據結構，不需要包含 screen_manager.h
struct FramebufferConfig {
    unsigned char* buffer;
    int width;
    int height;
    int pitch;
};

struct Vertex {
    float x, y, z;
    uint32_t color;
};

struct Vec3f {
    float x, y, z;
    Vec3f(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};