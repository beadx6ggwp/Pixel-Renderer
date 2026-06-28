# Pixel-Renderer

![](docs/img1.jpg)

A C++ software renderer built from scratch without OpenGL or DirectX.

The project starts from a Win32 framebuffer prototype and grows upward from `SetPixel()` to line drawing, triangle rasterization, depth buffer, transforms, shader-style stages, renderer structure, and debug tools.

The goal is to understand how pixels are produced by a renderer, then connect those ideas to the way Unity, Unreal, and custom engines organize cameras, materials, commands, backends, and debugging tools.

## Overview

Pixel-Renderer is currently a small Windows software rendering prototype. It creates a Win32 window, owns a DIBSection framebuffer through `ScreenManager`, and writes pixels from the CPU through `RenderDevice` and `Rasterizer`.

Current source focuses on the first visible layer of the renderer:

```text
Application
  |
  +-- ScreenManager
  |     Win32 window
  |     DIBSection framebuffer
  |     input
  |     BitBlt present
  |
  +-- RenderDevice
  |     Clear()
  |     SetPixel()
  |
  +-- Rasterizer
        DrawLine()
        DrawTriangle()
```

The next step is to make triangle rasterization more reliable before adding the larger 3D pipeline.

## Features

Current:

- Win32 window and DIBSection framebuffer.
- CPU-side `Clear()` and `SetPixel()`.
- Basic frame loop and input state.
- Bresenham-style line drawing.
- Basic bounding-box triangle fill with barycentric inside test.
- No graphics API dependency for rendering.

Planned:

- Edge-function triangle rasterization.
- Depth buffer and depth test.
- Color and depth interpolation.
- Small tests and debug views.
- SDL / Win32 display backend switching.
- Viewport / MVP transform.
- Shader-style stage boundary.
- Material and renderer structure.
- Debug UI and engine concept mapping.

## Getting Started

### Prerequisites

- Windows.
- `g++.exe`.
- GNU Make.
- Win32 libraries: `gdi32`, `user32`.

### Building

```bash
make debug
make release
make clean
```

Debug output:

```text
build/debug/PixelRenderer_debug.exe
```

Release output:

```text
build/release/PixelRenderer_release.exe
```

## Usage

Inherit from `Application` and override the lifecycle hooks:

```cpp
#include "core/application.h"

class MyApp : public Application {
public:
    MyApp() : Application(800, 600, L"PixelRenderer") {}

    void OnRender() override {
        device->Clear(0x000000);
        rasterizer->DrawLine(100, 100, 700, 500, 0x00FF00);
        rasterizer->DrawTriangle({400, 100}, {500, 300}, {300, 300}, 0xFF0000);
    }
};

int main() {
    MyApp app;
    app.Run();
    return 0;
}
```

## Directory Tree

```text
Pixel-Renderer/
  makefile
  README.md

  src/
    main.cpp
    types.h

    core/
      application.h/cpp
      screen_manager.h/cpp
      render_device.h/cpp

    render/
      rasterizer.h/cpp

  docs/
    README.md
    PROJECT_MAP.md
    ARCHITECTURE.md
    DEVELOPMENT.md
    foundations/
    architecture/
    verification/
    mapping/
    roadmap/
    adr/
    notes/
    tutorial-soft-renderer/
    tutorial-cpp/
```

## Roadmap

Near term:

- Replace the current triangle fill with a clearer screen-space triangle path.
- Add edge-function coverage, pixel-center sampling, and a stable edge rule.
- Add color interpolation, depth interpolation, depth buffer, and depth test.
- Add small tests and simple debug views for barycentric, depth, and wireframe.

Later:

- Add viewport transform, vector / matrix math, and MVP.
- Define shader-style stage data such as vertex output and fragment input.
- Add material and renderer structure.
- Make display backend switching more flexible, starting with Win32 and SDL.
- Separate framebuffer ownership from display presentation.
- Add CommandQueue, debug UI, and engine mapping notes.

Possible future shape:

```text
Application
  -> Renderer
  -> SoftwareBackend
  -> Rasterizer
  -> RenderDevice
  -> Framebuffer
  -> DisplayBackend
       -> Win32
       -> SDL
       -> Headless tests
```
