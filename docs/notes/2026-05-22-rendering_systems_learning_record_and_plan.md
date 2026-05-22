# Rendering Systems 學習思考紀錄與後續規劃

日期：2026-05-22  
主題：從 Pixel-Renderer 出發，理解渲染系統、商業引擎、UI、software GPU、FPGA 與職涯方向之間的關係

---

## 0. 這份筆記的目的

這份筆記不是單純整理「有哪些 graphics 專案可以看」，而是記錄今天思考後逐漸浮現出的核心問題：

> 我想要通透理解渲染，擁有自己的 renderer，能靈活擴充；同時也希望理解商業引擎，未來能在 Unity / Unreal / 自研引擎中有效工作。

目前還不需要急著決定最終要往 FPGA、SwiftShader、Filament、自製 engine、UI system 或商業遊戲 client 哪一條靠。更合理的策略是：

> 以 Pixel-Renderer 作為 Rendering Systems Lab，先建立共同主幹，再讓各個分支自然長出最小切片。

---

# 1. 今日核心結論

## 1.1 我真正關心的不是單一專案，而是 Rendering Systems

原本問題是從 Filament 開始：

- Filament 是什麼層級的 engine？
- 除了 Filament，還有哪些專案值得看？
- 為什麼 SwiftShader 放得比較後面？
- 如果不確定要走 FPGA / SwiftShader / engine / UI，該怎麼理解這些分支？
- 如果目標也包含米哈遊、暴雪等大型遊戲公司的工程師角色，該怎麼對應？

整理後發現，真正關心的是：

> 狀態、幾何、材質、UI、指令、後端，最後如何變成螢幕上的 pixels。

這個範圍比一般「Computer Graphics」更偏系統層，暫時可以命名為：

> **Rendering Systems**

它包含：

```text
Rendering Systems
│
├─ Software Rasterizer
│   Pixel-Renderer / TinyRenderer
│
├─ Rendering Engine
│   Filament / OGRE / Godot Renderer
│
├─ Graphics API Abstraction
│   bgfx / Diligent / The Forge
│
├─ Software GPU / Software Driver
│   SwiftShader / Mesa llvmpipe
│
├─ Hardware GPU / FPGA
│   raster unit / shader unit / golden model / verification
│
├─ UI Rendering System
│   immediate-mode UI / browser / desktop compositor
│
└─ Full Game Engine / Commercial Engine Usage
    Unity / Unreal / Godot / self-developed engine
```

---

## 1.2 不需要現在決定最終分支

目前感覺混亂，不是因為方向錯，而是因為 graphics stack 本來就很大，而且這些分支會互相交疊。

現在不應該急著選：

```text
我要做 FPGA
我要做 SwiftShader
我要做 Filament-like renderer
我要做完整 game engine
我要做 Unity / Unreal client
我要做 UI framework
```

比較好的定位是：

> 我想成為能從底層 rendering principle 理解商業引擎，並能擴充 renderer / tools / UI / client systems 的工程師。

這個定位保留了多條路：

```text
Graphics Engineer
Engine Engineer
Technical Artist
Game Client Engineer
Tools Engineer
Software GPU / Driver 方向
FPGA / GPU Architecture 方向
UI / Compositor / Browser Rendering 方向
```

---

# 2. 專案與技術層級地圖

## 2.1 Filament 的定位

Filament 不是 full game engine，而是：

> **real-time physically based rendering engine / renderer library / graphics middleware**

它處理：

```text
Scene
Camera
View
Renderer
Material
MaterialInstance
Light
Render pipeline
Backend abstraction
PBR
```

它不完整處理：

```text
gameplay
physics
audio
networking
full editor
scripting system
complete ECS/game framework
```

可以這樣定位：

```text
Game / App / Custom Engine
        ↓
    Filament
        ↓
OpenGL / Vulkan / Metal / WebGPU backend
        ↓
GPU driver
        ↓
GPU hardware
```

Filament 最值得學的不是「怎麼畫三角形」，而是：

> 當 renderer 長大後，API 和架構要如何切分。

重要概念：

```text
Engine
Renderer
SwapChain
View
Scene
Camera
Renderable
Material
MaterialInstance
Backend
```

---

## 2.2 TinyRenderer / Pixel-Renderer 的定位

這一層回答：

> 三角形到底怎麼變成 pixel？

它關心的是：

```text
SetPixel
framebuffer
line drawing
triangle rasterization
edge function
barycentric coordinates
Z-buffer
MVP
perspective-correct interpolation
texture sampling
Phong shading
normal mapping
shadow mapping
```

Pixel-Renderer 是目前最重要的主幹，因為它能建立 first principles。

如果沒有這一層，後面看 Filament、bgfx、SwiftShader、Unreal RHI 時，很容易只停在名詞層。

---

## 2.3 bgfx / Diligent / The Forge 的定位

這一層回答：

> 如何抽象 Vulkan / D3D12 / Metal / OpenGL / WebGPU？

它們不是完整 rendering engine，而是偏 graphics backend / RHI / API abstraction。

### bgfx

適合理解：

```text
multi-backend rendering abstraction
command submission
encoder
view
sort key
double-buffered command submission
API thread / render thread
shader cross-compilation
render state abstraction
```

它跟 Pixel-Renderer 的 Command Queue 很有對應關係。

Pixel-Renderer 的未來：

```text
Application
  → CommandQueue
  → SoftwareBackend
  → Rasterizer
  → Framebuffer
```

bgfx 類似：

```text
Application / API thread
  → submit commands
  → sort / batch
  → render thread
  → graphics API backend
```

---

### Diligent Engine

適合理解：

```text
Pipeline State Object
resource binding
descriptor model
command list
render pass
shader resource layout
modern explicit graphics API abstraction
```

比 bgfx 更貼近 Vulkan / D3D12 / Metal 的共同模型。

---

### The Forge

偏 production rendering framework。適合後期看：

```text
multi-platform renderer
modern explicit API
GPU resource management
threading
shader pipeline
render pass organization
```

目前不適合作為第一教材。

---

## 2.4 SwiftShader 的定位

SwiftShader 不是普通 renderer，而是：

> **software GPU / software Vulkan implementation**

它回答：

> 如果沒有 GPU，如何在 CPU 上實作 Vulkan graphics API 與 shader pipeline？

核心概念：

```text
Vulkan API implementation
VkDevice / VkQueue / VkCommandBuffer
Pipeline state
Descriptor set
SPIR-V
shader execution
JIT
SIMD
routine specialization
software rasterization
```

SwiftShader 跟 Pixel-Renderer 的關係：

```text
Pixel-Renderer:
  我自己定 API，自己呼叫 DrawTriangle，自己跑 software rasterizer。

SwiftShader:
  外部程式以為自己在使用 Vulkan。
  SwiftShader 要假裝自己是一張 Vulkan-capable GPU。
```

所以 SwiftShader 不是「下一個 TinyRenderer」。它更像：

```text
TinyRenderer
+ Vulkan driver object model
+ shader compiler / interpreter / JIT
+ SIMD backend
+ multithreaded scheduler
+ Vulkan conformance
```

因此：

```text
SwiftShader architecture docs：現在可以概念性看
SwiftShader source deep dive：後期再看
```

---

## 2.5 Mesa llvmpipe 的定位

Mesa llvmpipe 也是 software rasterizer，但更偏 Linux graphics stack / OpenGL / Gallium / LLVM JIT。

它代表：

```text
OpenGL API
  → Mesa state tracker
  → Gallium driver
  → llvmpipe
  → LLVM JIT
  → CPU execution
```

適合後期理解：

```text
software rasterizer optimization
LLVM IR
JIT shader execution
CPU multithreaded rendering
Linux driver architecture
```

---

## 2.6 Godot / Unreal / Unity 的定位

這些是 full game engine 或商業引擎生態。

它們不只包含 renderer，還包含：

```text
gameplay
scene system
editor
physics
audio
scripting
asset pipeline
animation
UI
tools
build / deployment
platform layer
```

### Godot

適合看：

```text
RenderingServer
RenderingDevice
SceneTree
Node
Resource system
renderer 如何嵌進 full engine
```

### Unreal

適合看：

```text
RHI
Render Graph
Material system
Shader permutation
Nanite
Lumen
Virtual Shadow Maps
Render Thread
Game Thread
RHI Thread
```

但 Unreal 太大，不能一開始直接亂讀 source。應該用任務切入。

### Unity

適合商業引擎使用與 client / TA / tool workflow：

```text
C# gameplay
URP / HDRP
Shader Graph
Custom Render Feature
Editor Tooling
UI Toolkit / UGUI
Profiling
Mobile game workflow
```

---

# 3. 共同主幹模型

不管是 Pixel-Renderer、Filament、SwiftShader、browser、desktop compositor、UI framework，本質上都有類似的資料流：

```text
State
  → Commands
  → Pipeline
  → Raster / Composite
  → Framebuffer / Surface
  → Present
```

更具體：

```text
Scene / UI / App State
        ↓
Frontend builds draw intent
        ↓
RenderCommand / DrawCommand
        ↓
PipelineState + Resources
        ↓
Vertex / Raster / Fragment / Blend / Composite
        ↓
Framebuffer / Texture / Surface
        ↓
SwapChain / Window / Compositor / Display
```

這個模型可以同時解釋：

## Pixel-Renderer

```text
Application state
  → DrawTriangleCmd / direct draw
  → Software rasterizer
  → framebuffer
  → Win32 DIB
```

## Filament

```text
Scene / View / Material
  → internal render commands
  → render pipeline
  → GPU backend
  → swapchain
```

## SwiftShader

```text
Vulkan commands
  → pipeline state
  → generated CPU routines
  → image memory
```

## Browser

```text
DOM / CSS / JS state
  → layout
  → paint
  → compositor layers
  → GPU / software backend
  → screen
```

## Desktop OS

```text
Application window surfaces
  → window manager / compositor
  → final framebuffer
  → display
```

## Immediate-mode UI

```text
Application state
  → widget function calls
  → UI draw commands
  → rectangles / text / clipping
  → framebuffer
```

---

# 4. UI / Browser / Desktop 為什麼也是同一條線

UI 處理不是旁支，它是 rendering systems 的重要分支。

UI 的核心問題是：

> 使用者互動狀態如何變成畫面，畫面又如何回應輸入？

它包含：

```text
input
hit testing
state
layout
paint
draw command
clip rect
batching
compositing
surface
present
```

## Immediate-mode UI

```text
App state
  → ui_button() / ui_slider()
  → widget state
  → UiCommandList
  → DrawRect / DrawText / SetClipRect
  → framebuffer
```

## React / Browser

```text
App state
  → React reconciliation
  → DOM update
  → CSS layout
  → paint commands
  → compositor layers
  → GPU / software renderer
```

## Desktop System

```text
App window
  → surface buffer
  → window compositor
  → final screen image
```

所以 UI 可以接到 Pixel-Renderer：

```text
scene_queue
  → render 3D scene

ui_queue
  → render UI overlay
```

這也對應 Pixel-Renderer 的既有規劃：未來可以做 immediate-mode UI、UI command list、clip rect、draw text、scene_queue / ui_queue 分離。

---

# 5. 職涯角色對應

這次討論也延伸到大型遊戲公司，例如米哈遊、暴雪、Ubisoft 等團隊中的角色。

關鍵是：

> 大型遊戲公司裡的 graphics engineer、engine engineer、technical artist、game client engineer，不是同一種工作。

---

## 5.1 Game Client Engineer

核心任務：

```text
gameplay feature
UI flow
input
camera
animation integration
network sync
resource loading
live service feature
performance bug
platform-specific issue
```

本質：

> 在大型 codebase 裡，把遊戲功能穩定接進引擎與產品流程。

需要能力：

```text
Unity / Unreal 使用能力
C# / C++
gameplay system
UI implementation
asset integration
profiling
cross-platform debugging
team collaboration
```

Pixel-Renderer 對這條有幫助，但不是直接對應。還需要補商業引擎實戰。

---

## 5.2 Technical Artist

核心任務：

```text
shader graph
material setup
VFX
rigging / animation pipeline
asset import pipeline
LOD / memory / draw call budget
artist tools
procedural workflow
editor extensions
```

本質：

> 站在美術與工程中間，把美術需求翻成引擎可執行、可維護、可控成本的方案。

需要能力：

```text
shader / material
engine editor
artist workflow
asset pipeline
scripting
performance budget
communication
```

如果我喜歡 shader、UI、tools、workflow，TA 是值得理解的方向。

---

## 5.3 Graphics / Rendering Engineer

核心任務：

```text
render pipeline
lighting
shadow
post-process
material system
shader framework
GPU profiling
render graph
draw call / batching / culling
platform-specific graphics API
artist-facing rendering tools
```

本質：

> 直接處理畫面如何被 renderer 產生，並且讓品質、效能、工具流程能支撐大型遊戲。

需要能力：

```text
C++
GPU pipeline
shader programming
Vulkan / D3D12 / Metal / RHI
RenderDoc / PIX / Nsight
PBR
math
engine architecture
performance profiling
```

這是最接近 Pixel-Renderer + Filament + bgfx + SwiftShader 交界的職位。

---

## 5.4 Engine Engineer

核心任務：

```text
memory allocator
job system
asset streaming
serialization
ECS / object model
build pipeline
editor tools
platform layer
physics integration
scripting binding
render backend
```

本質：

> 讓大型團隊可以穩定使用引擎生產遊戲。

需要能力：

```text
C++ systems programming
architecture
performance
threading
platform APIs
tooling
large codebase maintenance
```

Engine engineer 不一定專門做 graphics，但 graphics-aware engine engineer 很有價值。

---

## 5.5 Tools Engineer

核心任務：

```text
editor tools
asset validation
build automation
profiling tools
workflow tools
import / export pipeline
artist / designer tooling
```

這條跟 Technical Artist 和 Engine Engineer 交集很大。

對目前來說，如果想兼顧實用性與底層理解，tools / debug UI / profiling 工具會是一個很好的作品方向。

---

# 6. 靈活使用引擎的四個層級

「會用引擎」不是單一能力，可以拆成四層：

```text
Level 1：使用引擎
  會用 Scene、Prefab、Blueprint、Component、Material、Animation、UI

Level 2：擴充引擎
  會寫 custom component、editor tool、shader、render feature、import pipeline

Level 3：理解引擎
  知道 Scene / Render / Physics / Asset / UI / ECS / Threading 背後怎麼運作

Level 4：改造引擎
  能改 renderer、RHI、memory、job system、asset pipeline、platform backend
```

大致對應：

```text
Game Client Engineer:
  Level 1 ~ Level 2

Technical Artist:
  Level 1 ~ Level 2.5，偏 shader / material / tools / asset pipeline

Graphics Engineer:
  Level 3 ~ Level 4 的 rendering 區域

Engine Engineer:
  Level 3 ~ Level 4 的核心系統區域
```

我想要的應該至少是：

```text
Level 2：能實際擴充引擎
Level 3：理解引擎背後機制
```

長期視情況往 Level 4 靠。

---

# 7. Pixel-Renderer 的意義

Pixel-Renderer 不只是玩具專案。它可以變成：

> 我的 Rendering Systems Lab

它的價值不是直接取代 Unity / Unreal，而是建立底層直覺。

有了 Pixel-Renderer，看到商業引擎功能時可以反推出背後機制：

```text
Unity Material / Unreal Material
  → shader parameters、sampler、raster state、pipeline state

Camera
  → View matrix、Projection matrix、clip space、NDC、viewport transform

Canvas / UMG / UI Toolkit
  → layout、paint command、clip rect、batching、compositor

Post-process
  → render target、fullscreen pass、texture sampling

Shadow
  → depth pass、shadow map、bias、PCF、cascade

Performance issue
  → CPU submit、GPU fill-rate、overdraw、draw call、bandwidth、shader cost
```

所以 Pixel-Renderer 的長期價值是：

> 讓我不是只會操作引擎，而是理解引擎參數、功能、效能問題背後在 pipeline 哪一層發生。

---

# 8. 不同分支的核心問題

## 8.1 Software Rasterizer 路線

核心問題：

> 三角形、shader、深度、插值如何變成 framebuffer？

代表：

```text
Pixel-Renderer
TinyRenderer
Scratchapixel
```

適合訓練：

```text
first principles
math intuition
pipeline data flow
software implementation
```

---

## 8.2 Rendering Engine 路線

核心問題：

> renderer 長大後，Scene / View / Material / Renderer 如何分層？

代表：

```text
Filament
OGRE
Godot Renderer
```

適合訓練：

```text
API design
renderer architecture
scene organization
material system
render pipeline
```

---

## 8.3 Graphics Backend / RHI 路線

核心問題：

> draw intent 如何被錄製、排序、提交給不同 graphics API？

代表：

```text
bgfx
Diligent
The Forge
Unreal RHI
```

適合訓練：

```text
command buffer
pipeline state
resource binding
render pass
backend abstraction
threaded command submission
```

---

## 8.4 Software GPU / Driver 路線

核心問題：

> 如何在 CPU 上實作 GPU API 與 shader pipeline？

代表：

```text
SwiftShader
Mesa llvmpipe
```

適合訓練：

```text
Vulkan / OpenGL state model
SPIR-V / shader IR
JIT
SIMD
routine specialization
software rasterization optimization
```

---

## 8.5 FPGA / Hardware GPU 路線

核心問題：

> graphics pipeline 如何變成 clocked hardware data path？

代表方向：

```text
edge-function rasterizer
fixed-point arithmetic
pipeline stage
FIFO
latency / throughput
shader unit
memory bandwidth
golden model verification
```

Pixel-Renderer 可以作為 golden model。

---

## 8.6 UI / Browser / Desktop 路線

核心問題：

> UI state、layout、input、paint、composite 如何變成互動畫面？

代表：

```text
microui
Dear ImGui
browser rendering pipeline
React reconciliation
Skia
Windows DWM
Wayland / X11 compositor
```

適合訓練：

```text
immediate-mode UI
retained-mode UI
layout engine
hit testing
clip rect
dirty region
paint command
compositor
```

---

# 9. 推薦主線：Pixel-Renderer + Engine Mirror

目前最適合的策略不是只做 Pixel-Renderer，也不是只學 Unity / Unreal，而是：

> 同題雙解。

即：

```text
A 線：Pixel-Renderer
  從 first principles 實作底層原理

B 線：Unity / Unreal Mirror Demo
  在商業引擎中做同樣的效果或工具
```

這樣可以同時訓練：

```text
底層原理
商業引擎使用
引擎擴充能力
工具與 UI 能力
職涯可展示作品
```

---

## 9.1 同題雙解例子一：Phong / Blinn-Phong Material

Pixel-Renderer：

```text
手刻 vertex shader
手刻 fragment shader
normal interpolation
light equation
texture sampling
```

Unity / Unreal：

```text
用 Shader Graph / custom shader 做同樣效果
暴露 baseColor、normal、light direction、specular power 等參數
做 runtime debug panel 調參
```

可以學到：

```text
底層公式如何來
引擎中如何包成 material
artist 如何調參數
runtime 如何傳 uniform / texture
```

---

## 9.2 同題雙解例子二：Shadow Mapping

Pixel-Renderer：

```text
two-pass shadow map
depth map generation
depth compare
shadow bias
PCF
```

Unity / Unreal：

```text
觀察 directional light shadow
shadow resolution
bias
cascade shadow
PCF / filtering
```

可以學到：

```text
shadow map 原理
引擎參數為什麼存在
畫質與效能 tradeoff
```

---

## 9.3 同題雙解例子三：Immediate-mode UI vs Engine UI

Pixel-Renderer：

```text
ui_button
ui_slider
clip rect
UiCommandList
DrawRect
DrawText
```

Unity / Unreal：

```text
runtime debug panel
editor tool
UI Toolkit / UMG
parameter adjustment panel
```

可以學到：

```text
UI input / layout / rendering 本質
引擎 UI 系統如何管理 retained state
debug tooling 如何支援工程 workflow
```

---

## 9.4 同題雙解例子四：Render Command Queue

Pixel-Renderer：

```text
CommandQueue
scene_queue
ui_queue
SoftwareBackend
```

對照：

```text
bgfx command submission
Unreal RHI
Filament backend
Godot RenderingDevice
```

可以學到：

```text
為什麼引擎不直接 draw
為什麼要分 frontend / backend
為什麼要 render thread / command buffer
```

---

# 10. Pixel-Renderer Roadmap

## Phase 1：完成可解釋的 graphics pipeline

目標：

```text
DrawLine
DrawTriangle
Edge Function
Barycentric Coordinates
Z-buffer
Vec4 / Mat4
Model / View / Projection
Viewport Transform
IShader
Perspective-correct interpolation
Texture sampling
Phong Shader
Normal Map
Shadow Mapping
```

核心問題：

> 三角形與 shader 如何變成 framebuffer？

參考：

```text
TinyRenderer
Scratchapixel
Pixel-Renderer 自己的實作筆記
```

---

## Phase 2：mini-Filament style renderer skeleton

目標：

```text
Engine
Renderer
Scene
View
Camera
Entity
Mesh
Material
MaterialInstance
Transform
```

想要的 API：

```cpp
Engine engine;
Renderer renderer(engine);
Scene scene;
Camera camera;
View view;

view.setScene(&scene);
view.setCamera(&camera);

Mesh cube = Mesh::CreateCube();
Material mat = Material::CreatePhong();
MaterialInstance mi(&mat);

Entity e = scene.createEntity();
scene.setMesh(e, &cube);
scene.setMaterial(e, &mi);
scene.setTransform(e, Mat4::Translation(0, 0, -5));

renderer.beginFrame();
renderer.render(view);
renderer.endFrame();
```

內部仍然使用自己的 software rasterizer：

```text
Renderer::render(view)
  → collect renderables from Scene
  → compute MVP from Camera + Transform
  → run vertex shader
  → rasterize triangle
  → run fragment shader
  → depth test
  → write framebuffer
```

核心問題：

> renderer 長大後，資料邊界如何切？

對應：

```text
Filament
OGRE
Godot Renderer
```

---

## Phase 3：Material / MaterialInstance

從：

```cpp
IShader shader;
```

進化成：

```text
Material
  = shader behavior + raster state + parameter layout

MaterialInstance
  = Material + concrete parameter values
```

最小設計：

```cpp
struct RasterState {
    bool depthTest = true;
    bool depthWrite = true;
    bool cullBackFace = true;
    BlendMode blend = BlendMode::Opaque;
};

class Material {
public:
    std::unique_ptr<IShader> shader;
    RasterState rasterState;
    ParameterLayout parameterLayout;
};

class MaterialInstance {
public:
    Material* material;
    ParameterBlock parameters;
};
```

核心問題：

> shader、uniform、texture、raster state 如何組成 material system？

對應：

```text
Filament Materials
Unreal Material
Unity Shader Graph / SRP
```

---

## Phase 4：Command Queue / Frontend-Backend 分離

目標：

```text
Application / Scene
  → Renderer frontend
  → CommandQueue
  → SoftwareBackend
  → Rasterizer
  → Framebuffer
```

最小 command：

```cpp
struct ClearCmd {
    uint32_t color;
    float depth;
};

struct DrawMeshCmd {
    MeshHandle mesh;
    MaterialInstanceHandle material;
    Mat4 model;
};

using RenderCommand = std::variant<ClearCmd, DrawMeshCmd>;
```

核心分離：

```text
Frontend:
  決定要畫什麼

Backend:
  決定怎麼畫
```

進一步：

```text
Double buffered commands
scene_queue / ui_queue
State deduplication
Batching
Sorting
```

對應：

```text
bgfx
Diligent
Unreal RHI
Godot RenderingDevice
```

---

## Phase 5：加入 UI Overlay

先做最小 immediate-mode UI：

```cpp
ui_begin_frame();

if (ui_button("Toggle Wireframe")) {
    rendererSettings.wireframe = !rendererSettings.wireframe;
}

ui_slider("Light X", &light.x, -10.0f, 10.0f);

ui_end_frame();
```

內部產生：

```text
UiCommandList
  → DrawRect
  → DrawText
  → SetClipRect
```

Render flow：

```text
scene_queue
  → render 3D

ui_queue
  → render UI on top
```

核心問題：

> UI state / input / layout / draw commands 如何跟 renderer 接起來？

對應：

```text
microui
Dear ImGui
browser layout / paint / composite
desktop compositor
```

---

## Phase 6：mini software GPU 切片

不要直接做 SPIR-V。先做 toy ShaderVM。

概念：

```text
shader 不再只是 C++ virtual function
shader 是一段 program
runtime 可以解釋或編譯它
```

Toy bytecode 例子：

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

執行：

```cpp
ShaderVM vm;
vm.execute(vertexShaderBytecode, input, output);
```

核心問題：

> shader 如果是一段資料，renderer 要如何執行它？

對應：

```text
SwiftShader
SPIR-V
shader compiler
JIT
```

---

## Phase 7：PipelineState 與 routine specialization

建立：

```cpp
struct PipelineState {
    ShaderProgramHandle shader;
    RasterState rasterState;
    DepthState depthState;
    BlendState blendState;
};
```

Draw command：

```cpp
struct DrawCmd {
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    PipelineStateHandle pipeline;
    BindingSet bindings;
};
```

之後做 simple specialization：

```cpp
using PixelRoutine = void(*)(FragmentInput&);

PixelRoutine routine = getRoutine(pipelineState);
routine(fragment);
```

核心問題：

> 如何避免每個 pixel 都跑大量 runtime branch？

對應：

```text
SwiftShader Reactor
JIT
routine cache
pipeline specialization
```

---

## Phase 8：FPGA / Hardware GPU 切片

不用一開始做完整 GPU。先選一個硬體化單元：

```text
edge-function triangle rasterizer
```

輸入：

```text
triangle vertices
bounding box
pixel coordinate
```

輸出：

```text
inside / outside
barycentric weights
```

驗證：

```text
Pixel-Renderer software model
  ↔ FPGA hardware output
```

核心問題：

> 同一個 raster rule 如何用 software model 與 hardware model 對齊？

對應：

```text
golden model
hardware verification
fixed-point rasterization
pipeline stage
```

---

# 11. 建議閱讀與參考專案順序

## 現在最適合

```text
1. Pixel-Renderer 主線實作
2. TinyRenderer
3. Scratchapixel
4. Filament public API
5. Filament Materials Guide
6. bgfx internals 概念
7. SwiftShader architecture docs
```

重點：SwiftShader 現在只看 architecture，不深挖 source。

---

## 中期適合

```text
1. bgfx examples / command model
2. Diligent Engine 概念
3. Godot RenderingServer / RenderingDevice
4. Unity URP / Shader Graph / Editor Tooling
5. Unreal Material / RHI 概念
6. Vulkan Samples / Sascha Willems 局部範例
```

---

## 後期再看

```text
1. SwiftShader source deep dive
2. Mesa llvmpipe
3. The Forge
4. Unreal Engine renderer / Render Graph
5. Vulkan / D3D12 深入
6. FPGA rasterizer / shader unit
```

---

# 12. 角色能力地圖與訓練對應

| 角色 | 主要能力 | 對應訓練 |
|---|---|---|
| Game Client Engineer | Unity / Unreal、gameplay、UI、client system、效能 | Unity / Unreal 小遊戲功能、runtime UI、profiling |
| Technical Artist | shader、material、VFX、asset pipeline、工具 | Shader Graph、custom shader、editor tool、asset validation |
| Graphics Engineer | rendering pipeline、GPU、shader、profiling、C++ | Pixel-Renderer、RenderDoc、Unreal rendering、Filament / bgfx |
| Engine Engineer | memory、threading、asset、platform、tools、architecture | C++ systems、command queue、asset pipeline、engine source reading |
| Tools Engineer | editor、pipeline、automation、workflow | Unity Editor Tool、Unreal Editor Utility、asset import tool |
| UI / Game Frontend Engineer | UI architecture、layout、input、animation、UX performance | immediate-mode UI、Unity UI / Unreal UMG、browser rendering model |
| Software GPU / Driver Engineer | API implementation、shader execution、JIT、SIMD | SwiftShader、Mesa llvmpipe、ShaderVM、PipelineState |
| FPGA / GPU Architecture | hardware pipeline、fixed-point、verification | edge rasterizer hardware、golden model、Verilog / SystemVerilog |

目前最自然的定位：

> **Graphics-leaning Engine / Client / Tools hybrid**

也就是：

```text
懂 renderer first principles
能在商業引擎裡落地功能
能做 debug UI / tools
能理解 material / shader / render pipeline
未來可往 graphics engineer、engine engineer、TA、client engineer 分化
```

---

# 13. 下一步具體規劃

## 13.1 短期目標：不要急著選分支，先完成主幹

短期最穩的 milestone：

> 做出 mini-Filament style Pixel-Renderer skeleton，但內部仍然使用自己的 software rasterizer。

也就是：

```text
Engine
Renderer
Scene
View
Camera
Mesh
Material
MaterialInstance
Transform
```

這一步最不偏科，因為它同時保留以下方向：

```text
Filament / rendering engine
bgfx / command backend
SwiftShader / software backend
FPGA / golden model
UI / debug overlay
Unity / Unreal engine fluency
```

---

## 13.2 短期任務列表

### Task 1：整理目前 Pixel-Renderer 架構

輸出：

```text
README / docs
current architecture diagram
current class responsibility
next milestone list
```

要能清楚說明：

```text
Application 做什麼
RenderDevice 做什麼
Rasterizer 做什麼
IShader 未來做什麼
Framebuffer / DIB 怎麼接
```

---

### Task 2：完成 triangle + Z-buffer + MVP 基礎

優先順序：

```text
DrawTriangle
Z-buffer
Vec3 / Vec4 / Mat4
Model matrix
View matrix
Projection matrix
Viewport transform
```

完成後，renderer 才有真正 3D pipeline 主幹。

---

### Task 3：定義 VertexInput / VertexOutput / FragmentInput

目的：讓 shader stage input/output 明確化。

例：

```cpp
struct VertexInput {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

struct VertexOutput {
    Vec4 clipPosition;
    Vec3 worldNormal;
    Vec2 uv;
    float invW;
};

struct FragmentInput {
    Vec3 worldNormal;
    Vec2 uv;
    float depth;
};
```

這是之後做 interpolation、Material、ShaderVM 的基礎。

---

### Task 4：建立 Material / MaterialInstance 雛形

先不要做複雜 asset pipeline，只要把 `IShader` 包進 material。

輸出：

```text
Material
MaterialInstance
RasterState
ParameterBlock
```

---

### Task 5：建立 Renderer / Scene / View / Camera skeleton

先不做複雜 ECS，只做簡單容器。

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

---

### Task 6：做 Debug UI 的最小原型

先做：

```text
button
slider
checkbox
clip rect
text rendering
```

用來控制：

```text
wireframe on/off
light position
camera position
render mode: normal / depth / uv / shaded
```

這能把 UI 分支和 renderer 主幹接起來。

---

### Task 7：Unity / Unreal Mirror Demo

選一個小題目，在商業引擎做對照。

首選：

```text
Phong / Blinn-Phong material + runtime debug panel
```

對照文檔要寫：

```text
Pixel-Renderer 怎麼做
Unity / Unreal 怎麼暴露這件事
兩者概念如何對應
這在大型團隊中對應哪個角色
```

---

# 14. 中期作品形式

建議把作品做成：

```text
Pixel-Renderer Lab
  +
Engine Mirror Demo
  +
Concept Mapping Notes
```

## Pixel-Renderer Lab

展示：

```text
software rasterizer
MVP
Z-buffer
texture
Phong
Material / MaterialInstance
CommandQueue
Debug UI
```

## Engine Mirror Demo

展示：

```text
Unity 或 Unreal 中同樣的 shader / UI / render feature
debug panel
profiling notes
```

## Concept Mapping Notes

每個題目都整理：

```text
底層原理
Pixel-Renderer implementation
Unity / Unreal implementation
Filament / bgfx / SwiftShader 對應概念
職位能力對應
```

這種作品比單純「我會 Unity」或「我寫了一個 rasterizer」更有說服力。

它展示的是：

> 我知道引擎功能背後的原理，也能在商業引擎裡落地使用。

---

# 15. 暫時不做的事情

為了避免分散，短期先不要做：

```text
完整 Vulkan renderer
完整 SPIR-V parser
完整 SwiftShader source deep dive
完整 FPGA GPU
完整 game engine editor
大型 ECS
完整 browser layout engine
完整 Unreal source reading
```

這些不是不重要，而是目前過早。

目前策略是：

```text
每個分支只做最小切片
所有切片都回到 Pixel-Renderer 主幹
```

---

# 16. 判斷自己未來偏向哪條路的觀察問題

之後可以透過做專案時的感受，觀察自己更偏哪條路。

## 如果最在意：為什麼畫面是這樣算出來的？

可能偏：

```text
software rasterizer
graphics math
rendering fundamentals
```

---

## 如果最在意：renderer 長大後如何保持架構乾淨？

可能偏：

```text
rendering engine architecture
engine programmer
graphics framework
```

---

## 如果最在意：Vulkan / D3D / Metal 背後共同抽象是什麼？

可能偏：

```text
graphics backend
RHI
engine low-level rendering
```

---

## 如果最在意：沒有 GPU 時如何跑 GPU pipeline？

可能偏：

```text
software GPU
SwiftShader
Mesa
shader compiler / JIT
```

---

## 如果最在意：pipeline 能不能變成硬體？

可能偏：

```text
FPGA
GPU architecture
hardware verification
```

---

## 如果最在意：React / browser / desktop window 怎麼變成畫面？

可能偏：

```text
UI systems
browser rendering
compositor
tools / frontend infrastructure
```

---

## 如果最在意：商業引擎裡怎麼有效做功能？

可能偏：

```text
Game Client Engineer
Technical Artist
Tools Engineer
Engine integration
```

---

# 17. 一句話總結

目前方向可以暫時定義為：

> 以 software renderer 為核心，逐步理解 rendering pipeline、renderer architecture、UI/compositor、software GPU、hardware GPU，以及商業引擎使用與擴充之間的共同結構。

下一步不是急著決定要走 Filament、SwiftShader、FPGA、Unity、Unreal 或 UI，而是：

> 讓 Pixel-Renderer 長出一個可以對照大型系統的縮小版。

最推薦的下一個 milestone：

```text
mini-Filament style Pixel-Renderer skeleton
  → Engine / Renderer / Scene / View / Camera / Mesh / Material / MaterialInstance
  → 內部仍使用自己的 software rasterizer
  → 後續接 CommandQueue、Debug UI、Engine Mirror Demo
```

這一步完成後，再根據興趣與職涯需求，往以下方向分化：

```text
Rendering Engineer:
  深入 Filament / Unreal RHI / GPU profiling

Engine Engineer:
  深入 CommandQueue / memory / threading / asset pipeline

Technical Artist:
  深入 material / shader graph / VFX / tools

Game Client Engineer:
  深入 Unity / Unreal gameplay / UI / performance

Software GPU:
  深入 SwiftShader / ShaderVM / JIT / SIMD

FPGA / GPU Architecture:
  深入 edge rasterizer hardware / golden model / verification

UI / Browser / Desktop:
  深入 immediate UI / layout / compositor / surface model
```

---

# 18. 最後保留的原則

1. **不急著選最終身份。** 先建立共同主幹。
2. **每個分支只做最小切片。** 避免一開始掉進巨大 codebase。
3. **所有分支回到 Pixel-Renderer。** 讓它成為自己的 Rendering Systems Lab。
4. **同題雙解。** 同一個概念在 Pixel-Renderer 和 Unity / Unreal 各做一次。
5. **作品要展示理解與落地。** 不只展示效果，也展示 mapping、tradeoff、架構圖、debug UI。
6. **保留職涯彈性。** 往 graphics、engine、TA、client、tools、software GPU、FPGA 都還可以。

目前最有價值的不是立即做出大引擎，而是逐步建立一套自己能完全解釋、能擴充、能對照商業系統的 rendering stack。

