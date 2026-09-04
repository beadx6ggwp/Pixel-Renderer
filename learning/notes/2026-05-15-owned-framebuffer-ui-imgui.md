# Owned Framebuffer、UI 與 ImGui Software Backend

這份筆記整理一個核心設計原則：

```text
如果目標是完全掌握 software renderer 的細節，
主 framebuffer 必須由 renderer/core 自己擁有，
而不是由 SDL_Texture、Win32 DIBSection 或 macOS surface 擁有。
```

平台層可以幫忙開 window、收 input、present final pixels，但真正的畫面內容應該先由自己的程式寫進自己的 CPU framebuffer。

## 1. 為什麼要自己擁有 framebuffer

如果 framebuffer 由 renderer/core 擁有，才能完整掌握：

```text
pixel format
pitch / stride
row order
color packing
clipping
alpha blending
depth buffer
dirty rect
font atlas sampling
UI composition
screenshot / test output
```

這些不是 peripheral details。它們正是 software renderer、UI renderer、ImGui backend 的共同底層。

如果直接把主 framebuffer 綁到平台資源：

```text
Win32 DIBSection
SDL_Texture locked pixels
macOS surface / layer memory
```

renderer core 就會受平台生命週期限制：

```text
pointer 何時有效？
pitch 由誰決定？
old contents 是否可靠？
pixel format 是否固定？
能不能做 offscreen test？
能不能截圖？
能不能同時做 color buffer / depth buffer / UI overlay？
```

所以最乾淨的第一原則是：

```text
Renderer owns the framebuffer.
Display backend only presents it.
```

## 2. 正確的 ownership 分層

理想資料流：

```text
+---------------------------+
| Application / Scene / UI  |
| decide what to draw       |
+---------------------------+
              |
              v
+---------------------------+
| Software Renderer         |
| Rasterizer / UI renderer  |
| writes pixels             |
+---------------------------+
              |
              v
+---------------------------+
| Owned CPU Framebuffer     |
| color / depth / metadata  |
+---------------------------+
              |
              | Present(const Framebuffer&)
              v
+---------------------------+
| Display Backend           |
| Win32 / SDL / macOS       |
| upload/blit/present only  |
+---------------------------+
              |
              v
+---------------------------+
| OS compositor / monitor   |
+---------------------------+
```

這裡的 framebuffer 是 render target，不是 display backend 本身。

```text
Rendering Backend output
Display Backend input
```

## 3. 目前專案的問題

目前 Win32 path 比較接近：

```text
ScreenManager
    -> owns Win32 DIBSection framebuffer
    -> exposes pointer via GetFrameBuffer()
    -> RenderDevice writes that platform-owned memory
    -> ScreenManager::UpdateScreen() calls BitBlt()
```

這比直接在 `Rasterizer` 裡呼叫 Win32 API 好很多，因為 `RenderDevice::SetPixel()` 已經只看 `FramebufferConfig`。但它還不是最終乾淨型態，因為主 framebuffer 的 ownership 仍在 `ScreenManager`。

目前模型：

```text
ScreenManager owns framebuffer
RenderDevice borrows platform memory
Display and framebuffer ownership are coupled
```

目標模型：

```text
Renderer/core owns framebuffer
RenderDevice writes renderer-owned memory
DisplayBackend only reads/presents final pixels
```

也就是不要讓這件事成為主路徑：

```text
DisplayBackend::GetFrameBuffer()
    -> RenderDevice writes platform-owned memory
```

而是改成：

```text
RenderDevice owns or binds renderer-owned Framebuffer
DisplayBackend::Present(const Framebuffer& framebuffer)
```

## 4. Framebuffer 應該長什麼樣

最小資料結構：

```cpp
enum class PixelFormat {
    ARGB8888,
    RGBA8888,
    BGRA8888,
};

struct Framebuffer {
    uint32_t* pixels;
    int width;
    int height;
    int pitch;
    PixelFormat format;
};
```

更適合 renderer/core 擁有的版本：

```cpp
class Framebuffer {
public:
    int width = 0;
    int height = 0;
    int pitch = 0;
    PixelFormat format = PixelFormat::ARGB8888;
    std::vector<uint32_t> color;

    uint32_t* Row(int y) {
        return reinterpret_cast<uint32_t*>(
            reinterpret_cast<uint8_t*>(color.data()) + y * pitch
        );
    }

    const uint32_t* Row(int y) const {
        return reinterpret_cast<const uint32_t*>(
            reinterpret_cast<const uint8_t*>(color.data()) + y * pitch
        );
    }

    void Clear(uint32_t color);
    void SetPixel(int x, int y, uint32_t color);
};
```

如果之後加入 depth buffer：

```cpp
class RenderTarget {
public:
    Framebuffer color;
    std::vector<float> depth;
};
```

或更圖形管線化：

```text
RenderTarget
  color buffer
  depth buffer
  stencil buffer later
```

## 5. SDL/Win32/macOS 都只能是 present backend

SDL path 應該像這樣：

```text
Renderer-owned framebuffer
    |
    | SDLDisplayBackend::Present(framebuffer)
    v
SDL_UpdateTexture(texture, framebuffer.pixels, framebuffer.pitch)
    |
    v
SDL_RenderCopy / SDL_RenderTexture
    |
    v
SDL_RenderPresent
```

Win32 path 應該像這樣：

```text
Renderer-owned framebuffer
    |
    | Win32DisplayBackend::Present(framebuffer)
    v
Copy framebuffer into DIBSection or compatible window buffer
    |
    v
BitBlt
```

macOS native path 也應該像這樣：

```text
Renderer-owned framebuffer
    |
    | MacOSDisplayBackend::Present(framebuffer)
    v
Upload/copy into platform display surface or texture
    |
    v
present through Cocoa/Metal/CoreGraphics path
```

共同規則：

```text
DisplayBackend may own platform display resources.
DisplayBackend should not own the renderer's primary framebuffer.
```

## 6. 這對自製 UI 的意義

如果之後自己寫 immediate-mode UI，應該是：

```text
UI logic
    -> generates UI commands
    -> UI renderer executes commands
    -> writes into owned framebuffer
```

例子：

```text
Button("OK")
    -> command: DrawRect(x, y, w, h, color)
    -> command: DrawText(x, y, "OK")
```

UI software backend 做的事：

```text
DrawRect
    -> fill pixels in Framebuffer

DrawText
    -> sample font atlas
    -> alpha blend glyph pixels into Framebuffer

ClipRect / Scissor
    -> reject pixels outside UI parent/container

Layering
    -> draw scene first, then UI overlay
```

這些都需要直接掌握 framebuffer：

```text
address = base + y * pitch + x * bytes_per_pixel
```

以及掌握 pixel operation：

```text
dst = framebuffer[x, y]
src = UI pixel / glyph sample
out = alpha_blend(src, dst)
framebuffer[x, y] = out
```

如果主 framebuffer 是 SDL/Win32/macOS 的平台資源，UI renderer 會很快被平台細節污染。

## 7. 這對 ImGui 的意義

Dear ImGui 可以分成兩層理解：

```text
ImGui core
    -> handles UI state, layout, interaction
    -> produces ImDrawData

Renderer backend
    -> consumes ImDrawData
    -> turns vertices/indices/clip rects/textures into pixels
```

常見 ImGui backend 是 OpenGL/Vulkan/D3D/Metal。但也可以自己寫 software backend：

```text
ImGui::Render()
    -> ImDrawData
    -> for each command list
    -> for each ImDrawCmd
    -> apply clip rect
    -> rasterize indexed triangles
    -> sample font atlas
    -> alpha blend into owned framebuffer
```

ASCII：

```text
+----------------------+
| ImGui widgets        |
| Button / Slider ...  |
+----------------------+
          |
          v
+----------------------+
| ImDrawData           |
| cmd lists            |
| vertices / indices   |
| clip rects           |
| texture ids          |
+----------------------+
          |
          v
+----------------------+
| Your ImGui Software  |
| Renderer Backend     |
| - scissor            |
| - rasterize triangles|
| - sample font atlas  |
| - alpha blend        |
+----------------------+
          |
          v
+----------------------+
| Owned Framebuffer    |
+----------------------+
          |
          v
+----------------------+
| SDL/Win32 Present    |
+----------------------+
```

這樣分層後：

```text
ImGui = UI frontend
Your software ImGui backend = UI rendering backend
Owned framebuffer = render target
SDL/Win32/macOS = display backend
```

## 8. 為什麼不先用 SDL_LockTexture 當主 framebuffer

`SDL_LockTexture()` 可以給 temporary writable pixels pointer，但不適合當第一版主 framebuffer ownership。

原因：

```text
pointer only valid between lock/unlock
old contents not guaranteed
pitch decided by SDL/backend
texture memory may be staging/upload memory
RenderDevice must bind/rebind per frame
renderer loop must follow SDL lock lifetime
```

如果使用 lock texture，render loop 會被迫變成：

```text
PollEvents
SDL_LockTexture
Bind locked pixels to RenderDevice
Render scene/UI
SDL_UnlockTexture
SDL_RenderCopy / SDL_RenderTexture
SDL_RenderPresent
```

這不是錯，但它把 renderer core 生命週期拉向 SDL。

第一版更清楚：

```text
PollEvents
Render scene/UI into owned Framebuffer
DisplayBackend::Present(framebuffer)
```

SDL backend 內部再決定要：

```text
SDL_UpdateTexture
or later optimized path
```

## 9. 建議的長期架構

```text
src/core/
  framebuffer.h
  render_target.h
  render_device.h
  application.h

src/render/
  rasterizer.h
  software_renderer.h
  shader.h

src/ui/
  ui_context.h
  ui_renderer.h
  imgui_software_backend.h

src/platform/
  display_backend.h
  sdl_display_backend.h
  win32_display_backend.h
  macos_display_backend.h
```

資料流：

```text
Application
    |
    v
Input state  <------------------- DisplayBackend events
    |
    v
Scene + UI frontend
    |
    | produces draw commands
    v
Software renderer backend
    |
    | writes pixels
    v
Owned Framebuffer / RenderTarget
    |
    | present only
    v
Display backend
    |
    | SDL_UpdateTexture / BitBlt / native present
    v
Window
```

## 10. 重構順序建議

第一步不是急著加 SDL，而是把 ownership 邊界拉正。

```text
Step 1:
  introduce renderer-owned Framebuffer

Step 2:
  make RenderDevice write this Framebuffer

Step 3:
  change ScreenManager/DisplayBackend to Present(const Framebuffer&)

Step 4:
  keep Win32 backend working by copying owned framebuffer into DIBSection

Step 5:
  add SDLDisplayBackend using SDL_UpdateTexture

Step 6:
  build UI renderer / ImGui software backend on top of owned framebuffer
```

這樣每一步都保住同一個核心：

```text
All rendering details happen before present.
Platform layer only displays final pixels.
```

## 11. 暫時決策：SDL/macOS 先記錄，之後再處理

目前 SDL/macOS 支援不是主線優先事項。短期情境只是外出時用 macOS，需要先理解可行方向，不一定要馬上重構。

當之後真的要處理 macOS SDL support 時，方向應該是：

```text
Do not:
  directly patch Win32 ScreenManager into macOS-specific code
  replace RenderDevice::SetPixel with SDL draw calls
  let SDL_Texture become the primary framebuffer

Do:
  keep renderer-owned Framebuffer as the long-term target
  introduce DisplayBackend interface
  implement SDLDisplayBackend as present-only backend
  use SDL_UpdateTexture first
  keep Win32 backend as native/reference backend if useful
```

短期 reality check：

```text
Current project:
  Windows-only Makefile
  g++.exe
  -lgdi32 -luser32
  <windows.h>
  QueryPerformanceCounter
  VK_ESCAPE

macOS SDL support needs:
  platform abstraction
  non-Win32 timer/input path
  SDL include/link setup
  preferably CMake or platform-aware build scripts
```

所以先記錄決策：

```text
SDL/macOS is a future portability task.
Do not interrupt current renderer/tutorial work for it yet.
When implemented, use it to validate DisplayBackend + owned Framebuffer design.
```

## 12. 最短結論

如果目標是完全掌握 framebuffer、自己寫 UI、甚至自己接 ImGui：

```text
Do:
  own CPU framebuffer in renderer/core
  write all pixels yourself
  treat SDL/Win32/macOS as present-only backends

Do not:
  let SDL_Texture become the main framebuffer
  let Win32 DIBSection ownership define renderer memory
  replace SetPixel/DrawLine/DrawTriangle with SDL draw calls
```

真正要保住的邊界：

```text
UI / Scene decides what to draw.
Software renderer decides how pixels are produced.
Framebuffer stores the result.
Display backend only presents the result.
```
