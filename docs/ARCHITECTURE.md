# Pixel-Renderer Technical Target Architecture

日期：2026-05-22

這份文件描述 Pixel-Renderer 的技術目標架構：短期、中期、長期各自要長出的 rendering architecture、核心技術、工程邊界與驗證方式。

它的角色不是教學章節，也不是 git workflow。分工如下：

```text
README.md
  對外入口：專案是什麼、怎麼 build、目前狀態、從哪裡開始讀。

docs/PROJECT_MAP.md
  知識地圖：Rendering Systems 的學習脈絡、roadmap、engine mapping、職涯方向。

docs/ARCHITECTURE.md
  技術目標架構：短中長期 architecture、核心 module、資料流、工程邊界。

docs/architecture/
  細部設計：display backend、renderer core、command queue、resource lifetime、debug UI。
```

核心原則：

```text
不要為了讓 class 名字像 engine 而抽象。
每一層 architecture 都必須對應到真實技術壓力：
correctness, ownership, data flow, backend separation, debugging, testing, or engine mapping.
```

---

## 1. Architecture North Star

Pixel-Renderer 的長期定位不是單純的 software rasterizer，也不是一開始就做完整 game engine。

比較準確的技術目標是：

```text
A first-principles software rendering stack
that can grow into a small rendering engine skeleton,
then branch into engine fluency, software GPU, and FPGA golden-model directions.
```

整體技術主線：

```text
SetPixel / Framebuffer
  -> line rasterization
  -> triangle rasterization
  -> barycentric / edge function
  -> depth buffer
  -> viewport / MVP
  -> programmable shader boundary
  -> material / scene / view / camera
  -> command queue / backend split
  -> debug UI / tooling
  -> engine mirror mapping
  -> optional software GPU / FPGA branches
```

這裡的重點不是「越快做出更多功能越好」，而是每個階段都要產生一個可信技術層：

```text
can explain
can test
can debug
can refactor
can map to commercial engine concepts
```

---

## 2. Current Snapshot

目前程式是一個 Win32 software renderer prototype。

```text
main.cpp
  |
  v
Application
  |
  +-- ScreenManager
  |     owns Win32 window, input, DIBSection framebuffer, BitBlt present
  |
  +-- RenderDevice
  |     borrows framebuffer pointer, Clear(), SetPixel()
  |
  +-- Rasterizer
        DrawLine(), DrawTriangle()
```

目前資料流：

```text
demo code in main.cpp
  -> Rasterizer
  -> RenderDevice::SetPixel
  -> ScreenManager-owned DIBSection memory
  -> ScreenManager::UpdateScreen
  -> Win32 BitBlt
  -> OS compositor / display
```

目前這個形態適合快速看到畫面，但有幾個架構壓力：

```text
1. framebuffer memory is owned by display/window layer
2. renderer core is tied to Win32 presentation details
3. demo code directly calls rasterizer
4. draw result is visible, but correctness is not yet strongly tested
5. debug visualization is not first-class
```

這些壓力決定了後續 architecture 的演化方向。

---

## 3. Target Stack

長期目標可以拆成幾層：

```text
Application / Platform Layer
  owns process lifecycle, window loop, input pump, frame timing

DisplayBackend Layer
  presents framebuffer to Win32 / SDL / headless test target

Renderer Frontend
  interprets scene, view, camera, material, renderable objects

Command Layer
  records rendering intent as immutable command data

Software Backend
  consumes command queue and dispatches to rasterizer / render device

Raster Core
  computes primitive coverage, barycentric weights, depth, interpolation

RenderDevice / Framebuffer
  owns pixel writes, depth writes, clear, blend, clip, buffer storage

Debug / Verification Layer
  tests, traces, golden images, debug visualization, runtime panels
```

目標資料流：

```text
Application
  -> Engine / Renderer
  -> View + Camera + Scene
  -> Mesh + MaterialInstance
  -> RenderCommand stream
  -> SoftwareBackend
  -> Rasterizer
  -> RenderDevice
  -> Framebuffer
  -> DisplayBackend
  -> OS window / headless output
```

這不是一次完成，而是分階段長出來。

---

## 4. Short-Term Architecture

短期目標：

```text
建立可信的 screen-space raster core。
```

這一階段的目的不是 engine abstraction，而是讓最底層 rasterization 可以被信任。

### 4.1 Scope

短期先處理：

```text
DDA line reference
Bresenham line rasterizer
ScreenVertex
DrawTriangleScreenSpace
Bounding Box
Edge Function
Barycentric Coordinates
Depth Buffer
Depth Test
Color Interpolation
Barycentric Debug View
Depth Debug View
Wireframe Overlay
small deterministic tests
```

短期暫時不碰：

```text
MVP
LookAt
Perspective
IShader
Material
Scene
Camera
CommandQueue
Texture
Phong
OBJ loader
Shadow mapping
```

原因很直接：這些都會增加錯誤來源。現在要先確認 triangle / depth / interpolation 本身可信。

### 4.2 Short-Term Module Shape

短期可接受的架構形態：

```text
Application
  -> Rasterizer
  -> RenderDevice
  -> current framebuffer memory
  -> ScreenManager present
```

但要開始把概念名稱穩定下來：

```text
Rasterizer
  DrawLineDDAReference()
  DrawLineBresenham()
  DrawTriangleScreenSpace()

RenderDevice
  ClearColor()
  ClearDepth()
  SetPixel()
  SetPixelDepthTest()

Framebuffer
  planned owner of color/depth memory

DebugView
  planned runtime visualization mode
```

即使 `Framebuffer` 還沒有完全抽出，也要先在文件和 code comments 中明確記錄：

```text
current framebuffer owner: ScreenManager / DIBSection
target framebuffer owner: Framebuffer
```

### 4.3 Short-Term Done Criteria

短期完成標準：

```text
1. DDA and Bresenham behavior can be compared on known line cases
2. triangle bbox coverage is deterministic
3. barycentric weights are numerically explainable
4. depth overlap case renders correctly
5. color interpolation behaves as expected
6. barycentric / depth / wireframe views can expose internal state
7. tests cover bbox, barycentric, depth, basic line cases
```

短期 architecture 的核心成果不是漂亮 demo，而是：

```text
first trusted island:
screen-space triangle pipeline
```

---

## 5. Mid-Term Architecture

中期目標：

```text
把 renderer core 從 platform/display 中分離，
並建立 MVP / shader / material 前的穩定資料邊界。
```

### 5.1 Framebuffer Ownership

目前問題：

```text
RenderDevice writes pixels,
but pixel memory is owned by Win32 ScreenManager.
```

中期目標：

```text
Framebuffer owns pixels.
RenderDevice writes Framebuffer.
DisplayBackend presents Framebuffer.
```

目標形態：

```text
Framebuffer
  color buffer
  depth buffer
  width / height
  pixel format
  clear operations

RenderDevice
  writes to Framebuffer
  handles depth test, blending, clip rect

DisplayBackend
  presents Framebuffer
```

這個分離會打開三個後續能力：

```text
Win32DisplayBackend
SDLDisplayBackend
HeadlessTestBackend
```

### 5.2 Display Backend

中期 display abstraction 應該回答：

```text
given a Framebuffer,
how do I present it on this platform?
```

而 renderer core 應該回答：

```text
given geometry and render state,
which pixels should be written into the Framebuffer?
```

目標 interface 可以先很小：

```cpp
class IDisplayBackend {
public:
    virtual bool initialize(int width, int height, const char* title) = 0;
    virtual void present(const Framebuffer& framebuffer) = 0;
    virtual void pollEvents(InputState& input) = 0;
    virtual bool shouldClose() const = 0;
};
```

這不是最終 API，只是方向。重點是三個邊界：

```text
renderer core calculates pixels
Framebuffer stores pixels
DisplayBackend shows pixels
```

### 5.3 Math / Transform Boundary

中期接上：

```text
Vec2 / Vec3 / Vec4
Mat4
Viewport transform
Model / View / Projection
clip space
NDC
screen space
```

這一階段必須搭配 `rendering_conventions`：

```text
coordinate spaces
handedness
matrix convention
depth convention
screen origin
pixel center rule
winding rule
top-left rule
```

如果 conventions 沒有先定清楚，後面的 MVP bug 會很難判斷是數學錯、座標系錯，還是 rasterizer 錯。

### 5.4 Programmable Pipeline Boundary

中期的 shader 目標不是一開始做 Phong，而是先定清楚資料邊界：

```text
VertexInput
VertexOutput
FragmentInput
ShaderContext
IShader
```

最小 shader set：

```text
FlatColorShader
VertexColorShader
DepthDebugShader
UVDebugShader
```

這一階段的關鍵問題：

```text
vertex stage outputs what?
rasterizer interpolates what?
fragment stage receives what?
RenderDevice writes what?
```

### 5.5 Mid-Term Done Criteria

中期完成標準：

```text
1. Framebuffer owns color/depth memory
2. RenderDevice no longer depends on Win32-owned memory
3. Win32DisplayBackend presents Framebuffer
4. optional HeadlessTestBackend can run render tests without window
5. viewport transform tests pass
6. Mat4 / camera tests pass
7. IShader data boundary is implemented with simple debug shaders
8. traceVertex can show OS / WS / VS / CS / NDC / SS values
```

中期 architecture 的核心成果是：

```text
renderer core is platform-independent enough to test and evolve.
```

---

## 6. Long-Term Architecture

長期目標：

```text
在可信 renderer core 上建立最小 rendering engine skeleton，
並讓它能分支到 engine fluency、debug tooling、software GPU、FPGA golden model。
```

### 6.1 Renderer / Scene / View / Camera

長期 engine skeleton：

```text
Engine
  |
  v
Renderer
  |
  +--> View
  |     +--> Camera
  |     +--> Scene
  |
  v
Renderable
  +--> Mesh
  +--> Transform
  +--> MaterialInstance
          |
          v
        Material
          |
          v
        IShader / Shader Program
```

目標 API 方向：

```cpp
Scene scene;
Camera camera;
View view;
Renderer renderer;

view.setScene(&scene);
view.setCamera(&camera);

renderer.render(view);
```

這一層的意義不是模仿 Filament 的 class 名字，而是把真實問題分開：

```text
Scene: what exists?
View: how is it observed and rendered?
Camera: how do world coordinates become view/clip coordinates?
MaterialInstance: what per-object shader parameters are bound?
Renderer: how does scene data become render commands?
```

### 6.2 CommandQueue / Backend Split

長期 command architecture：

```text
Renderer Frontend
  records render intent

CommandQueue
  stores immutable command snapshots

SoftwareBackend
  consumes commands and calls Rasterizer / RenderDevice
```

核心 command：

```text
ClearCmd
DrawLineCmd
DrawTriangleCmd
DrawMeshCmd
SetViewportCmd
SetShaderCmd
SetMaterialParamsCmd
SetClipRectCmd
```

重要 rule：

```text
RenderCommand stores data snapshots, not dangling pointers.
```

這是未來 render thread、deferred execution、debug capture 的基礎。

### 6.3 Debug UI / Tooling

Debug UI 不是裝飾，而是 rendering system 的顯微鏡。

目標控制項：

```text
render mode: shaded / wireframe / barycentric / depth / normal / uv
depth test on/off
backface culling on/off
perspective-correct interpolation on/off
light direction
camera transform
draw call / triangle / pixel stats
selected stage trace
```

UI 技術方向：

```text
immediate-mode UI
UiContext
UiCommandList
ClipRect
DrawRect / DrawText / DrawSpan
scene_queue + ui_queue
```

### 6.4 Engine Mirror Demo

長期每個重要 topic 都應該有 mirror mapping：

```text
Pixel-Renderer implementation
  -> Unity / Unreal / Godot / Filament concept
  -> workflow / tooling
  -> performance tradeoff
  -> related job role
```

例子：

```text
Pixel-Renderer CommandQueue
  -> Unreal RHI command flow
  -> bgfx command submission
  -> Filament backend abstraction

Pixel-Renderer MaterialInstance
  -> Unity material parameters
  -> Unreal material instance
  -> technical artist workflow
```

### 6.5 Software GPU / FPGA Branches

這不是短期主線，但 architecture 要保留分支空間。

Software GPU 方向：

```text
ShaderVM
PipelineState
specialized routines
SIMD raster blocks
SwiftShader / llvmpipe comparison
```

FPGA / hardware GPU 方向：

```text
fixed-point edge function
tile rasterization
depth test hardware model
attribute interpolation hardware model
software golden model comparison
Verilog / SystemVerilog testbench
```

這些分支都依賴同一個前提：

```text
software renderer core must be deterministic and testable.
```

### 6.6 Long-Term Done Criteria

長期完成標準不是「做完所有功能」，而是專案具有可擴展架構：

```text
1. Renderer can render a View from Scene + Camera
2. MaterialInstance can bind shader parameters
3. CommandQueue can capture frame-level render intent
4. SoftwareBackend can execute commands deterministically
5. Debug UI can inspect pipeline state
6. Headless tests can compare golden outputs
7. selected topics have engine mirror notes
8. optional software GPU / FPGA experiments can reuse the renderer as golden model
```

---

## 7. Technical Roadmap Summary

用時間尺度整理：

```text
Short term:
  trusted screen-space raster core
  DDA / Bresenham / triangle / barycentric / depth / debug views / tests

Mid term:
  renderer core separation
  owned Framebuffer
  DisplayBackend
  headless tests
  viewport / MVP
  IShader boundary

Long term:
  Renderer / Scene / View / Camera
  Material / MaterialInstance
  CommandQueue / SoftwareBackend
  immediate-mode debug UI
  engine mirror demos
  software GPU / FPGA branches
```

用依賴順序整理：

```text
1. raster correctness
2. debug visibility
3. testability
4. framebuffer ownership
5. platform backend boundary
6. math / transform pipeline
7. shader stage boundary
8. renderer object model
9. command queue
10. tooling / mirror / hardware-oriented branches
```

---

## 8. README Migration Rule

README 最終應該保持短：

```text
project purpose
current status
build / run instructions
minimal example
docs entry links
```

技術架構內容放這裡：

```text
short / mid / long architecture
target module boundaries
rendering stack evolution
technical roadmap
```

細部設計放 `docs/architecture/`：

```text
docs/architecture/display_backend_architecture.md
docs/architecture/renderer_core_architecture.md
docs/architecture/resource_lifetime.md
docs/architecture/command_queue_architecture.md
docs/architecture/debug_ui_architecture.md
```

其中第一優先應該是：

```text
docs/architecture/display_backend_architecture.md
```

原因是 SDL / Win32 / headless test 的分離會影響後面所有架構：

```text
Framebuffer ownership
RenderDevice dependency
test runner
debug output
cross-platform development
future engine mirror demos
```
