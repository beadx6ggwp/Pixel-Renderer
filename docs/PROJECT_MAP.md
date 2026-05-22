# Pixel-Renderer Project Map

日期：2026-05-22

定位：整理 Pixel-Renderer 的知識脈絡、技術架構與未來規劃。這份文件不是取代既有教學，而是作為 `docs/` 的總入口。

相關入口：

```text
docs/ARCHITECTURE.md
  技術目標架構：短中長期 architecture、核心 module、資料流、display/backend 邊界。

docs/DEVELOPMENT.md
  輕量開發規範：branch、commit、experiment、remote branch、main history 管理。
```

---

## 1. 專案核心定位

Pixel-Renderer 目前不應只被理解成「寫一個 software rasterizer」。

更精準的定位是：

> 以純 C++ software renderer 為核心的 Rendering Systems Lab。

它從最底層的 `SetPixel()` 開始，逐步建立一條自己能解釋、能驗證、能擴充的 rendering stack：

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
  -> Optional Software GPU / FPGA branches
```

這條主線的價值不是只做出畫面，而是讓每個 rendering concept 都能同時回答：

```text
1. 底層機制是什麼？
2. 數學結構是什麼？
3. C++ 實作邊界在哪裡？
4. 如何 debug / test？
5. 在 Unity / Unreal / Filament / bgfx / SwiftShader 中對應到什麼抽象？
6. 對哪些職涯角色有訓練價值？
```

---

## 2. 知識脈絡

目前整個學習方向可以暫時命名為 `Rendering Systems`。

它包含多條互相交疊的分支：

```text
Rendering Systems
|
+-- Software Rasterizer
|   Pixel-Renderer / TinyRenderer / Scratchapixel
|
+-- Rendering Engine
|   Filament / OGRE / Godot renderer
|
+-- Graphics API Abstraction / RHI
|   bgfx / Diligent Engine / The Forge / Unreal RHI
|
+-- Software GPU / Driver
|   SwiftShader / Mesa llvmpipe / ShaderVM / pipeline specialization
|
+-- Hardware GPU / FPGA
|   raster unit / shader unit / fixed-point pipeline / golden model verification
|
+-- UI / Compositor System
|   immediate-mode UI / browser rendering / desktop compositor / dirty region
|
+-- Commercial Engine Fluency
    Unity / Unreal / Godot / self-developed engine workflows
```

短期不需要決定最終要走哪一條。比較好的策略是：

```text
先建立共同主幹，
再用小型實驗觀察哪個分支最值得深化。
```

---

## 3. Project Strategy: Pixel-Renderer + Engine Mirror

推薦長期採用「同題雙解」策略：

```text
A 線：Pixel-Renderer
  從 first principles 實作底層機制。

B 線：Engine Mirror Demo
  在 Unity / Unreal / Godot / Filament 等系統中重現或觀察同一概念。
```

每個主題都整理成 mapping note：

```text
Topic
  -> Pixel-Renderer implementation
  -> Engine abstraction
  -> Workflow / tooling
  -> Debug method
  -> Performance tradeoff
  -> Related job role
```

例子：

```text
Phong / Blinn-Phong Material
  Pixel-Renderer:
    vertex shader, fragment shader, normal interpolation, light equation

  Unity / Unreal:
    Shader Graph / Material Editor / custom shader / runtime debug panel

  Role mapping:
    graphics engineer, technical artist, game client engineer
```

```text
Render Command Queue
  Pixel-Renderer:
    CommandQueue, scene_queue, ui_queue, SoftwareBackend

  Engine mapping:
    bgfx command submission, Unreal RHI, Filament backend, Godot RenderingDevice

  Role mapping:
    rendering engineer, engine engineer, tools engineer
```

---

## 4. 目前程式碼架構

目前 `src/` 是一個 Windows / Win32 software renderer prototype。

```text
Application
  owns ScreenManager
  owns RenderDevice
  owns Rasterizer
  controls frame loop

ScreenManager
  owns Win32 window resources
  owns DIBSection framebuffer memory
  handles input events
  presents by BitBlt

RenderDevice
  borrows framebuffer pointer
  provides Clear()
  provides SetPixel()

Rasterizer
  borrows RenderDevice
  provides DrawLine()
  provides DrawTriangle()

main.cpp
  demo application
  calls device / rasterizer directly
```

目前資料流：

```text
main.cpp demo code
  -> Rasterizer::DrawLine / DrawTriangle
  -> RenderDevice::SetPixel
  -> ScreenManager-owned DIBSection memory
  -> ScreenManager::UpdateScreen
  -> Win32 BitBlt
  -> window / OS compositor / display
```

目前已經做到：

```text
1. Win32 window + DIB framebuffer
2. Application frame loop
3. CPU-side Clear / SetPixel
4. Bresenham DrawLine
5. Bounding-box triangle fill
6. Barycentric inside test
```

目前還沒有：

```text
1. Owned Framebuffer abstraction
2. Depth buffer
3. Edge-function rasterizer
4. Color / depth interpolation
5. Unit tests
6. Debug views
7. Mat4 / MVP / viewport transform
8. IShader
9. Material / Renderer / Scene / View / Camera
10. CommandQueue
```

---

## 5. 技術架構目標

長期目標不是立刻做完整 game engine，而是讓 Pixel-Renderer 長出一個最小 rendering engine skeleton。

目標結構：

```text
Application
  |
  v
Engine
  |
  +-------------------+
  |                   |
  v                   v
Renderer            Resource System
  |
  v
View ---- Camera
  |
  v
Scene
  |
  v
Renderable Objects
  |
  +-- Mesh
  +-- Transform
  +-- MaterialInstance
        |
        v
      Material
        |
        v
      IShader / Shader Program

Renderer frontend
  |
  v
CommandQueue
  |
  v
SoftwareBackend
  |
  v
Rasterizer
  |
  v
RenderDevice / Framebuffer
  |
  v
DisplayBackend
```

但這個 skeleton 必須建立在可信 pipeline 上。否則只會變成：

```text
class 名字像 engine，
但底層 triangle / depth / MVP 不可信。
```

---

## 6. 隱性主幹

除了功能順序，Pixel-Renderer 還需要一組隱性主幹。

這些東西不一定直接產生漂亮畫面，但決定專案能不能長期維護：

```text
1. Rendering conventions
2. Math-to-renderer mapping
3. Debug visualization
4. Testing / golden images
5. Data boundaries
6. Resource lifetime
7. Profiling metrics
8. Commercial engine mapping
9. Project narrative
10. Documentation structure
```

建議優先補三份短文件：

```text
docs/rendering_conventions.md
docs/pipeline_flow.md
docs/math_to_renderer.md
```

### 6.1 Rendering Conventions

應明確定義：

```text
Coordinate spaces: OS / WS / VS / CS / NDC / SS
Handedness: right-handed or left-handed
Matrix convention: row-major / column-major, vector on left / right
Depth convention: near/far range, smaller-is-nearer or larger-is-nearer
Screen convention: origin, y direction, pixel center rule
Triangle convention: winding order, top-left rule, degenerate handling
```

### 6.2 Math-to-Renderer Mapping

線性代數不要孤立學，必須接到 renderer：

```text
Field
  -> scalar values, interpolation weights

Vector Space
  -> directions, normals, tangents

Affine Space
  -> positions, points, barycentric combination

Basis
  -> coordinate spaces

Linear Map / Matrix
  -> rotation, scale, transform

Homogeneous Coordinates
  -> translation, projection, clip space

Barycentric Coordinates
  -> interpolation across triangles
```

### 6.3 Debug / Testing

Debug visualization 和 testing 是兩種不同工具：

```text
Debug Visualization
  runtime microscope
  用來看 pipeline 內部狀態

Testing
  correctness guardrail
  用來防止 regression
```

短期要先建立可信小島：

```text
Unit Test / Known Case
  -> Numeric Dump / Pipeline Trace
  -> 2D Debug Draw
  -> Stage-local Visualization
  -> Full Debug Visualization
  -> Golden Image Regression
```

---

## 7. 短期實作主線

目前最重要的判斷：

> 下一步不是馬上做 mini-Filament skeleton，而是先建立第一個可信小島。

第一個可信小島：

```text
Screen-space Triangle Pipeline
```

它包含：

```text
ScreenVertex
DrawTriangleScreenSpace
Bounding Box
Edge Function / Barycentric
Depth Buffer
Depth Test
Color Interpolation
Barycentric View
Depth View
Wireframe View
Unit Tests
Examples
```

這一步暫時不碰：

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

原因：這些都會增加錯誤來源。現在要先確認 rasterizer 本身可信。

---

## 8. Roadmap

### Milestone A: Screen-space Triangle Baseline

目標：

```text
不靠 MVP，直接畫 screen-space triangle。
```

輸出：

```text
ScreenVertex
DrawTriangleScreenSpace
Bounding Box
Barycentric
DepthBuffer
Barycentric View
Depth View
Wireframe View
test_barycentric
test_depth_buffer
test_bounding_box
examples/01_screen_triangle
examples/02_depth_overlap
```

完成標準：

```text
1. 能畫 screen-space RGB triangle
2. 能畫 depth overlap
3. Barycentric view 正常
4. Depth view 正常
5. Wireframe view 正常
6. barycentric / depth / bbox tests pass
```

### Milestone B: NDC / Viewport Baseline

目標：

```text
只測 NDC -> Screen。
```

輸出：

```text
Viewport
viewportTransform
DrawTriangleNDC
test_viewport
examples/03_ndc_triangle
```

完成標準：

```text
1. NDC triangle 正確出現在畫面中央
2. viewport tests pass
3. y flip 符合 rendering convention
```

### Milestone C: Math / Camera Baseline

目標：

```text
建立 Vec4 / Mat4 / LookAt / Orthographic 的可信基礎。
```

輸出：

```text
Vec4
Mat4
Translation / Scale / Rotation
LookAt
Orthographic
traceVertex
test_mat4
test_camera
examples/04_orthographic_triangle
```

完成標準：

```text
1. Mat4 tests pass
2. LookAt tests pass
3. traceVertex 可印出 OS / WS / VS / CS / NDC / SS
4. Orthographic triangle 可正常顯示
```

### Milestone D: Perspective Baseline

目標：

```text
接上 perspective projection，但仍使用最簡單 geometry。
```

輸出：

```text
Perspective matrix
clip.w trace
perspective divide
test_projection
examples/05_perspective_triangle
```

完成標準：

```text
1. Projection tests pass
2. Perspective triangle 可正常顯示
3. clip.w / NDC 數值合理
4. Depth 在 perspective 下仍正常
```

### Milestone E: IShader / Stage Data

目標：

```text
把 vertex stage / rasterizer interpolation / fragment stage 的資料邊界定清楚。
```

核心資料：

```text
VertexInput
VertexOutput
FragmentInput
ShaderContext
IShader
```

第一批 shader：

```text
FlatColorShader
VertexColorShader
DepthDebugShader
UVDebugShader
```

不要一開始做 Phong。

### Milestone F: Material / Renderer Skeleton

目標：

```text
建立 mini-Filament-style renderer skeleton。
```

核心物件：

```text
Renderer
Scene
View
Camera
Mesh
Transform
Material
MaterialInstance
```

目標 API：

```cpp
Scene scene;
Camera camera;
View view;
Renderer renderer;

view.setScene(&scene);
view.setCamera(&camera);

renderer.render(view);
```

### Milestone G: CommandQueue / Debug UI

目標：

```text
把 renderer frontend 和 software backend 分開。
```

最小順序：

```text
Renderer direct execution
  -> Renderer records DrawMeshCmd
  -> SoftwareBackend executes CommandQueue
```

Debug UI 第一版控制：

```text
RenderMode
Depth test on/off
Wireframe overlay
Perspective-correct interpolation on/off
Light direction
Camera position
RenderStats
```

---

## 9. 中長期分支

### 9.1 Material / Shader Direction

```text
Texture sampling
Perspective-correct interpolation
Phong / Blinn-Phong
Normal mapping
Shadow mapping
PBR concepts
```

對應職涯：

```text
graphics engineer
technical artist
rendering engineer
```

### 9.2 Engine Architecture Direction

```text
Scene / View / Camera
Material / MaterialInstance
CommandQueue
Resource handles
Batching / sorting
Frontend / backend split
```

對應職涯：

```text
engine engineer
rendering engineer
tools engineer
```

### 9.3 UI / Tooling Direction

```text
Immediate-mode debug UI
UiCommandList
ClipRect
Text rendering
DrawRect / DrawSpan
Runtime parameter panel
```

對應職涯：

```text
tools engineer
UI / game frontend engineer
technical artist
engine integration
```

### 9.4 Software GPU Direction

```text
Toy ShaderVM
PipelineState
Routine specialization
SIMD
JIT concept
SwiftShader / llvmpipe reading
```

對應職涯：

```text
software GPU engineer
driver engineer
compiler-adjacent rendering engineer
```

### 9.5 FPGA / Hardware GPU Direction

```text
Edge-function rasterizer hardware slice
Fixed-point barycentric
Depth test hardware model
Software golden model comparison
Verilog / SystemVerilog testbench
```

對應職涯：

```text
GPU architecture
hardware verification
FPGA graphics prototype
```

### 9.6 Engine Mirror Demo Direction

```text
Unity / Unreal mirror demo
Runtime debug panel
Shader Graph / Material Editor
Profiling notes
Concept mapping documentation
```

對應職涯：

```text
game client engineer
technical artist
tools engineer
engine engineer
graphics engineer
```

---

## 10. 近期不要做的事

短期不要做：

```text
1. 完整 ECS
2. 完整 Render Graph
3. 完整 Vulkan / D3D / Metal backend
4. 完整 SPIR-V parser
5. 完整 ShaderVM / JIT
6. 完整 FPGA GPU
7. 完整 UI framework
8. 大型 Unreal source deep dive
9. PBR / shadow / normal map 太早進主線
```

這些方向都可以存在，但現在只做 minimal slice。

目前優先順序：

```text
Correctness
  > Observability
  > Clear Architecture
  > Performance
```

---

## 11. Docs 導覽

### Stable doc folders

```text
docs/README.md
docs/foundations/
docs/architecture/
docs/verification/
docs/mapping/
docs/roadmap/
docs/adr/
```

用途：

```text
把 notes 中已經沉澱的方向整理成穩定規格文件。
foundations 放基本規約，architecture 放工程邊界，
verification 放 debug/testing，mapping 放商業引擎與職涯對照，
roadmap 放里程碑，adr 放重要架構決策。
```

### Graphics / Rasterization

```text
docs/tutorial-soft-renderer/index.html
```

用途：

```text
從 SetPixel 到 MVP / IShader / perspective-correct interpolation 的圖學主線。
```

### C++ / Engineering

```text
docs/tutorial-cpp/index.html
docs/tutorial-cpp/ROADMAP.md
docs/tutorial-cpp/AUDIT.md
```

用途：

```text
C++ theory、toolchain、build system、backend boundary、testing、dependency hygiene。
```

### Current architecture

```text
docs/tutorial-cpp/impl/i01_current_renderer_architecture_walkthrough.html
docs/notes/2026-05-15-renderer-architecture-map.md
docs/notes/2026-05-15-owned-framebuffer-ui-imgui.md
```

用途：

```text
理解目前 Application / ScreenManager / RenderDevice / Rasterizer 的責任與後續重構方向。
```

### 2026-05-22 planning notes

```text
docs/notes/2026-05-22-rendering_systems_learning_record_and_plan.md
docs/notes/2026-05-22-pixel_renderer_architecture_and_learning_roadmap.md
docs/notes/2026-05-22-pixel_renderer_next_trusted_pipeline_plan.md
docs/notes/2026-05-22-pixel_renderer_debug_testing_architecture.md
docs/notes/2026-05-22-pixel_renderer_hidden_spine.md
```

用途：

```text
整理 Rendering Systems 定位、Engine Mirror 策略、可信 pipeline、debug/testing、隱性主幹。
```

---

## 12. 最短結論

Pixel-Renderer 現在的主線應該收斂成：

```text
1. 先建立可信 Screen-space Triangle Pipeline
2. 再建立 NDC / Viewport / Mat4 / Camera / Perspective
3. 再建立 IShader / Stage Data
4. 再建立 Material / Renderer / Scene / View / Camera
5. 再建立 CommandQueue / Debug UI
6. 再做 Engine Mirror Demo
7. 最後依興趣分支到 software GPU / FPGA / UI / commercial engine
```

這份專案真正要訓練的能力是：

```text
從底層 pixel rule 出發，
一路理解現代引擎如何把 scene、material、shader、command、backend、UI、tooling
組成可維護的 rendering system。
```
