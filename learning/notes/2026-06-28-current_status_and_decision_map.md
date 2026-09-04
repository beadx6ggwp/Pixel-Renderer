# Pixel-Renderer 當前現況分析與選擇地圖

日期: 2026-06-28

狀態: rough analysis note

目的: 重新整理 Pixel-Renderer 目前的 source, docs, notes, roadmap, 並把下一步可以做的選擇攤開, 讓後續決策有共同基準.

---

## 0. 這篇筆記為什麼重寫

這篇不是新的 roadmap, 也不是要取代 `docs/PROJECT_MAP.md` 或 `docs/ARCHITECTURE.md`.

這篇要回答的是:

```text
現在 Pixel-Renderer 到底處於什麼狀態?
3 月以來累積的 source, docs, notes 分別代表什麼?
哪些想法是長期 north star?
哪些想法已經是 stable decision?
哪些想法仍是探索中的 branch?
下一個實作 branch 有哪些選擇?
每個選擇的代價是什麼?
```

這次重寫的核心判準來自 `docs/notes/2026-05-22-rendering_systems_learning_record_and_plan.md` 裡的兩句話. 我把標點改成半形, 但意思不改:

> 我想要通透理解渲染, 擁有自己的 renderer, 能靈活擴充, 自由的做各種事情; 同時也希望理解商業引擎, 未來能在 Unity / Unreal / 自研引擎中有效工作.

> 以 Pixel-Renderer 作為 Rendering Systems Lab, 先建立共同主幹, 再讓各個分支自然長出最小切片.

這兩句比任何單一 TODO 都重要.

它說明 Pixel-Renderer 不只是:

```text
software rasterizer toy project
```

也不是現在就要變成:

```text
Filament clone
SwiftShader clone
full game engine
full UI framework
FPGA GPU implementation
Unity / Unreal demo collection
```

更準確的定位是:

```text
Pixel-Renderer = Rendering Systems Lab
```

這代表每個重要概念最好都能一路連到:

```text
底層 mechanism
  -> 數學 / 幾何
  -> C++ implementation boundary
  -> debug / testing method
  -> renderer architecture
  -> commercial engine abstraction
  -> optional software GPU / FPGA branch
```

目前真正要決定的不是最終走哪一個身份, 而是:

```text
下一段共同主幹應該先補哪裡?
```

---

## 1. 本次重新檢查的範圍

本次重新看了三層.

### 1.1 Source reality

實際 source:

```text
src/main.cpp
src/types.h
src/core/application.h
src/core/application.cpp
src/core/screen_manager.h
src/core/screen_manager.cpp
src/core/render_device.h
src/core/render_device.cpp
src/render/rasterizer.h
src/render/rasterizer.cpp
makefile
README.md
```

實際 build:

```text
make debug
```

結果:

```text
make: Nothing to be done for 'debug'.
```

### 1.2 Stable docs

重新看過這些比較穩定的文件:

```text
docs/PROJECT_MAP.md
docs/ARCHITECTURE.md
docs/DEVELOPMENT.md
docs/README.md
docs/foundations/rendering_conventions.md
docs/foundations/rasterization_edge_rules.md
docs/HANDOFF.md
docs/AGENTS.md
```

這些文件代表目前 repo 已經相對沉澱的方向.

### 1.3 Notes and tutorial docs

也重新納入這些探索與教學材料:

```text
docs/notes/2026-04-06-*
docs/notes/2026-04-07-*
docs/notes/2026-04-08-*
docs/notes/2026-05-14-*
docs/notes/2026-05-15-*
docs/notes/2026-05-22-*
docs/tutorial-soft-renderer/
docs/tutorial-cpp/
```

這些不是全部都等於 immediate roadmap.

比較好的讀法是:

```text
stable docs:
  現在應該遵守的方向與規約

notes:
  保存探索脈絡, 分支想法, 當時的 reasoning

tutorial-soft-renderer:
  graphics pipeline learning and implementation order

tutorial-cpp:
  C++ engineering, backend boundary, verification, tooling path
```

---

## 2. 一句話現況

Pixel-Renderer 目前是:

```text
一個可 build 的早期 Win32 software renderer prototype,
搭配一組已經很豐富但比 source 超前的 Rendering Systems docs.
```

它已經有:

```text
Win32 window
DIBSection color buffer
frame loop
Clear
SetPixel
Bresenham-style DrawLine
bounding-box barycentric triangle fill
大量 docs / notes / tutorial
明確的 Rendering Systems Lab 定位
```

它還沒有:

```text
trusted raster core
depth buffer
edge-function coverage
top-left rule
color / depth interpolation
unit tests
debug views
owned Framebuffer
DisplayBackend boundary
viewport / MVP
IShader
Material / Renderer / Scene / View / Camera
CommandQueue
Debug UI
Engine Mirror Demo
```

所以現況不是缺少方向.

現況真正缺的是:

```text
把第一個底層可信小島做出來.
```

---

# 3. Source code 現況

## 3.1 現在的 architecture

目前 source 還是 early Win32 prototype.

```text
Application
  owns ScreenManager
  owns RenderDevice*
  owns Rasterizer*
  controls frame loop

ScreenManager
  owns Win32 window
  owns Win32 DIBSection framebuffer memory
  handles Win32 events and input state
  presents by BitBlt

RenderDevice
  borrows framebuffer pointer through FramebufferConfig
  Clear(color)
  SetPixel(x, y, color)

Rasterizer
  borrows RenderDevice*
  DrawLine(...)
  DrawTriangle(...)
```

目前資料流:

```text
MyApp::OnRender()
  -> device->Clear()
  -> demo background writes pixels
  -> rasterizer->DrawTriangle()
  -> rasterizer->DrawLine()
  -> ScreenManager::UpdateScreen()
  -> BitBlt
```

這代表 current source 的主體仍然是:

```text
Application + Win32 ScreenManager + borrowed framebuffer writer + Rasterizer
```

不是:

```text
Renderer / Scene / View / Camera engine skeleton
```

也不是:

```text
platform-independent renderer core
```

## 3.2 目前已做到的事

目前 source 實際做到:

```text
Win32 window setup
top-down DIBSection framebuffer
keyboard / mouse state
high-resolution frame timing
dynamic window title with FPS and frame count
RenderDevice::Clear()
RenderDevice::SetPixel()
Rasterizer::DrawLine()
Rasterizer::DrawTriangle()
demo background gradient
flat red triangle demo
green line demo
```

## 3.3 目前 source 的主要限制

目前限制:

```text
ScreenManager still owns framebuffer memory
RenderDevice only borrows raw pointer
no depth buffer
no color/depth interpolation
no separate Framebuffer / RenderTarget
no unit test harness
no headless output path
no debug view mode
no renderer-owned frame stats
main.cpp directly calls Rasterizer and RenderDevice
```

這些限制不是錯, 因為 prototype 本來就是這樣長出來的.

但它們決定了下一步不能假裝 source 已經是 renderer architecture.

---

# 4. Triangle rasterizer 的精確狀態

## 4.1 Current `DrawTriangle`

`Rasterizer::DrawTriangle()` 現在做的是:

```text
1. 將 v1 / v2 / v3 的 x, y cast 成 int
2. 算 min_x, max_x, min_y, max_y
3. bbox clamp 到 screen
4. 用 inclusive loop 掃描:
     for y <= max_y
     for x <= max_x
5. 在 integer coordinate (x, y) 上計算 barycentric
6. 如果 w1 >= 0, w2 >= 0, w3 >= 0, 就 SetPixel
7. 使用單一 flat color
```

它目前沒有:

```text
pixel center sampling at (x + 0.5, y + 0.5)
half-open bbox [x0, x1), [y0, y1)
edge-function coverage
top-left shared-edge rule
degenerate triangle guard
depth interpolation
depth buffer
depth test
vertex color interpolation
attribute interpolation
debug output
unit tests
```

所以 current `DrawTriangle()` 的角色是:

```text
educational prototype
```

不是:

```text
trusted raster core
```

## 4.2 和 target convention 的差距

`docs/foundations/rendering_conventions.md` 和 `docs/foundations/rasterization_edge_rules.md` 已經把 target 寫得很明確:

```text
screen origin:
  top-left

screen axes:
  x right, y down

pixel coverage:
  sample at pixel center, (x + 0.5, y + 0.5)

bbox:
  half-open ranges, [x0, x1), [y0, y1)

coverage:
  edge functions

shared edge:
  top-left rule

depth:
  [0, 1], smaller is closer, clear = 1.0

culling:
  disabled in screen-space baseline
```

也就是說, target convention 已經存在, 但 source 還沒跟上.

這就是 `render/raster-baseline` 的理由.

---

# 5. Docs 現況

## 5.1 Stable docs 已經很清楚

目前 stable docs 已經把專案定位收斂成:

```text
Pixel-Renderer = Rendering Systems Lab
```

核心主線:

```text
SetPixel / Framebuffer
  -> DrawLine
  -> Triangle Rasterization
  -> Barycentric / Edge Function
  -> Z-buffer
  -> MVP / Viewport / Perspective
  -> IShader / Stage Data
  -> Material / MaterialInstance
  -> Scene / View / Camera / Renderer
  -> CommandQueue / Backend Separation
  -> Debug UI / Tooling
  -> Engine Mirror Demo
  -> optional Software GPU / FPGA branches
```

這條線本身是合理的.

問題是不能把所有節點都當成下一步.

## 5.2 `PROJECT_MAP.md`

`PROJECT_MAP.md` 的價值是定義:

```text
Pixel-Renderer 不是只是一個 software rasterizer,
而是一個能連接 graphics pipeline, renderer architecture,
commercial engine fluency, UI, software GPU, FPGA 的 Rendering Systems Lab.
```

它回答的是:

```text
這個專案為什麼值得做?
它的知識地圖在哪裡?
它和 Unity / Unreal / Filament / bgfx / SwiftShader 有什麼關係?
```

## 5.3 `ARCHITECTURE.md`

`ARCHITECTURE.md` 的價值是定義:

```text
短期:
  trusted screen-space raster core

中期:
  owned Framebuffer
  DisplayBackend
  viewport / MVP
  IShader boundary

長期:
  Renderer / Scene / View / Camera
  Material / MaterialInstance
  CommandQueue
  Debug UI
  Engine Mirror
  Software GPU / FPGA branch
```

它也明確提醒:

```text
不要為了讓 class 名字像 engine 而抽象.
```

## 5.4 `foundations/`

`docs/foundations/rendering_conventions.md` 和 `docs/foundations/rasterization_edge_rules.md` 是接下來最重要的實作規約.

它們已經定義:

```text
coordinate spaces
Vulkan-inspired post-projection convention
screen-space convention
pixel center
depth convention
triangle winding policy
edge-function rule
top-left rule
degenerate triangle policy
minimum tests
```

如果下一步做 raster, 應該照這兩份文件實作, 而不是重新發明一套隱性規則.

## 5.5 `tutorial-soft-renderer/`

`tutorial-soft-renderer` 是 graphics learning track.

對現在最直接的是:

```text
ch09 Edge Function
ch10 Barycentric
ch11 Bounding Box
ch12 Z-buffer
ch20 Viewport
ch21 MVP Pipeline
ch25 IShader
ch26 Perspective-correct Interpolation
```

它給出的 implementation order 是:

```text
screen-space raster correctness
  -> viewport
  -> MVP
  -> IShader
  -> perspective-correct interpolation
```

所以它支持:

```text
先做 trusted screen-space triangle,
不要直接跳 IShader.
```

## 5.6 `tutorial-cpp/`

`tutorial-cpp` 是 C++ / engineering track.

對現在最直接的是:

```text
i08_framebuffer_ownership_refactor
i09_display_backend_interface
i10_win32_display_backend
i11_sdl_display_backend
i13_renderer_frontend_backend_boundary
i14_immediate_mode_ui_integration_boundary
i15_shader_pipeline_interface_boundary
i16_verification_golden_image_regression_testing
```

它支持另一條合理路線:

```text
先做 owned Framebuffer / RenderTarget,
再做 DisplayBackend 和 headless verification.
```

所以現況不是只有 `render/raster-baseline` 一條路.

更準確地說, 現在有兩條正當 next branch:

```text
graphics-first:
  render/raster-baseline

architecture-first:
  arch/render-target
```

## 5.7 README 目前比較舊

`README.md` 還保留了較舊的大型 TODO, 包含:

```text
Reverse-Z
IShader
CommandQueue
UI
OBJ
Texture
Shadow
Normal Map
multi-threading
full UI system
```

這些不是錯, 但層級混在一起.

短期可以之後做:

```text
docs/readme-current-state
```

把 README 簡化成:

```text
project purpose
current source status
build/run
docs entry points
near-term milestone
```

但這不應該比 source 的第一個 trusted pipeline 更優先.

---

# 6. 3 月以來的脈絡分層

## 6.1 3 月: prototype + graphics tutorial

這段主要建立:

```text
Win32 prototype
DrawLine
DrawTriangle
barycentric triangle fill
MVP documentation
OpenGL pipeline / IShader / perspective-correct interpolation docs
```

價值:

```text
把 Pixel-Renderer 的 graphics pipeline 從 SetPixel 打開到 MVP / IShader.
```

限制:

```text
source 還沒有 trusted raster convention, depth, tests, debug views.
```

## 6.2 4 月上旬: architecture explosion

這段打開很多大方向:

```text
RenderDevice / Rasterizer decoupling
CommandQueue
frontend/backend split
UI overlay
browser / compositor analogy
FPGA hardware GPU
software renderer as golden model
```

價值:

```text
它正確看見 Pixel-Renderer 不只是 rasterizer.
```

限制:

```text
當時 source 還沒有足夠穩定的 shared trunk.
```

所以 4 月 notes 現在應該當成:

```text
branch vocabulary and future map
```

不是:

```text
immediate implementation order
```

## 6.3 5 月中: engineering boundary

這段收斂出非常重要的架構原則:

```text
Framebuffer owns memory
RenderDevice writes Framebuffer
DisplayBackend presents Framebuffer
```

這修正了一個容易走歪的方向:

```text
不要讓 SDL_Texture, Win32 DIBSection, macOS surface 成為 renderer core 的 primary target.
```

它也說明:

```text
SDL 是 platform / present backend,
不是 renderer core,
也不是取代 SetPixel / DrawLine / DrawTriangle 的東西.
```

## 6.4 5 月 22 日之後: stable docs 收斂

5/22 之後幾篇 notes 和 stable docs 把方向收斂成:

```text
先建立 trusted raster pipeline
再做 viewport / MVP / shader / renderer skeleton
再讓 UI / software GPU / FPGA / engine mirror 分支長出來
```

這是目前最應該遵守的優先序.

它不否定早期的 mini-Filament idea.

它只是把 mini-Filament skeleton 放到更合理的位置:

```text
medium-term trunk stage,
not immediate next branch.
```

---

# 7. 中央矛盾

Pixel-Renderer 現在的矛盾是:

```text
你想通透理解渲染,
所以不能只堆 engine class.

你也想理解商業引擎,
所以不能永遠停在 DrawTriangle function.

你想保留 UI, software GPU, FPGA, engine mirror 等分支,
所以不能把專案鎖死成單一小 rasterizer.
```

如果只做底層:

```text
SetPixel
DrawLine
DrawTriangle
Z-buffer
```

風險是:

```text
會和 Unity / Unreal / Filament / bgfx 的 renderer abstraction 斷開.
```

如果太早做高層:

```text
Engine
Renderer
Scene
View
Camera
Material
CommandQueue
```

風險是:

```text
class name 像 engine,
但 triangle, depth, viewport, interpolation 都不可信.
```

如果太早做分支:

```text
UI framework
SwiftShader-like VM
FPGA GPU
SDL backend
Unity mirror demo
```

風險是:

```text
共同主幹還沒出來,
每個分支都會各自長歪.
```

所以比較好的解法是:

```text
先定義共同主幹,
再讓 branch 最小切片自然長出來.
```

---

# 8. 共同主幹是什麼

目前共同主幹不是:

```text
完整 engine skeleton
```

也不是:

```text
完整 platform abstraction
```

目前共同主幹應該是:

```text
trusted screen-space raster pipeline
  + explicit rendering conventions
  + early tests
  + debug visibility
  + ownership direction that does not block future backends
```

它要先回答:

```text
一個 triangle 到底覆蓋哪些 pixels?
sample point 在哪裡?
shared edge 誰擁有?
depth 如何比較?
color 如何插值?
這些結果怎麼測?
錯了怎麼看?
```

只要這層可信, 後面才有意義:

```text
viewport:
  NDC 如何變成 screen triangle?

MVP:
  3D point 如何變成 NDC?

IShader:
  stage data 如何被 rasterizer 插值?

Material:
  shader behavior 和 per-object parameters 如何分離?

Renderer:
  scene and view 如何變成 draw intent?

CommandQueue:
  draw intent 如何變成 backend commands?

Engine Mirror:
  這些概念在 Unity / Unreal / Filament 怎麼對應?

FPGA:
  software golden model 的 edge/depth rules 是什麼?
```

---

# 9. 下一步選擇 A: `render/raster-baseline`

## 9.1 這條路線要解什麼

`render/raster-baseline` 要解的是:

```text
讓 screen-space triangle pipeline 變成可信小島.
```

最小 scope:

```text
ScreenVertex
DrawTriangleScreenSpace
edge-function coverage
pixel-center sampling
half-open bbox
top-left rule
degenerate triangle rejection
barycentric weights
color interpolation
depth interpolation
depth buffer
depth test
small tests
Win32 visual demo
```

## 9.2 為什麼它很適合現在做

因為它正好補 current source 和 stable docs 之間最大的 gap:

```text
docs 已經定義 raster convention,
source 還沒有實作.
```

它也支撐最多長期分支:

```text
MVP:
  needs trusted screen-space raster after viewport

IShader:
  needs interpolation and fragment input

Material:
  needs depth, raster state, fragment stage

Debug UI:
  needs render modes and inspectable buffers

FPGA:
  needs deterministic golden model

SwiftShader / software GPU:
  needs pipeline state and raster behavior later

Engine Mirror:
  needs a concept that has already been implemented locally
```

## 9.3 優點

```text
最直接回到 graphics pipeline 本體
最貼近 5/22 stable docs
能讓 source 從 prototype 進入可信 core
後面每一層都能接在它上面
```

## 9.4 風險

```text
如果還不抽 owned Framebuffer,
depth buffer 和 tests 可能暫時掛在 RenderDevice 附近,
架構不會最漂亮.
```

但這個風險可以控制:

```text
depth owner 不要放進 ScreenManager.
tests 先測 pure helper.
如果 current DIB ownership 真的卡住, 再拆 arch/render-target.
```

## 9.5 不該混進這個 branch 的東西

```text
SDL
DisplayBackend hierarchy
Mat4
MVP
IShader
Material
Scene
View
Camera
CommandQueue
full UI framework
OBJ loader
Texture system
Phong
Shadow mapping
```

如果混進來, 這條 branch 就會失去 "trusted first slice" 的價值.

---

# 10. 下一步選擇 B: `arch/render-target`

## 10.1 這條路線要解什麼

`arch/render-target` 要解的是:

```text
讓 renderer output memory 不再概念上屬於 ScreenManager.
```

目標形狀:

```text
Framebuffer / RenderTarget owns color memory
RenderDevice writes Framebuffer / RenderTarget
ScreenManager or Win32 present path copies/presents it
future DisplayBackend presents Framebuffer
```

## 10.2 為什麼它也是正當路線

因為 current source 現在是:

```text
ScreenManager owns DIBSection memory
RenderDevice borrows raw pointer
Rasterizer writes through RenderDevice
```

這短期可以跑, 但會讓這些事情變卡:

```text
depth buffer ownership
headless tests
golden image output
PPM output
SDL backend
macOS backend
renderer core isolation
```

所以 `arch/render-target` 不是 overengineering.

它解的是實際 ownership pressure.

## 10.3 優點

```text
讓 depth buffer 有自然 owner
讓 headless test 比較自然
讓 SDL/macOS 不會污染 renderer core
讓 DisplayBackend boundary 未來比較乾淨
```

## 10.4 風險

```text
會碰 Win32 resource lifecycle
會延後 graphics pipeline 的可見進展
scope 容易滑向完整 DisplayBackend / SDL / build rewrite
```

## 10.5 這條 branch 的邊界

如果選 `arch/render-target`, scope 應該是:

```text
Framebuffer owns color memory
RenderDevice writes Framebuffer
Win32 path still presents correctly
depth owner policy documented
no SDL yet
no full backend hierarchy yet
no CMake rewrite
no renderer skeleton
```

完成標準:

```text
old demo still renders
renderer memory conceptually belongs to renderer side
future depth buffer has a natural home
future headless output has a natural path
```

---

# 11. 下一步選擇 C: `test/raster-core`

## 11.1 這條路線要解什麼

`test/raster-core` 要解的是:

```text
先建立 correctness guardrail,
再接回 visual demo.
```

可測核心:

```text
edge(a, b, p)
computeBoundingBox(...)
classifyTopLeftEdge(...)
insideTriangle(...)
computeBarycentric(...)
interpolateDepth(...)
depthTest(...)
```

## 11.2 優點

```text
風險最低
不需要先大改 Win32 path
可以很早固定 convention
能避免只靠看畫面判斷正確性
```

## 11.3 缺點

```text
短期畫面進展較慢
如果不接回 Rasterizer, milestone 感不強
```

## 11.4 我對這條的看法

這條不一定要獨立成 branch.

更好的方式是:

```text
render/raster-baseline branch 裡面採用 test/raster-core 的方法.
```

也就是:

```text
先寫可測 helper,
再接 DrawTriangleScreenSpace,
最後接 Win32 demo.
```

---

# 12. 下一步選擇 D: docs cleanup

## 12.1 可以做什麼

可以開:

```text
docs/readme-current-state
docs/roadmap-next
docs/verification-strategy
docs/display-backend-architecture
```

整理:

```text
README 太大的 TODO
current status
near-term branch sequence
raster tests
debug view strategy
Framebuffer / DisplayBackend boundary
```

## 12.2 優點

```text
讓文件入口更乾淨
避免 README 繼續混合 long-term dreams 和 immediate steps
把 current note 的結論抽成 stable docs
```

## 12.3 缺點

```text
repo 已經有很多 docs
如果繼續只整理 docs, source 仍停在 prototype
```

## 12.4 建議

現在可以先不要再開大型 docs-only branch.

比較好的順序:

```text
1. 用這篇 note 做決策
2. 選下一個 source branch
3. source branch 做到一個穩定結論
4. 再把 durable decision 抽到 stable docs / ADR
```

---

# 13. 我目前的建議

如果下一步要開始 coding, 我建議:

```text
branch:
  render/raster-baseline
```

但實作方法採用:

```text
test/raster-core 的精神
```

也就是:

```text
pure raster helpers
  -> tests
  -> DrawTriangleScreenSpace
  -> depth buffer and depth test
  -> color interpolation
  -> Win32 demo
  -> first debug views
```

如果中途發現 current `ScreenManager` ownership 讓 depth/test/output 太卡, 再拆:

```text
arch/render-target
```

不要把這些全部塞進同一條 branch:

```text
edge function
depth buffer
owned Framebuffer
DisplayBackend
SDL
Mat4
IShader
CommandQueue
UI
```

簡短說:

```text
graphics-first outcome,
engineering-aware implementation.
```

---

# 14. Windows-first 與 cross-platform

## 14.1 現況

source 目前是 Windows-first:

```text
windows.h
RegisterClass
CreateWindow
QueryPerformanceCounter
CreateDIBSection
BitBlt
VK_ESCAPE
g++.exe
gdi32
user32
```

這符合專案主要在 Windows 開發的現況.

## 14.2 短期建議

短期:

```text
維持 Win32 path.
不要現在把 display path 換成 SDL.
```

理由:

```text
SDL 會牽動 platform, input, timer, build, texture update.
如果 Framebuffer ownership 還沒定清楚,
SDL_Texture 很容易被誤當成 renderer primary target.
```

## 14.3 正確 cross-platform path

正確順序:

```text
1. Renderer owns Framebuffer / RenderTarget
2. RenderDevice writes Framebuffer
3. Win32DisplayBackend presents Framebuffer
4. HeadlessTestBackend outputs image
5. SDLDisplayBackend presents Framebuffer
6. macOSDisplayBackend presents Framebuffer
```

錯誤順序:

```text
1. 導入 SDL
2. 讓 renderer 寫 SDL_Texture
3. 再試圖抽象平台
```

核心規則:

```text
renderer core decides pixels.
DisplayBackend shows pixels.
```

---

# 15. Framebuffer ownership

## 15.1 Current model

現在是:

```text
ScreenManager owns DIBSection memory
RenderDevice borrows raw pointer
Rasterizer writes through RenderDevice
```

這比把 Win32 API 放進 `Rasterizer` 好很多.

但它還不是 target model.

問題:

```text
renderer memory lifetime tied to Win32
depth buffer owner unclear
offscreen tests awkward
screenshot / golden image not first-class
SDL/macOS may repeat ownership confusion
```

## 15.2 Target model

目標:

```text
Framebuffer / RenderTarget owns color and depth memory
RenderDevice writes Framebuffer
DisplayBackend presents Framebuffer
```

資料流:

```text
Rasterizer
  -> RenderDevice
  -> Framebuffer(color + depth)
  -> DisplayBackend::Present(framebuffer)
  -> Win32 / SDL / Headless
```

## 15.3 什麼時候做

有兩個合理時間點:

```text
Option 1:
  先做 arch/render-target,
  再做 render/raster-baseline.

Option 2:
  先做 render/raster-baseline,
  若 depth/test/output 卡住,
  再拆 arch/render-target.
```

我目前偏 Option 2.

原因:

```text
現在最缺的是可信 raster behavior.
Ownership 很重要, 但可以在 raster branch 中避免把 depth 放進 ScreenManager.
如果真的卡住, 再抽出小 branch 解 ownership.
```

---

# 16. Raster rules

## 16.1 不建議延用 current `DrawTriangle` 當主線

current `DrawTriangle` 可以保留成:

```text
old educational prototype
```

但下一版應該新增或重構成:

```cpp
DrawTriangleScreenSpace(v0, v1, v2)
```

並遵守 stable convention.

## 16.2 Target raster convention

應採用:

```text
Screen Space:
  origin top-left
  x right
  y down

Sample:
  pixel center = (x + 0.5, y + 0.5)

Bounding box:
  half-open ranges
  [x0, x1), [y0, y1)

Coverage:
  edge functions
  deterministic top-left rule

Culling:
  disabled in screen-space baseline
  accept CW and CCW

Depth:
  [0, 1]
  smaller is closer
  clear = 1.0
  pass if incoming < stored

Color:
  start with Color32 or simple packed color
  interpolate vertex colors in screen space
```

這不是風格問題.

這是在避免:

```text
shared edge crack
double draw
off-by-one
winding confusion
viewport debug confusion
depth convention mismatch
```

---

# 17. Testing 與 debug view

## 17.1 不要只靠看畫面

graphics code 很常出現:

```text
畫面看起來差不多,
但 convention 其實錯了.
```

所以第一個 raster milestone 就應該有 tests.

## 17.2 第一批 tests

最小測試:

```text
edge function sign
bbox half-open range
pixel-center sampling
clockwise triangle
counter-clockwise triangle
degenerate triangle
shared-edge top-left ownership
barycentric sum approximately 1
depth clear value
depth less-than pass
depth fail
color interpolation
```

其中最重要的是:

```text
shared-edge top-left ownership
```

因為它會逼迫 implementation 真正處理 triangle edge ownership.

## 17.3 第一批 debug views

優先:

```text
Barycentric View
Depth View
Wireframe Overlay
```

用途:

```text
Barycentric View:
  看 coverage, winding, interpolation 是否合理.

Depth View:
  看 depth range, depth interpolation, depth test 是否合理.

Wireframe Overlay:
  看 triangle vertex position, edge, fill 是否對齊.
```

之後再做:

```text
UV
Normal
Overdraw
ClipSpaceW
TriangleID
FrontBackFace
```

## 17.4 Golden image 何時做

不要第一步就做大型 golden image system.

合理順序:

```text
1. unit tests
2. deterministic demo
3. PPM output helper
4. golden comparison
5. diff image output
```

如果 raster rules 還沒穩, 太早 golden image 只會一直更新 expected image.

---

# 18. Math / MVP 的選擇

## 18.1 不要現在直接做完整 MVP

現在不要直接做:

```text
Vec4
Mat4
LookAt
Perspective
IShader
OBJ
Texture
```

原因不是它們不重要.

原因是錯誤來源太多.

如果 screen-space triangle 不可信, MVP 畫錯時你不知道錯在:

```text
matrix convention
handedness
projection
viewport y flip
depth range
rasterizer coverage
top-left rule
```

哪一層.

## 18.2 合理順序

等 raster baseline 後:

```text
Milestone B:
  NDC / Viewport

Milestone C:
  Vec4 / Mat4 / LookAt / Orthographic

Milestone D:
  Perspective / clip.w / perspective divide
```

這樣能把問題拆開:

```text
先測 NDC -> Screen
再測 Math -> NDC
最後測 Perspective w
```

---

# 19. IShader / Material / Renderer skeleton

## 19.1 IShader 很重要, 但不是 immediate step

`IShader` 需要前提:

```text
vertex output can produce screen-space triangle
rasterizer can interpolate varyings
fragment input has a clear boundary
depth/color write is trustworthy
```

如果現在直接做 `IShader`, 容易變成:

```text
virtual function shell
```

而不是 programmable pipeline boundary.

## 19.2 Material / MaterialInstance 也應該延後

Material 的意義是:

```text
shared rendering behavior
shader
raster state
parameter layout
```

MaterialInstance 的意義是:

```text
per-object parameter values
```

但在沒有:

```text
IShader
FragmentInput
depth/raster state
interpolation
Renderer rendering a View
```

之前, `Material` 很容易只是命名漂亮.

## 19.3 Renderer / Scene / View / Camera

這些很重要, 但應該在:

```text
raster baseline done
viewport / MVP done
IShader stage data done
```

之後.

原因:

```text
Renderer::render(view)
```

應該包住真實 pipeline, 而不是包住不可信的 demo calls.

---

# 20. CommandQueue

## 20.1 為什麼不要現在做

如果現在做 `CommandQueue`, 它很可能只會變成:

```text
ClearCmd
DrawLineCmd
DrawTriangleCmd
```

這不是 renderer frontend/backend split.

這只是把 current immediate calls 包成資料.

真正的 CommandQueue 需要比較完整的 draw intent:

```text
DrawMeshCmd
MaterialInstance
PipelineState or ShaderContext
RenderTarget
Viewport
resource handles or snapshots
```

## 20.2 CommandQueue 未來要保留的核心 rule

未來重要規則:

```text
RenderCommand stores data snapshots or stable handles.
Do not store dangling pointers across deferred execution.
```

這會關係到:

```text
render thread
frame capture
debug replay
software GPU path
FPGA command contract
```

但現在還不急.

---

# 21. UI / Debug UI

## 21.1 UI 是重要分支

UI 不是旁支, 因為它連到:

```text
input
state
layout
hit testing
clip rect
draw command
paint
composite
debug tooling
browser rendering
engine editor tools
```

但完整 UI framework 現在太早.

## 21.2 第一版 UI 應該是 debug tooling

第一版 UI 的目的應該是:

```text
debug renderer
```

不是:

```text
完整 immediate-mode UI toolkit
browser-like layout engine
retained UI system
```

等 renderer 有 debug modes 後, 再做:

```text
RenderMode selector
Depth test toggle
Wireframe toggle
Perspective-correct toggle
Light direction slider
Camera controls
RenderStats display
```

## 21.3 UI branch 的正確接法

未來 flow:

```text
scene render first
ui overlay after
UI produces draw commands
UI renderer writes Framebuffer
DisplayBackend only presents
```

不要讓 UI widget 直接亂操作 platform surface.

---

# 22. Software GPU / SwiftShader

## 22.1 現在的定位

SwiftShader 現在應該是:

```text
conceptual reference
```

不是:

```text
source deep dive target
```

因為 SwiftShader 是:

```text
Vulkan implementation
pipeline state
SPIR-V
shader execution
JIT / Reactor
SIMD
routine specialization
Vulkan conformance
```

Pixel-Renderer 目前還沒有:

```text
IShader
Material
PipelineState
CommandQueue
Texture
Fragment shader
```

所以現在直接 deep dive SwiftShader source 會太早.

## 22.2 中間橋

未來如果要往 software GPU, 先做:

```text
Toy ShaderVM
```

不要直接碰 SPIR-V.

例子:

```text
LOAD_ATTR position
MUL_MAT4 MVP
STORE_POSITION

LOAD_ATTR normal
DOT lightDir
MAX 0
MUL_COLOR baseColor
STORE_COLOR
```

但它應該在:

```text
IShader
Material
PipelineState
CommandQueue
```

之後.

---

# 23. FPGA / hardware GPU

## 23.1 長期重要, 但不是 next task

FPGA 方向很重要, 因為它對應:

```text
hardware data path
fixed-point math
pipeline stage
FIFO
latency / throughput
golden model verification
```

但現在不是直接做完整 GPU.

## 23.2 software renderer 和 FPGA 的正確關係

正確關係:

```text
Pixel-Renderer software model
  defines deterministic raster/depth rules

FPGA hardware model
  implements a subset in RTL

verification
  compares traces, depth, coverage, framebuffer
```

不是:

```text
把 C++ for loop 直接搬到 FPGA.
```

## 23.3 第一個合理 hardware slice

第一個合理 slice:

```text
edge-function triangle rasterizer
```

輸入:

```text
triangle vertices
pixel coordinate or scan range
```

輸出:

```text
inside / outside
barycentric weights
depth candidate
```

這正好反過來支持現在的優先序:

```text
先把 software raster baseline 做 deterministic.
```

---

# 24. Engine Mirror Demo

## 24.1 為什麼需要 Engine Mirror

你不只想自己寫 renderer, 也想未來能在 Unity / Unreal / 自研引擎中有效工作.

所以長期策略應該是:

```text
Pixel-Renderer first-principles implementation
  +
Unity / Unreal / Filament / Godot mapping
```

也就是:

```text
同題雙解.
```

## 24.2 但不要現在做大型 mirror demo

如果 Pixel-Renderer 裡的概念還沒做出來, mirror demo 會變成:

```text
只是在商業引擎裡操作功能,
沒有 first-principles 對照.
```

## 24.3 第一批適合 mirror 的題目

等 Pixel-Renderer 做到對應功能後:

```text
Viewport / Camera:
  Unity / Unreal camera projection mapping

Material / Shader:
  Phong / Blinn-Phong + runtime parameters

Debug UI:
  runtime debug panel

CommandQueue:
  bgfx / Unreal RHI / Filament backend conceptual mapping
```

最自然的第一個 mirror 可能是:

```text
Phong / Blinn-Phong material + runtime debug panel
```

但它應該在 Pixel-Renderer 已經有:

```text
MVP
IShader
normal interpolation
fragment shading
```

之後.

---

# 25. Docs extraction queue

這篇目前仍是 `docs/notes/`, 因為它是決策分析.

如果後續要抽成 stable docs, 順序可以是:

```text
docs/roadmap/next_steps.md
  近期 branch sequence

docs/verification/testing_strategy.md
  raster tests, debug view, golden image 分層

docs/architecture/display_backend_architecture.md
  Framebuffer / RenderDevice / DisplayBackend ownership

docs/foundations/pipeline_flow.md
  screen-space raster -> viewport -> MVP -> IShader

docs/foundations/math_to_renderer.md
  linear algebra concepts -> renderer tasks
```

不要在還沒選 branch 前一次補完全部 stable docs.

比較好的做法:

```text
notes 先服務決策.
stable docs 只收 durable decision.
ADR 只記會約束 future branch 的決策.
```

---

# 26. ADR candidates

這些未來可能值得寫 ADR:

```text
owned Framebuffer / RenderTarget
DisplayBackend present-only boundary
depth convention
pixel-center sampling and top-left rule
screen-space triangle before MVP
traditional depth before Reverse-Z
Material vs MaterialInstance
CommandQueue after Renderer skeleton
raw pointer vs handle vs snapshot for commands
```

不要為每個小想法寫 ADR.

ADR 應該只記:

```text
會影響 future branch 的 durable decision.
```

---

# 27. 決策表

| 路線 | 現在可做性 | 解的問題 | 主要風險 | 建議 |
|---|---:|---|---|---|
| `render/raster-baseline` | 高 | raster correctness | ownership 可能不漂亮 | default next branch |
| `test/raster-core` | 高 | correctness guardrail | 畫面進展慢 | 併入 raster branch |
| `arch/render-target` | 高 | framebuffer ownership | 會碰 Win32 lifecycle | 若 depth/test 卡住就拆 |
| `docs/readme-current-state` | 中 | README/TODO 混亂 | 繼續延後 source | branch 選定後 |
| `docs/roadmap-next` | 中 | 決策沉澱 | 可能變 docs churn | branch 選定後 |
| `arch/display-backend` | 中低 | present abstraction | 現在太廣 | owned Framebuffer 後 |
| `render/viewport-ndc` | 中低 | post-projection mapping | 依賴 raster baseline | raster 後 |
| `render/math-camera` | 中低 | MVP foundation | 錯誤來源多 | viewport 後 |
| `render/shader-stage` | 低 | programmable pipeline | 依賴 interpolation/depth | later |
| `arch/renderer-skeleton` | 低 | engine abstraction | 容易空殼 | IShader 後 |
| `arch/command-queue` | 低 | frontend/backend split | 現在只是 drawing queue | Renderer 後 |
| `debug/ui-overlay` | 低 | runtime controls | 需要 debug modes | debug views 後 |
| `exp/fpga-raster-model` | 低 | hardware golden model | 需要 deterministic raster | raster baseline 後 |
| `exp/software-gpu-shadervm` | 低 | shader execution model | 需要 IShader/PipelineState | later |

---

# 28. 建議的近期 branch sequence

我目前建議:

```text
1. render/raster-baseline
2. debug/raster-views or include first views in raster branch
3. arch/render-target
4. render/viewport-ndc
5. render/math-camera
6. render/perspective
7. render/shader-stage
8. arch/renderer-skeleton
9. arch/command-queue
10. debug/ui-overlay
```

如果你想更保守:

```text
1. test/raster-core
2. render/raster-baseline
3. arch/render-target
```

如果你想先解 architecture:

```text
1. arch/render-target
2. render/raster-baseline
3. test/raster-core
```

我個人仍偏第一條.

原因:

```text
現在最缺的是可信 pipeline,
而不是更多架構外殼.
```

---

# 29. `render/raster-baseline` 的 done condition

如果下一步選 `render/raster-baseline`, 建議完成標準:

Required:

```text
ScreenVertex exists
DrawTriangleScreenSpace exists
edge-function coverage works
pixel-center sampling works
half-open bbox works
degenerate triangle rejected
barycentric weights computed
color interpolation works
depth buffer exists
depth test uses smaller-is-closer
at least edge/bbox/barycentric/depth tests pass
one demo renders RGB screen-space triangle
one demo renders depth overlap
existing Win32 demo path still works
```

Strongly recommended:

```text
shared-edge top-left test
Barycentric View
Depth View
Wireframe Overlay
```

Explicitly out of scope:

```text
SDL
DisplayBackend hierarchy
Mat4
MVP
IShader
Material
Renderer
Scene
View
Camera
CommandQueue
UI framework
OBJ
Texture
Phong
Shadow
```

如果 branch 開始碰這些東西, 就該停下來拆 branch.

---

# 30. 當前 local worktree note

本篇撰寫時 local worktree 有這些 docs/rules 相關變更:

```text
M  .gitattributes
 M AGENTS.md
 M docs/HANDOFF.md
?? docs/AGENTS.md
?? docs/notes/2026-06-28-current_status_and_decision_map.md
```

解讀:

```text
.gitattributes:
  line-ending and repo text policy work

AGENTS.md, docs/AGENTS.md, docs/HANDOFF.md:
  agent and handoff role updates

this note:
  current status and decision analysis
```

這些不是 renderer source changes.

---

# 31. 最短結論

Pixel-Renderer 現在有:

```text
可 build 的 early Win32 prototype
大量且有價值的 docs / notes
很清楚的 Rendering Systems Lab 定位
長期分支地圖
```

但它還沒有:

```text
trusted raster core
depth buffer
tests
debug views
owned framebuffer boundary
renderer skeleton
```

目前最重要的不是再選最終身份.

目前最重要的是:

```text
建立共同主幹的第一個可信小島.
```

我建議這個可信小島是:

```text
Screen-space Triangle Pipeline
```

也就是下一步優先:

```text
render/raster-baseline
```

這條路最符合那兩句核心目標:

```text
通透理解渲染
擁有自己的 renderer
能自由擴充
未來能理解 Unity / Unreal / 自研引擎
以 Rendering Systems Lab 方式建立共同主幹
再讓分支自然長出最小切片
```

如果這一步做對, Pixel-Renderer 會從:

```text
有 Win32 window, line, triangle 的 prototype
```

進入:

```text
有可信 raster foundation 的 Rendering Systems Lab
```

這會讓後面的:

```text
MVP
IShader
Material
Renderer
CommandQueue
Debug UI
Engine Mirror
Software GPU
FPGA golden model
```

都有共同地基.
