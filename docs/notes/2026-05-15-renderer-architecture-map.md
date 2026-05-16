# Pixel-Renderer Architecture Map

這份筆記是 Pixel-Renderer 的架構地圖，用來描述未來抽象分離時，各層責任、資料流、ownership 與 backend 替換點。

核心目標：

```text
Rendering details stay in renderer/core.
Platform backend only handles window, input, and present.
```

## 1. 目前架構地圖

目前專案的 Win32 path 大致是：

```text
CURRENT STRUCTURE
=================

+-----------------------------+
| Application / main          |
|                             |
| - frame loop                |
| - decides what to draw      |
| - owns ScreenManager        |
| - owns RenderDevice         |
| - owns Rasterizer           |
+-----------------------------+
              |
              v
+-----------------------------+
| ScreenManager               |
|                             |
| - Win32 window              |
| - Win32 input               |
| - Win32 DIBSection          |
| - owns frame_buffer pointer |
| - BitBlt present            |
+-----------------------------+
              |
              | GetFrameBuffer()
              v
+-----------------------------+
| RenderDevice                |
|                             |
| - Clear                     |
| - SetPixel                  |
| - writes ScreenManager      |
|   owned memory              |
+-----------------------------+
              ^
              |
+-----------------------------+
| Rasterizer                  |
|                             |
| - DrawLine                  |
| - DrawTriangle              |
+-----------------------------+
```

問題不是 `RenderDevice::SetPixel()` 本身，而是 `ScreenManager` 同時承擔太多責任：

```text
ScreenManager =
  window
  input
  framebuffer ownership
  present
  Win32 platform code
```

## 2. 目標架構地圖

目標是把「渲染」和「顯示」拆開：

```text
TARGET STRUCTURE
================

+-----------------------------+
| Application                 |
|                             |
| - frame loop                |
| - calls OnUpdate/OnRender   |
| - owns renderer/core state  |
| - owns DisplayBackend       |
+-----------------------------+
              |
              v
+-----------------------------+
| Rendering Frontend          |
|                             |
| decides WHAT to draw        |
| - demo scene                |
| - UI commands               |
| - ImGui draw data later     |
| - draw order                |
+-----------------------------+
              |
              | draw commands / direct calls
              v
+-----------------------------+
| Software Rendering Backend  |
|                             |
| decides HOW to draw         |
| - Rasterizer                |
| - DrawLine                  |
| - DrawTriangle              |
| - depth test later          |
| - alpha blend later         |
| - shader interface later    |
+-----------------------------+
              |
              | SetPixel / spans / triangles
              v
+-----------------------------+
| Owned Framebuffer           |
|                             |
| - color buffer              |
| - depth buffer later        |
| - pitch / stride            |
| - pixel format              |
+-----------------------------+
              |
              | Present(framebuffer)
              v
+-----------------------------+
| DisplayBackend Interface    |
|                             |
| - Init                      |
| - PollEvents                |
| - Present                   |
| - ShouldClose               |
| - IsKeyDown                 |
+-----------------------------+
              |
       +------+------+
       |             |
       v             v
+-------------+  +-------------+
| Win32Backend|  | SDLBackend  |
|             |  |             |
| - DIBSection|  | - SDL_Window|
| - BitBlt    |  | - SDL_Texture
| - Win32 msg |  | - SDL events|
+-------------+  +-------------+
       |             |
       +------+------+
              v
+-----------------------------+
| OS compositor / monitor     |
+-----------------------------+
```

## 3. 三種 backend 的替換點

核心 renderer 不應該知道 Win32、SDL 或 macOS。替換點只在 `DisplayBackend`。

```text
Renderer/core common path
=========================

Application / Scene / UI
    |
    v
Rasterizer / RenderDevice
    |
    v
Owned Framebuffer
    |
    v
DisplayBackend::Present(framebuffer)
```

Win32 implementation：

```text
Win32DisplayBackend::Present(framebuffer)
    |
    | copy owned framebuffer
    v
Win32 DIBSection
    |
    | BitBlt
    v
Window DC
    |
    v
DWM compositor / monitor
```

SDL implementation：

```text
SDLDisplayBackend::Present(framebuffer)
    |
    | SDL_UpdateTexture
    v
SDL_Texture
    |
    | SDL_RenderCopy / SDL_RenderTexture
    v
SDL renderer backbuffer
    |
    | SDL_RenderPresent
    v
OS compositor / monitor
```

Future macOS native implementation：

```text
MacOSDisplayBackend::Present(framebuffer)
    |
    | copy/upload pixels
    v
Native surface / texture
    |
    | platform present path
    v
Quartz/Metal/compositor / monitor
```

## 4. Ownership Map

最重要的是 ownership：

```text
CURRENT
=======

ScreenManager owns framebuffer
RenderDevice borrows platform memory
Display and framebuffer ownership are coupled


TARGET
======

Renderer/core owns framebuffer
RenderDevice writes renderer-owned memory
DisplayBackend only reads/copies/uploads final pixels
```

圖：

```text
BEFORE
------

+---------------+      pointer       +--------------+
| ScreenManager | -----------------> | RenderDevice |
| DIBSection    |                    | SetPixel     |
+---------------+                    +--------------+


AFTER
-----

+---------------+      writes        +---------------+
| RenderDevice  | -----------------> | Framebuffer   |
| SetPixel      |                    | owned by core |
+---------------+                    +---------------+
                                             |
                                             | read/copy/upload
                                             v
                                     +---------------+
                                     | DisplayBackend|
                                     | Present(fb)   |
                                     +---------------+
```

## 5. Frame Loop Map

未來 frame loop 應該像這樣：

```text
FRAME LOOP
==========

while running:

  1. DisplayBackend::PollEvents()
       |
       v
     update InputState

  2. Application::OnUpdate(dt)
       |
       v
     update scene / UI state

  3. Framebuffer.Clear()
       |
       v
     reset color/depth

  4. Application::OnRender()
       |
       v
     Rendering Frontend decides what to draw

  5. Rasterizer / UI Renderer
       |
       v
     writes pixels into owned Framebuffer

  6. DisplayBackend::Present(framebuffer)
       |
       v
     Win32 BitBlt or SDL_RenderPresent
```

Compact view：

```text
+-------------+
| PollEvents  |
+-------------+
       |
       v
+-------------+
| OnUpdate    |
+-------------+
       |
       v
+-------------+
| Clear FB    |
+-------------+
       |
       v
+-------------+
| OnRender    |
+-------------+
       |
       v
+-------------+
| Rasterize   |
+-------------+
       |
       v
+-------------+
| Framebuffer |
+-------------+
       |
       v
+-------------+
| Present     |
+-------------+
```

## 6. UI / ImGui Map

自製 UI 或 ImGui 不應該直接呼叫 SDL drawing API。它們應該走 software renderer。

```text
UI / ImGui frontend
    |
    | produces rectangles / triangles / text commands
    v
UI software renderer
    |
    | scissor / rasterize / alpha blend
    v
Owned Framebuffer
    |
    | present only
    v
SDL or Win32 DisplayBackend
```

ImGui software backend：

```text
ImGui widgets
    |
    v
ImDrawData
    |
    v
Your ImGui software backend
    |
    | iterate cmd lists
    | apply clip rect
    | rasterize indexed triangles
    | sample font atlas
    | alpha blend
    v
Owned Framebuffer
    |
    v
DisplayBackend::Present
```

不要走：

```text
UI -> SDL_RenderDrawRect
UI -> SDL_RenderGeometry
UI -> platform framebuffer pointer
```

## 7. Interface Sketch

`DisplayBackend` 的核心不是回傳 framebuffer pointer，而是 present renderer-owned framebuffer。

```cpp
class DisplayBackend {
public:
    virtual ~DisplayBackend() = default;

    virtual bool Init(int width, int height, const char* title) = 0;
    virtual void Shutdown() = 0;

    virtual void PollEvents() = 0;
    virtual bool ShouldClose() const = 0;
    virtual bool IsKeyDown(KeyCode key) const = 0;

    virtual void Present(const Framebuffer& framebuffer) = 0;
};
```

核心差異：

```text
Avoid:
  DisplayBackend::GetFrameBuffer()
  RenderDevice writes platform-owned memory

Prefer:
  DisplayBackend::Present(const Framebuffer&)
  DisplayBackend only consumes final pixels
```

## 8. 重構地圖

建議順序：

```text
Step 1:
  Add renderer-owned Framebuffer / RenderTarget.

Step 2:
  Make RenderDevice bind/write owned Framebuffer.

Step 3:
  Change Application to own Framebuffer.

Step 4:
  Introduce DisplayBackend interface.

Step 5:
  Convert current ScreenManager into Win32DisplayBackend.

Step 6:
  Make Win32DisplayBackend::Present copy owned framebuffer into DIBSection,
  then BitBlt.

Step 7:
  Add SDLDisplayBackend using SDL_UpdateTexture.

Step 8:
  Move build system toward platform-aware build, preferably CMake.

Step 9:
  Build UI renderer / ImGui software backend on top of owned framebuffer.
```

## 9. 最短結論

這張地圖要保住的核心邊界：

```text
Rendering Frontend:
  decides what to draw

Software Rendering Backend:
  produces pixels

Owned Framebuffer:
  stores render result

Display Backend:
  presents final pixels
```

只要這個邊界穩定，Win32、SDL、macOS 都只是 present backend；自製 UI 和 ImGui 都能在同一個 owned framebuffer 上完全由自己控制。
