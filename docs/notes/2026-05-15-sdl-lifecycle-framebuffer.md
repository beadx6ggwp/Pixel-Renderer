# SDL Lifecycle、Framebuffer 與 Present Path

這份筆記要釐清一件事：在 software renderer 裡使用 SDL，並不代表把 rasterization 交給 SDL 或 GPU。SDL 可以被用成 platform layer 與 present layer，讓我們把 CPU 算好的 framebuffer 顯示到 window。

重點不是背 API 名稱，而是理解這幾個邊界：

- `RenderDevice::SetPixel()` 寫的是哪一塊 memory？
- `SDL_Texture` 是 CPU memory 還是 GPU/driver resource？
- `SDL_UpdateTexture()` 與 `SDL_LockTexture()` 為什麼都存在？
- `SDL_RenderPresent()` 之後，畫面怎麼進到 OS compositor 與 monitor？

參考文件：

- SDL2 wiki: https://wiki.libsdl.org/SDL2
- SDL2 migration guide: https://wiki.libsdl.org/SDL2/MigrationGuide
- `SDL_LockTexture`: https://wiki.libsdl.org/SDL_LockTexture
- `SDL_RenderPresent`: https://wiki.libsdl.org/SDL2/SDL_RenderPresent

## 1. SDL 的總定位

SDL 不是 renderer core，也不是單純的 framebuffer wrapper。比較準確的定位是：

```text
SDL = cross-platform platform / media abstraction layer
```

它把不同 OS 的底層能力包成同一套 C API：

```text
Window      : Win32 HWND / Cocoa NSWindow / X11 Window / Wayland surface
Input       : keyboard / mouse / controller / touch
Event loop  : OS message queue
Timer       : high-resolution counter
Audio       : audio device abstraction
Renderer    : optional 2D rendering API
Texture     : renderer 能使用的 image resource
Present     : 把 final frame 交給 window/compositor/display
```

對 Pixel-Renderer 來說，SDL 最重要的角色是：

```text
Your CPU rasterizer
    -> CPU framebuffer
    -> SDL present path
    -> window
```

而不是：

```text
Your triangles
    -> SDL_RenderGeometry / OpenGL shader
    -> GPU rasterization
```

## 2. SDL 的幾種 graphics 用法

SDL 的教學看起來會互相矛盾，是因為 SDL 可以站在不同層級。

### 2.1 Window only

SDL 只負責開 window、收 input，真正繪圖交給 OpenGL/Vulkan/Metal。

```text
SDL_CreateWindow()
SDL_GL_CreateContext() or Vulkan/Metal setup
OpenGL/Vulkan/Metal draw calls
```

這種路線會用 shader 畫三角形。triangle coverage、interpolation、fragment output 都是 GPU pipeline 處理。

### 2.2 SDL_Renderer 2D API

使用 SDL 自己的 2D renderer：

```text
SDL_RenderDrawLine
SDL_RenderFillRect
SDL_RenderCopy / SDL_RenderTexture
SDL_RenderPresent
```

這條路線是「叫 SDL 幫你畫 2D primitives」。底層可能是 Direct3D、Metal、OpenGL，也可能是 software backend。這對一般 2D app 很方便，但對 Pixel-Renderer 的 rasterizer 學習不適合，因為它會繞過你自己的 Bresenham、edge function、barycentric 等核心。

### 2.3 SDL_UpdateTexture software framebuffer path

你的 renderer 自己產生一張完整 bitmap，SDL 只負責 upload/present。

```text
Rasterizer
    -> RenderDevice::SetPixel
    -> your CPU framebuffer
    -> SDL_UpdateTexture
    -> SDL_RenderCopy / SDL_RenderTexture
    -> SDL_RenderPresent
```

這是最適合 Pixel-Renderer 第一版 SDL backend 的路線。

### 2.4 SDL_LockTexture streaming texture path

SDL 暫時借出一塊 writable texture update memory，讓你在 lock/unlock 期間寫入。

```text
SDL_LockTexture
    -> temporary pixels pointer + pitch
    -> RenderDevice::SetPixel writes locked pixels
    -> SDL_UnlockTexture
    -> SDL_RenderCopy / SDL_RenderTexture
    -> SDL_RenderPresent
```

這條路線可能少一份 application-owned CPU framebuffer copy，但會讓 renderer lifetime 跟 SDL texture lock/unlock 綁在一起。

## 3. SDL 程式完整生命週期

最小 SDL renderer lifecycle：

```text
Program start
    |
    v
SDL_Init(SDL_INIT_VIDEO)
    |
    | initialize SDL global state
    | initialize video subsystem
    | select/load platform video driver
    v
SDL_CreateWindow()
    |
    | create native OS window:
    |   Windows -> HWND
    |   macOS   -> NSWindow / NSView
    |   Linux   -> X11 Window / Wayland surface
    v
SDL_CreateRenderer()
    |
    | choose renderer backend:
    |   D3D / Metal / OpenGL / software / etc.
    | create renderer state
    | create renderer backbuffer / render target path
    v
SDL_CreateTexture(... STREAMING ...)
    |
    | create image resource used by renderer
    | may be GPU texture
    | may have staging/update path
    v
Main loop
    |
    +--> SDL_PollEvent()
    |       |
    |       | pump OS event queue
    |       | translate native events into SDL_Event
    |       v
    |
    +--> CPU software rendering
    |       |
    |       | Rasterizer
    |       | RenderDevice::SetPixel
    |       v
    |    CPU framebuffer
    |
    +--> SDL_UpdateTexture()
    |       or
    |    SDL_LockTexture() / SDL_UnlockTexture()
    |       |
    |       | copy/upload/apply pixels to SDL_Texture
    |       v
    |
    +--> SDL_RenderCopy / SDL_RenderTexture()
    |       |
    |       | draw texture to renderer backbuffer
    |       v
    |
    +--> SDL_RenderPresent()
            |
            | submit composed backbuffer
            | possibly wait for vsync
            | hand frame to OS compositor
            v
        Monitor

Shutdown
    |
    v
SDL_DestroyTexture()
SDL_DestroyRenderer()
SDL_DestroyWindow()
SDL_Quit()
```

## 4. Init 階段背後發生什麼

`SDL_Init(SDL_INIT_VIDEO)` 不是開 window，而是初始化 SDL 的 video subsystem。

概念上它會做這些事：

```text
SDL_Init(SDL_INIT_VIDEO)
    |
    +-- setup SDL global/internal state
    +-- initialize event subsystem needed by video
    +-- choose platform video driver
    +-- prepare OS-specific video backend
```

平台差異大致像這樣：

```text
Windows -> Win32 video backend
macOS   -> Cocoa backend
Linux   -> X11 or Wayland backend
```

所以 SDL 的第一層價值是：你的 app 不再直接碰 `RegisterClass`、`CreateWindow`、`NSWindow`、X11/Wayland event details。

## 5. Window 階段背後發生什麼

`SDL_Window` 是 native window 的 SDL handle。

```text
Your code
    |
    | SDL_CreateWindow(...)
    v
+------------------------+
| SDL_Window             |
| cross-platform handle  |
+------------------------+
    |
    v
+------------------------+
| Native OS window       |
| Windows: HWND          |
| macOS: NSWindow/View   |
| Linux: X11/Wayland     |
+------------------------+
```

`SDL_Window` 本身不是 framebuffer。它代表一個 OS window，之後 renderer 或 surface path 才會把 pixels 顯示到這個 window。

## 6. Renderer 階段背後發生什麼

`SDL_Renderer` 是 SDL 的 2D rendering context。它不是你的 software rasterizer，也不是固定等於 GPU，但它底層可能用 GPU API 實作。

```text
SDL_CreateRenderer(window, ...)
    |
    +-- choose backend
    |     Windows: D3D / OpenGL / software
    |     macOS:   Metal / OpenGL / software
    |     Linux:   OpenGL / software / platform backend
    |
    +-- create renderer state
    +-- prepare renderer backbuffer or equivalent render target
```

當你呼叫：

```text
SDL_RenderDrawLine
SDL_RenderFillRect
SDL_RenderCopy / SDL_RenderTexture
```

你是在使用 SDL renderer API。SDL 可能把它翻譯成 GPU draw calls，也可能在 software backend 裡自己畫。

對 Pixel-Renderer 的限制是：

```text
不要用 SDL_RenderDrawLine 畫 Bresenham line
不要用 SDL_RenderGeometry 畫 triangle
不要用 SDL_RenderDrawPoint 取代 SetPixel
```

因為那會讓 SDL/backend 接管繪圖本體。

## 7. Texture 階段背後發生什麼

`SDL_Texture` 是 renderer 可以使用的 image resource。

```text
SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    width,
    height
)
```

概念上：

```text
+--------------------------+
| SDL_Texture              |
|                          |
| If GPU backend:          |
|   GPU texture or driver  |
|   managed resource       |
|                          |
| If software backend:     |
|   SDL-managed image data |
+--------------------------+
```

重點：`SDL_Texture` 不保證是一塊你可以永久拿 `uint32_t*` 直接寫的 linear CPU memory。

原因包括：

- GPU texture 可能在 VRAM 或 driver-private memory。
- texture layout 可能是 tiled/swizzled，不是 CPU 常見的 `y * pitch + x * 4`。
- GPU 可能正在 sample/render 這張 texture，需要 synchronization。
- CPU update 可能要經過 staging/upload buffer。

## 8. Event loop 背後發生什麼

Win32 版本現在做的是：

```text
PeekMessage
TranslateMessage
DispatchMessage
WindowProc
```

SDL 版本會變成：

```text
SDL_Event event;
while (SDL_PollEvent(&event)) {
    ...
}
```

底層對照：

```text
Native OS event                  SDL_Event
-------------------------------  -------------------------
WM_KEYDOWN                       SDL_EVENT_KEY_DOWN
WM_MOUSEMOVE                     SDL_EVENT_MOUSE_MOTION
WM_CLOSE                         SDL_EVENT_QUIT
Cocoa NSEvent                    SDL_Event
X11 / Wayland event              SDL_Event
```

資料流：

```text
OS event queue
    |
    | SDL_PollEvent
    v
+-------------------+
| SDL_Event         |
| normalized input  |
+-------------------+
    |
    v
Application state
```

所以 SDL 也會替換目前 `ScreenManager::DispatchEvents()` 與 Win32 key code handling，而不是只替換 framebuffer。

## 9. Frame loop 背後發生什麼

software renderer 的一幀可以拆成兩個完全不同的問題：

```text
Software rasterization:
  哪些 pixels 被 triangle 覆蓋？
  每個 pixel 是什麼 color？
  depth test 怎麼做？
  interpolation / shading / texture sampling 怎麼做？

Presentation:
  已經算好的 final image 要怎麼進 window？
  CPU memory 要不要 copy/upload？
  backbuffer 何時 present？
  OS compositor 何時合成？
```

Pixel-Renderer 的核心在第一段。SDL 主要處理第二段。

```text
CPU side                                      SDL / Driver / GPU side
--------                                      -----------------------

+-----------------------+
| Your C++ Rasterizer   |
| edge/barycentric/etc. |
+-----------------------+
          |
          | CPU stores
          v
+-----------------------+
| CPU framebuffer       |
| linear y*pitch+x*4    |
+-----------------------+
          |
          | update / lock path
          v
                                               +----------------------+
                                               | SDL_Texture          |
                                               +----------------------+
                                                          |
                                                          | render texture
                                                          v
                                               +----------------------+
                                               | Renderer backbuffer  |
                                               +----------------------+
                                                          |
                                                          | present
                                                          v
                                               +----------------------+
                                               | OS compositor/display|
                                               +----------------------+
```

## 10. `SDL_UpdateTexture` 路徑

`SDL_UpdateTexture()` 的 ownership model：

```text
你的程式擁有 CPU framebuffer。
SDL 只在 present 前把這份 pixels copy/upload 到 SDL_Texture。
```

用法概念：

```cpp
std::vector<uint32_t> framebuffer(width * height);
int pitch = width * sizeof(uint32_t);

// software renderer writes framebuffer
framebuffer[y * width + x] = color;

// present path uploads/copies it into SDL_Texture
SDL_UpdateTexture(texture, nullptr, framebuffer.data(), pitch);
```

ASCII：

```text
SDL_UpdateTexture path
======================

CPU                                      SDL / Driver / GPU
---                                      ------------------

+---------------------+
| your framebuffer    |
| uint32_t pixels[]   |
| owned by project    |
| CPU-writable        |
+---------------------+
          |
          | SDL_UpdateTexture(texture, pixels, pitch)
          | copy / convert / upload
          v
                                      +---------------------+
                                      | SDL_Texture         |
                                      | renderer resource   |
                                      +---------------------+
                                                 |
                                                 | SDL_RenderCopy /
                                                 | SDL_RenderTexture
                                                 v
                                      +---------------------+
                                      | renderer backbuffer |
                                      +---------------------+
                                                 |
                                                 | SDL_RenderPresent
                                                 v
                                      +---------------------+
                                      | compositor/display  |
                                      +---------------------+
```

優點：

- `RenderDevice` 永遠寫自己擁有的 CPU memory。
- framebuffer pointer lifetime 穩定。
- screenshot、test、debug 都容易。
- SDL 只在 `Present()` 階段接手。

缺點：

- 每 frame 多一次 CPU framebuffer 到 SDL texture 的 copy/upload。

對第一版 cross-platform backend 來說，這個缺點通常可以接受，因為它換來非常乾淨的架構邊界。

## 11. `SDL_LockTexture` / `SDL_UnlockTexture` 路徑

`SDL_LockTexture()` 的 ownership model：

```text
SDL owns texture/update memory。
你的程式在 lock/unlock 期間暫時取得 writable pixels pointer。
```

用法概念：

```cpp
void* pixels = nullptr;
int pitch = 0;

SDL_LockTexture(texture, nullptr, &pixels, &pitch);

uint8_t* base = static_cast<uint8_t*>(pixels);
uint32_t* row = reinterpret_cast<uint32_t*>(base + y * pitch);
row[x] = color;

SDL_UnlockTexture(texture);
```

官方文件的重點限制：

- texture 必須是 `SDL_TEXTUREACCESS_STREAMING`。
- 回傳的 `pixels` 只在 lock 到 unlock 期間有效。
- old texture data 不保證存在，應視為 write-only。
- 寫完必須 `SDL_UnlockTexture()`，變更才會套用。

ASCII：

```text
SDL_LockTexture path
====================

CPU                                      SDL / Driver / GPU
---                                      ------------------

SDL_LockTexture(texture)
          |
          v
+---------------------+
| temporary pixels    |
| void* pixels        |
| int pitch           |
| CPU-writable        |
| valid until unlock  |
+---------------------+
          |
          | RenderDevice::SetPixel writes here
          v
+---------------------+
| modified pixels     |
+---------------------+
          |
          | SDL_UnlockTexture(texture)
          | apply / upload / make visible
          v
                                      +---------------------+
                                      | SDL_Texture         |
                                      | renderer resource   |
                                      +---------------------+
                                                 |
                                                 | SDL_RenderCopy /
                                                 | SDL_RenderTexture
                                                 v
                                      +---------------------+
                                      | renderer backbuffer |
                                      +---------------------+
                                                 |
                                                 | SDL_RenderPresent
                                                 v
                                      +---------------------+
                                      | compositor/display  |
                                      +---------------------+
```

`SDL_LockTexture()` 不代表 CPU 拿到真正 GPU VRAM 的永久 pointer。更實際的理解是：

```text
SDL/driver prepares CPU-writable update memory
    -> CPU writes pixels
    -> SDL_UnlockTexture
    -> SDL/driver applies, converts, or uploads to actual texture resource
```

可能的底層模型：

```text
CPU                                SDL / Driver                         GPU
---                                ------------                         ---

SDL_LockTexture()
        |
        v
                           +----------------------+
                           | staging memory       |
                           | CPU-writable         |
                           +----------------------+
                                      ^
                                      |
CPU writes pixels -------------------+
                                      |
SDL_UnlockTexture()                   |
        |                             |
        v                             v
                           copy / convert / upload
                                      |
                                      v
                                                               +------------------+
                                                               | GPU texture      |
                                                               | tiled/private    |
                                                               +------------------+
                                                                         |
                                                                         | sample/copy
                                                                         v
                                                               renderer backbuffer
```

`LockTexture` 的優點：

- 對某些 backend 可能比較接近 streaming texture 的理想更新路徑。
- 可能減少你自己維護的一份 framebuffer 到 SDL staging memory 的 copy。

缺點：

- `RenderDevice` 的 framebuffer pointer 只在 lock/unlock 期間有效。
- 每 frame 可能需要 rebind framebuffer pointer。
- frame loop 需要 `BeginFrame()` / `EndFrame()` 之類的生命週期。
- 不能假設舊 pixels 還在。

## 12. 為什麼 CPU 不能永遠直接寫 GPU texture

最直覺的 CPU framebuffer 是：

```text
CPU
 |
 | store instruction
 v
+-------------------------+
| System RAM              |
| uint32_t framebuffer[]  |
+-------------------------+
```

這種 memory 可以長期持有 pointer：

```cpp
uint32_t* framebuffer = new uint32_t[width * height];
framebuffer[y * width + x] = color;
```

但 GPU texture 常常不是這種普通 linear memory。

### 12.1 GPU texture 可能不在 CPU 可直接尋址的普通 RAM

```text
CPU side                         GPU side
--------                         --------

+-------------+                  +----------------+
| CPU cores   |                  | GPU cores      |
+-------------+                  +----------------+
      |                                  |
      v                                  v
+-------------+                  +----------------+
| System RAM  |                  | VRAM / GPU mem |
+-------------+                  +----------------+
```

如果 texture 在 GPU local memory，CPU 不一定有永久、普通、便宜的 pointer 可以直接寫。

### 12.2 GPU texture layout 可能不是 linear

CPU 想像的 linear layout：

```text
row 0: A B C D E F G H
row 1: I J K L M N O P
row 2: Q R S T U V W X

address = base + y * pitch + x * 4
```

GPU 為了 cache locality，可能使用 tiled/swizzled layout：

```text
possible tiled layout:

A B I J C D K L
Q R E F S T G H
...
```

所以即使某個 backend 可以 map texture memory，也不代表它的 physical/driver layout 就是 `y * pitch + x * 4`。

### 12.3 GPU 可能正在使用這張 texture

```text
GPU command queue:
  draw using texture T
  draw using texture T
  present

CPU:
  writes texture T at the same time
```

這會產生同步問題：

```text
GPU 讀到舊資料？
GPU 讀到新資料？
同一個 draw 中途資料改變？
```

所以 graphics API 和 driver 需要明確的 synchronization 或 staging/update path。`LockTexture` / `UnlockTexture` 就是在 SDL 抽象層裡表達這段生命週期。

### 12.4 CPU 寫 GPU memory 可能很慢

就算 CPU 可以 map 某些 GPU-visible memory，寫入也可能牽涉：

```text
CPU write
  -> staging/upload buffer
  -> cache flush
  -> driver synchronization
  -> GPU resource transition
  -> GPU copy/upload
```

所以實務上常見模型是：

```text
CPU-writable staging memory
    |
    | copy/upload
    v
GPU-optimized texture
```

## 13. `SDL_RenderPresent` 之後發生什麼

SDL renderer API 的繪圖通常不是每呼叫一次就直接出現在螢幕上。它會先畫到 renderer backbuffer 或 equivalent render target，最後由 `SDL_RenderPresent()` submit。

概念流程：

```text
SDL_RenderCopy / SDL_RenderTexture
    |
    | draw texture to renderer backbuffer
    v
+----------------------+
| renderer backbuffer  |
+----------------------+
    |
    | SDL_RenderPresent
    | flush/submit backend commands
    | maybe wait for vsync
    v
+----------------------+
| OS compositor        |
| DWM / Quartz / etc.  |
+----------------------+
    |
    | compose all windows:
    | browser, terminal, your window, desktop...
    v
+----------------------+
| final desktop image  |
+----------------------+
    |
    | display scanout
    v
+----------------------+
| monitor              |
+----------------------+
```

如果底層是 GPU backend，可能是：

```text
draw textured quad/copy
    -> GPU render target
    -> present/swap path
    -> compositor
    -> scanout
```

如果底層是 software backend，可能是：

```text
software backbuffer
    -> window surface update
    -> compositor
    -> scanout
```

重點：`SDL_RenderPresent()` 是 presentation boundary，不是 triangle rasterization boundary。

## 14. Win32 BitBlt vs SDL Present 對照

Pixel-Renderer 目前 Win32 path：

```text
WIN32 CURRENT PATH
==================

CPU side                                             OS / GPU / display side
--------                                             ----------------------

+-----------------------------+
| Your C++ code               |
| Rasterizer                  |
| RenderDevice::SetPixel()    |
+-----------------------------+
              |
              | CPU store
              | *(frame_buffer + y*pitch + x*4) = color
              v
+--------------------------------------------------+
| CPU-addressable memory                           |
| Win32 DIBSection backing memory                  |
| frame_buffer                                     |
| Created by: CreateDIBSection()                   |
| Owned by:   GDI / Win32 bitmap object            |
| Writable by CPU: yes                             |
+--------------------------------------------------+
              |
              | BitBlt(mem_dc -> window_dc)
              | CPU/GDI copy or driver-assisted blit
              v
+--------------------------------------------------+        +------------------+
| Window surface / GDI target                      | -----> | DWM compositor   |
| Owned by OS / window system                      |        | desktop image    |
+--------------------------------------------------+        +------------------+
                                                                     |
                                                                     | present / scanout
                                                                     v
                                                          +----------------------+
                                                          | GPU display pipeline |
                                                          | monitor              |
                                                          +----------------------+
```

SDL `UpdateTexture` path：

```text
SDL UPDATE TEXTURE PATH
=======================

CPU side                                             SDL / GPU / display side
--------                                             ------------------------

+-----------------------------+
| Your C++ code               |
| Rasterizer                  |
| RenderDevice::SetPixel()    |
+-----------------------------+
              |
              | CPU store
              v
+--------------------------------------------------+
| CPU memory                                       |
| Your framebuffer                                 |
| std::vector<uint32_t> or new uint32_t[]          |
| Owned by:   SDLDisplayBackend / project          |
| Writable by CPU: yes                             |
+--------------------------------------------------+
              |
              | SDL_UpdateTexture(texture, pixels, pitch)
              | copy / upload
              v
+--------------------------------------------------+        +------------------+
| SDL_Texture                                      | -----> | Renderer backbuf |
| If GPU backend: GPU texture or driver resource   |        | Owned by SDL /   |
| If software backend: SDL-managed CPU resource    |        | backend          |
| Writable by CPU directly: no, not normally       |        +------------------+
+--------------------------------------------------+                 |
              ^                                                       |
              | SDL_RenderCopy / SDL_RenderTexture                    |
              +-------------------------------------------------------+
                                                                      |
                                                                      | SDL_RenderPresent()
                                                                      v
                                                          +----------------------+
                                                          | OS compositor        |
                                                          | GPU display pipeline |
                                                          | monitor              |
                                                          +----------------------+
```

SDL `LockTexture` path：

```text
SDL LOCK TEXTURE PATH
=====================

CPU side                                             SDL / GPU / display side
--------                                             ------------------------

+-----------------------------+
| Your C++ code               |
| SDL_LockTexture()           |
+-----------------------------+
              |
              | asks SDL for writable pixels
              v
+--------------------------------------------------+
| Temporary CPU-writable pointer                   |
| void* pixels                                     |
| int pitch                                        |
| Actually may be:                                 |
| - mapped staging memory                          |
| - backend upload buffer                          |
| - software texture memory                        |
| - SDL-managed temporary buffer                   |
| Owned by: SDL / renderer backend                 |
| Valid only between Lock and Unlock               |
| Old contents: not guaranteed                     |
+--------------------------------------------------+
              |
              | CPU store
              | RenderDevice::SetPixel() writes here
              v
+--------------------------------------------------+
| Locked texture update memory                     |
+--------------------------------------------------+
              |
              | SDL_UnlockTexture()
              | apply / upload / make visible to renderer
              v
+--------------------------------------------------+        +------------------+
| SDL_Texture                                      | -----> | Renderer backbuf |
| If GPU backend: GPU texture or driver resource   |        +------------------+
| If software backend: CPU texture resource        |                 |
+--------------------------------------------------+                 |
              ^                                                       |
              | SDL_RenderCopy / SDL_RenderTexture                    |
              +-------------------------------------------------------+
                                                                      |
                                                                      | SDL_RenderPresent()
                                                                      v
                                                          +----------------------+
                                                          | OS compositor        |
                                                          | GPU display pipeline |
                                                          | monitor              |
                                                          +----------------------+
```

## 15. Pixel-Renderer 的設計結論

不要把 Pixel-Renderer 的 software rasterization 替換成 SDL drawing API：

```text
Do NOT:

DrawLine      -> SDL_RenderDrawLine
DrawTriangle  -> SDL_RenderGeometry
SetPixel      -> SDL_RenderDrawPoint
```

因為這會把 rasterization 工作交給 SDL/backend/GPU。

正確方向是：

```text
Rasterizer
    -> RenderDevice::SetPixel
    -> CPU framebuffer
    -> DisplayBackend::Present
    -> SDL_UpdateTexture or SDL_LockTexture
    -> SDL_RenderPresent
```

更完整的分層：

```text
+--------------------------------------------------+
| Application                                      |
| - owns frame loop                                |
| - calls DisplayBackend::PollEvents               |
| - calls OnUpdate / OnRender                      |
+--------------------------------------------------+
        |
        v
+--------------------------------------------------+
| Rasterizer / RenderDevice                        |
| - DrawLine / DrawTriangle                        |
| - SetPixel                                       |
| - writes linear CPU framebuffer                  |
+--------------------------------------------------+
        |
        v
+--------------------------------------------------+
| DisplayBackend                                   |
| - Win32DisplayBackend: DIBSection + BitBlt       |
| - SDLDisplayBackend: CPU buffer + SDL_Texture    |
| - later: MacOSDisplayBackend                     |
+--------------------------------------------------+
        |
        v
+--------------------------------------------------+
| OS compositor / display                          |
+--------------------------------------------------+
```

第一版 SDL backend 建議：

```text
Use:
  std::vector<uint32_t> framebuffer
  SDL_UpdateTexture()

Reason:
  framebuffer lifetime stable
  RenderDevice does not know SDL
  easy screenshot/test/debug
  clean ownership boundary
```

之後若要優化，再考慮：

```text
Use:
  SDL_LockTexture / SDL_UnlockTexture

Only if:
  frame loop has BeginFrame / EndFrame
  RenderDevice can rebind framebuffer pointer per frame
  code accepts that old texture contents are not guaranteed
```

## 16. 最短心智模型

```text
Software rasterization answers:

  Which pixels should exist?
  What color/depth should each pixel have?

Presentation answers:

  How does the final image reach a window?
  How is CPU memory copied/uploaded?
  How does the OS compositor display it?
```

Pixel-Renderer 應該保住這條線：

```text
Your math
  -> Your rasterizer
  -> Your SetPixel
  -> Your CPU framebuffer
  -> SDL only presents final image
```

而不是走成：

```text
Your triangles
  -> SDL/GPU draws primitives
```

這樣 SDL 就只是 cross-platform display backend，而不是 renderer core。
