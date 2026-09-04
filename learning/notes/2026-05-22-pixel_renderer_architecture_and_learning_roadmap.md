# Pixel-Renderer 架構設計與學習 Roadmap

日期：2026-05-22  
定位：Pixel-Renderer 作為個人的 Rendering Systems Lab  
目標：從 first principles 建立 software renderer，並逐步連到 renderer architecture、commercial engine fluency、UI/compositor、software GPU、FPGA/golden model 等分支。

---

## 0. 文件目的

這份文件不是單純列 TODO，而是幫 Pixel-Renderer 建立一條清楚的「技術主幹」。

目前核心問題是：

> 我想通透理解渲染，擁有自己的 renderer，能靈活擴充；同時未來也能理解並使用 Unity / Unreal / Filament / 自研引擎等大型系統。

因此 Pixel-Renderer 不應該只是：

```text
DrawPixel → DrawLine → DrawTriangle
```

而應該逐步長成：

```text
SetPixel / Framebuffer
  → Rasterizer
  → Shader / Material
  → Scene / View / Camera
  → Renderer frontend
  → CommandQueue
  → SoftwareBackend
  → Debug UI
  → Engine Mirror Demo
```

這樣它會同時支撐以下分支：

```text
Rendering Systems
│
├─ Software Rasterizer
├─ Renderer Architecture
├─ Material / Shader System
├─ Graphics API Abstraction / RHI
├─ UI / Compositor System
├─ Software GPU / SwiftShader-like 路線
├─ FPGA / Hardware GPU golden model
└─ Unity / Unreal / Commercial Engine Fluency
```

---

# 1. 核心原則

## 1.1 目前不急著鎖死最終方向

目前不需要決定：

```text
我要做 FPGA
我要做 SwiftShader
我要做 Filament-like renderer
我要做完整 game engine
我要做 Unity/Unreal client
我要做 technical artist
我要做 UI framework
```

比較合理的策略是：

> 用 Pixel-Renderer 建立共同主幹，再用小型實驗觀察自己更適合往哪個分支深化。

---

## 1.2 Pixel-Renderer 的主軸

Pixel-Renderer 應該被視為：

> 一個從 `SetPixel()` 開始的 Rendering Systems Lab。

它不是單純做畫面，而是用自己的手把以下概念從底層接起來：

```text
數學結構
  → 幾何表示
  → 座標變換
  → 三角形 rasterization
  → 深度測試
  → shader
  → material
  → render command
  → backend execution
  → UI overlay
  → engine abstraction
```

---

## 1.3 不要過早 over-engineering

短期不要做：

```text
完整 ECS
完整 Render Graph
完整 Vulkan backend
完整 SPIR-V parser
完整 JIT
完整 FPGA GPU
完整 browser layout engine
完整 commercial game engine clone
```

現在要做的是：

```text
每個大型系統概念，只做一個 minimal slice。
```

也就是：

```text
Material system：先做 Material / MaterialInstance，不做完整 material compiler
CommandQueue：先做單執行緒 queue，不做多執行緒 render thread
UI：先做 debug UI，不做完整 retained UI toolkit
ShaderVM：先做 toy bytecode，不碰 SPIR-V
FPGA：先做 edge-function rasterizer golden-model 對照，不做完整 programmable GPU
```

---

# 2. 總體架構藍圖

## 2.1 最終希望長出的核心結構

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
    +---- Mesh
    +---- Transform
    +---- MaterialInstance
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
Win32 DIB / Present
```

---

## 2.2 目前最重要的 milestone

下一個穩定 milestone：

```text
mini-Filament-style Pixel-Renderer skeleton
```

核心 class：

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

暫時不做完整 ECS，不做完整資源系統，不做多 backend。

內部仍使用 software rasterizer。

---

## 2.3 為什麼是 mini-Filament skeleton？

因為現在如果只繼續寫：

```cpp
rasterizer.DrawTriangle(v0, v1, v2, shader);
```

會停留在「低階 drawing API」。

而真正的 renderer / engine 需要的是：

```cpp
renderer.render(view);
```

這代表概念從：

```text
我直接畫一個 triangle
```

提升為：

```text
我描述一個 scene，renderer 從某個 view 把它畫出來。
```

這是從 toy rasterizer 走向 renderer architecture 的關鍵轉折。

---

# 3. 分階段 Roadmap

## Phase 0：現有基礎整理

### 目標

讓目前 codebase 的職責邊界清楚。

### 目前應該已經有或正在建立的模組

```text
core/
  screen_manager
  render_device
  application

render/
  rasterizer
  shader
  texture
  math_utils

ui/
  ui_context
```

### 各層責任

```text
ScreenManager
  Win32 window
  input events
  DIB setup
  BitBlt / present

RenderDevice
  framebuffer ownership
  SetPixel
  Clear
  depth buffer later

Rasterizer
  DrawLine
  DrawTriangle
  barycentric / edge function
  interpolation

Application
  main loop
  OnInit / OnUpdate / OnRender
  time step

Shader
  vertex / fragment abstraction

Texture
  image data
  sample nearest / bilinear later

Math
  Vec2 / Vec3 / Vec4 / Mat4
  transform utilities
```

### 這一階段要輸出

```text
1. README 架構圖
2. 每個 class 的 responsibility note
3. 一個最小 render demo
4. 一份目前 pipeline flow 文件
```

---

## Phase 1：完成基礎 software graphics pipeline

### 核心問題

> 三角形如何從頂點資料變成 framebuffer 裡的 pixels？

### 必做項目

```text
DrawLine
DrawTriangle
Bounding Box
Edge Function
Barycentric Coordinates
Z-buffer
Color interpolation
Depth interpolation
```

### 推薦順序

```text
1. DrawLine: DDA / Bresenham
2. DrawTriangle: bounding box + barycentric
3. Z-buffer: depth test + depth clear
4. Color interpolation
5. UV interpolation
6. Perspective-correct interpolation
```

### 這一階段要學什麼

```text
2D coordinate system
pixel center convention
triangle coverage rule
edge function
orientation / winding order
barycentric coordinates
linear interpolation
Z-buffer invariant
```

### 關鍵 invariant

```text
對每個 pixel：
  如果 pixel 在 triangle 內，計算 barycentric weights。
  如果 depth 比目前 depth buffer 更近，更新 color buffer 與 depth buffer。
```

### 建議測試

```text
1. 單一三角形純色填滿
2. RGB 三頂點顏色插值
3. 兩個重疊三角形測 depth
4. triangle winding 正反測試
5. 邊界 pixel 是否穩定
```

---

## Phase 2：線性代數與 3D Transform 主幹

### 核心問題

> 3D 世界中的點，如何透過矩陣變換變成螢幕上的 pixel？

### 必做項目

```text
Vec2 / Vec3 / Vec4
Mat4
matrix multiplication
translation / rotation / scale
Model matrix
View matrix
Projection matrix
MVP
Perspective divide
Viewport transform
```

---

# 4. 線性代數學習如何接到 Pixel-Renderer

你目前正在從比較數學系的角度學線性代數：field、vector space 開始。這非常適合接 graphics，但要注意：graphics 裡的很多空間其實不只是 vector space，還包含 affine space、projective space、coordinate representation。

## 4.1 Field

數學上：

```text
Field F
  是可以做加、減、乘、除的 scalar 系統。
```

在 renderer 裡：

```text
float / double
  是實作上近似 real number field 的 scalar type。
```

要注意：

```text
數學 field 是精確結構。
float 不是真正 field，因為有 rounding、overflow、NaN、Inf、非結合性。
```

這會影響 graphics：

```text
depth precision
z-fighting
matrix inversion stability
normal normalization error
barycentric edge cases
```

---

## 4.2 Vector Space

數學上：

```text
Vector space V over field F
  是一組 vectors，加上 vector addition 與 scalar multiplication。
```

在 graphics 裡：

```text
Vec3
  可以表示 direction、normal、velocity、offset。
```

但是要小心：

```text
point 不是 vector。
direction 才比較自然是 vector。
```

例如：

```text
Point + Vector = Point
Point - Point = Vector
Vector + Vector = Vector
Point + Point 沒有幾何意義
```

C++ 裡常常都用 `Vec3` 表示 point 和 direction，但語意不同。

### 對 Pixel-Renderer 的啟示

可以考慮概念上區分：

```cpp
struct Point3 { float x, y, z; };
struct Vec3   { float x, y, z; };
```

實作上可以暫時都用 `Vec3`，但文件裡要明確標示：

```text
position 是 point
direction / normal 是 vector
```

---

## 4.3 Basis 與 Coordinates

數學上：

```text
basis 是一組能唯一表示 vector 的 independent generators。
```

graphics 裡：

```text
一個 Vec3 不是抽象 vector 本身，而是某個 basis 下的 coordinates。
```

例如同一個物理方向，在不同座標系中有不同 coordinates：

```text
object space
world space
view space
clip space
NDC
screen space
```

### 對 Pixel-Renderer 的啟示

不要只寫：

```cpp
Vec3 position;
```

心裡要知道：

```text
這個 position 是在哪個 coordinate space？
```

建議命名：

```cpp
Vec3 positionOS; // object space
Vec3 positionWS; // world space
Vec3 positionVS; // view space
Vec4 positionCS; // clip space
Vec3 positionNDC;
Vec2 positionSS; // screen space
```

---

## 4.4 Linear Map 與 Matrix

數學上：

```text
linear map T: V → W
```

滿足：

```text
T(a * u + b * v) = a * T(u) + b * T(v)
```

在選定 basis 後，linear map 可以用 matrix 表示。

graphics 裡：

```text
rotation
scale
shear
```

都是 linear maps。

但是 translation 不是 linear map，因為：

```text
T(0) ≠ 0
```

所以需要 affine transform / homogeneous coordinates。

---

## 4.5 Affine Space

graphics 裡的 object/world 比較自然是 affine space：

```text
point 沒有自然 origin
vector 表示兩個 points 的差
```

Model transform 通常是 affine transform：

```text
p_world = R * S * p_object + t
```

它不是純 linear map，因為有 translation。

---

## 4.6 Homogeneous Coordinates

為了把 affine transform 寫成 matrix multiplication，graphics 使用 homogeneous coordinates。

```text
Point:     (x, y, z, 1)
Direction: (x, y, z, 0)
```

因此：

```text
translation 會影響 point
translation 不會影響 direction
```

例如：

```text
[ R  t ] [p]
[ 0  1 ] [1]
```

會得到：

```text
R * p + t
```

但對 direction：

```text
[ R  t ] [v]
[ 0  1 ] [0]
```

會得到：

```text
R * v
```

### 對 Pixel-Renderer 的啟示

`Vec4.w` 不是只是多一個數字，而是語意標籤：

```text
w = 1：point
w = 0：direction
clip space position：需要保留 w 做 perspective divide
```

---

## 4.7 MVP Pipeline 與座標空間

典型流程：

```text
Object Space
  -- Model -->
World Space
  -- View -->
View / Camera Space
  -- Projection -->
Clip Space
  -- Perspective Divide -->
NDC
  -- Viewport Transform -->
Screen Space
```

ASCII：

```text
positionOS
   |
   | Model
   v
positionWS
   |
   | View
   v
positionVS
   |
   | Projection
   v
positionCS = (x, y, z, w)
   |
   | divide by w
   v
positionNDC = (x/w, y/w, z/w)
   |
   | viewport transform
   v
positionSS = pixel coordinate
```

---

## 4.8 Projection Matrix 為什麼不是一般 3D 線性變換

Perspective projection 的核心不是把遠物體縮小而已，而是：

```text
clip space 裡保留 w
NDC 時做 x/w, y/w, z/w
```

所以它依賴 homogeneous coordinates / projective geometry。

對 Pixel-Renderer 來說，重點是：

```text
不要太早把 Vec4 clip position 轉成 Vec3。
```

因為後面的 perspective-correct interpolation 需要 `1/w`。

---

## 4.9 Barycentric Coordinates 與 Affine Combination

Barycentric coordinates 可以理解為 affine coordinates。

對 triangle 三頂點 `A, B, C`，點 `P` 可以表示成：

```text
P = αA + βB + γC
α + β + γ = 1
```

若：

```text
α, β, γ >= 0
```

則 P 在三角形內部或邊界。

這不是普通 linear combination，而是 affine combination，因為係數和為 1。

### 對 Pixel-Renderer 的啟示

Barycentric weights 同時可以用於：

```text
inside triangle test
color interpolation
depth interpolation
UV interpolation
normal interpolation
```

但要注意：

```text
screen space barycentric 對 attributes 不是全部都 perspective-correct。
```

所以 UV / depth / varying attribute 需要 `1/w trick`。

---

## 4.10 Linear Algebra 到 Renderer 的學習對照表

| 線性代數概念 | Graphics 對應 | Pixel-Renderer 任務 |
|---|---|---|
| Field | scalar type / float precision | float error、depth precision notes |
| Vector Space | direction / normal / velocity | Vec3、dot、cross |
| Basis | coordinate representation | object/world/view space 命名 |
| Linear Map | rotation / scale | Mat4 rotation / scale |
| Matrix Representation | chosen basis 下的 linear map | Mat4 multiplication |
| Affine Space | points + vectors | position vs direction 語意區分 |
| Affine Transform | model transform | Model matrix |
| Homogeneous Coordinates | translation + projection | Vec4、w=0/1、clip space |
| Projective Transform | perspective projection | Projection matrix、perspective divide |
| Dot Product | projection / lighting | N·L、view angle |
| Cross Product | normal / orientation | face normal、edge function intuition |
| Change of Basis | camera transform | View matrix / LookAt |
| Barycentric Coordinates | triangle interpolation | rasterizer interpolation |
| Subspace | tangent plane / normal space | TBN、normal mapping later |
| Eigenvectors | principal axes / transforms | optional later |

---

# 5. Phase 3：mini-Filament-style Renderer Skeleton

## 5.1 核心問題

> 如何從「直接畫 triangle」進化成「renderer 從 view 渲染 scene」？

---

## 5.2 最小 class 設計

### Engine

第一版 `Engine` 不要太神化。它只是資源與核心系統的組裝點。

```cpp
class Engine {
public:
    Engine(RenderDevice& device);

    RenderDevice& getDevice();
    Renderer& getRenderer();

private:
    RenderDevice& device;
    std::unique_ptr<Renderer> renderer;
};
```

第一版 `Engine` 可以做得很薄，甚至先不寫也可以。

---

### Renderer

```cpp
class Renderer {
public:
    Renderer(RenderDevice& device, Rasterizer& rasterizer);

    void beginFrame();
    void render(View& view);
    void endFrame();

private:
    RenderDevice& device;
    Rasterizer& rasterizer;
};
```

第一版 `Renderer::render(view)` 可以直接呼叫 rasterizer。之後再改成錄製 CommandQueue。

---

### View

```cpp
class View {
public:
    void setScene(Scene* scene);
    void setCamera(Camera* camera);
    void setViewport(int x, int y, int w, int h);

    Scene* getScene() const;
    Camera* getCamera() const;
    Rect getViewport() const;

private:
    Scene* scene = nullptr;
    Camera* camera = nullptr;
    Rect viewport;
};
```

`View` 的概念是：

```text
我要從哪個 camera 看哪個 scene，並畫到哪個 viewport。
```

未來 View 還可以加：

```text
clear color
render flags
post-process settings
shadow settings
debug draw settings
```

---

### Camera

```cpp
class Camera {
public:
    void lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
    void setPerspective(float fovY, float aspect, float nearZ, float farZ);

    Mat4 getViewMatrix() const;
    Mat4 getProjectionMatrix() const;

private:
    Vec3 eye;
    Vec3 target;
    Vec3 up;

    float fovY;
    float aspect;
    float nearZ;
    float farZ;

    Mat4 view;
    Mat4 projection;
};
```

這會強迫你真正理解：

```text
LookAt matrix
Perspective projection
near / far plane
aspect ratio
clip space convention
```

---

### Scene

第一版不用 ECS，直接用 vector。

```cpp
class Scene {
public:
    void addRenderable(const Renderable& renderable);
    const std::vector<Renderable>& getRenderables() const;

private:
    std::vector<Renderable> renderables;
};
```

---

### Renderable

```cpp
struct Renderable {
    Mesh* mesh = nullptr;
    MaterialInstance* material = nullptr;
    Transform transform;
};
```

這個結構對應到商業引擎中的：

```text
Unity: MeshRenderer + MeshFilter + Transform + Material
Unreal: StaticMeshComponent + MaterialInstance + Transform
Filament: Renderable + Transform + MaterialInstance
```

---

### Transform

第一版可以直接存 matrix：

```cpp
struct Transform {
    Mat4 model = Mat4::Identity();
};
```

中期可以改成：

```cpp
struct Transform {
    Vec3 position;
    Quat rotation;
    Vec3 scale;

    Mat4 toMatrix() const;
};
```

不要一開始就寫 scene graph。先只做 per-object model matrix。

---

### Mesh

```cpp
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    Color color;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
```

第一版只要支援 indexed triangle list。

---

### Material / MaterialInstance

```cpp
struct RasterState {
    bool depthTest = true;
    bool depthWrite = true;
    bool cullBackFace = true;
};

class Material {
public:
    IShader* shader = nullptr;
    RasterState rasterState;
};

class MaterialInstance {
public:
    Material* material = nullptr;

    Vec3 baseColor = Vec3(1.0f, 1.0f, 1.0f);
    Texture* diffuseTexture = nullptr;
};
```

第一版不要做複雜 parameter reflection。

---

# 6. Render Flow 詳細設計

## 6.1 外部使用方式

希望 main.cpp 看起來像：

```cpp
int main() {
    Application app(800, 600);

    Engine engine(app.getRenderDevice());
    Renderer& renderer = engine.getRenderer();

    Scene scene;
    Camera camera;
    View view;

    camera.lookAt(
        Vec3(0, 0, 3),
        Vec3(0, 0, 0),
        Vec3(0, 1, 0)
    );
    camera.setPerspective(60.0f, 800.0f / 600.0f, 0.1f, 100.0f);

    view.setScene(&scene);
    view.setCamera(&camera);
    view.setViewport(0, 0, 800, 600);

    Mesh triangle = Mesh::CreateTriangle();
    Material phong = Material::CreatePhong();
    MaterialInstance redMaterial(&phong);
    redMaterial.baseColor = Vec3(1, 0, 0);

    Renderable obj;
    obj.mesh = &triangle;
    obj.material = &redMaterial;
    obj.transform.model = Mat4::Identity();

    scene.addRenderable(obj);

    while (app.running()) {
        renderer.beginFrame();
        renderer.render(view);
        renderer.endFrame();
    }
}
```

---

## 6.2 Renderer::render(view) 內部流程

```text
Renderer::render(view)
  1. Validate view.scene and view.camera
  2. Fetch camera matrices
  3. For each renderable in scene:
       a. Fetch mesh
       b. Fetch material instance
       c. Compute MVP
       d. For each triangle:
            i. Build VertexInput
           ii. Run vertex shader
          iii. Clip / reject later
           iv. Perspective divide
            v. Viewport transform
           vi. Rasterize triangle
          vii. Run fragment shader per covered pixel
         viii. Depth test
           ix. Write color
```

---

## 6.3 第一版可以暫時省略的 pipeline 階段

暫時省略：

```text
frustum culling
near-plane clipping
backface culling 的完整處理
render sorting
batching
alpha blending
MSAA
mipmapping
normal mapping
shadow mapping
```

先讓 flow 清楚。

---

# 7. Shader Stage 設計

## 7.1 為什麼要先明確 VertexInput / VertexOutput / FragmentInput

如果 shader 只是：

```cpp
Vec4 vertex(Vertex v);
Color fragment(...);
```

很快會不知道 varyings 怎麼傳。

建議先定義 pipeline stage data：

```cpp
struct VertexInput {
    Vec3 positionOS;
    Vec3 normalOS;
    Vec2 uv;
    Color color;
};

struct VertexOutput {
    Vec4 positionCS;   // clip space
    Vec3 positionWS;
    Vec3 normalWS;
    Vec2 uv;
    Color color;
    float invW;
};

struct FragmentInput {
    Vec3 positionWS;
    Vec3 normalWS;
    Vec2 uv;
    Color color;
    float depth;
};
```

---

## 7.2 IShader 第一版

```cpp
class IShader {
public:
    virtual ~IShader() = default;

    virtual VertexOutput vertex(
        const VertexInput& in,
        const ShaderContext& ctx
    ) = 0;

    virtual Color fragment(
        const FragmentInput& in,
        const ShaderContext& ctx
    ) = 0;
};
```

---

## 7.3 ShaderContext

```cpp
struct ShaderContext {
    Mat4 model;
    Mat4 view;
    Mat4 projection;
    Mat4 mvp;

    Vec3 cameraPositionWS;
    Vec3 lightDirectionWS;

    MaterialInstance* material = nullptr;
};
```

第一版可以用這個簡化 uniform 系統。

未來才拆成：

```text
UniformBuffer
ParameterBlock
BindingSet
PipelineState
```

---

# 8. Material System 設計方向

## 8.1 Naive 方法

最 naive 的做法：

```cpp
rasterizer.DrawTriangle(v0, v1, v2, shader);
```

問題：

```text
shader 和物件參數混在一起
沒有 material / material instance 分離
無法表達 raster state
每個 object 的參數不容易管理
未來無法接近 Unity / Unreal / Filament 的概念
```

---

## 8.2 Material / MaterialInstance 分離

比較好的結構：

```text
Material
  = shared rendering behavior
  = shader + raster state + parameter layout

MaterialInstance
  = per-object parameter values
  = baseColor / texture / roughness / metallic / custom params
```

例子：

```text
Material: PhongMaterial
  shader = PhongShader
  depthTest = true
  cullBackFace = true

MaterialInstance A:
  baseColor = red
  diffuseTexture = brick

MaterialInstance B:
  baseColor = blue
  diffuseTexture = metal
```

---

## 8.3 對應商業引擎

```text
Unity:
  Shader + Material

Unreal:
  Material + Material Instance

Filament:
  Material + MaterialInstance
```

Pixel-Renderer 要刻意做這個分離，因為這是 engine fluency 的重要概念。

---

# 9. CommandQueue 設計方向

## 9.1 為什麼不要太早做 CommandQueue

如果還沒有 Scene / View / Material，就直接做 CommandQueue，很容易變成：

```text
DrawLineCmd
DrawTriangleCmd
SetPixelCmd
```

這比較像 drawing command list，不像 renderer command queue。

比較好的順序：

```text
Scene / View / Material
  → Renderer frontend
  → CommandQueue
  → Backend
```

---

## 9.2 第一版 CommandQueue

```cpp
struct ClearCmd {
    Color color;
    float depth;
};

struct DrawMeshCmd {
    Mesh* mesh;
    MaterialInstance* material;
    Mat4 model;
    Mat4 view;
    Mat4 projection;
};

using RenderCommand = std::variant<ClearCmd, DrawMeshCmd>;

class CommandQueue {
public:
    void push(const RenderCommand& cmd);
    void clear();
    const std::vector<RenderCommand>& commands() const;

private:
    std::vector<RenderCommand> cmds;
};
```

---

## 9.3 SoftwareBackend

```cpp
class SoftwareBackend {
public:
    SoftwareBackend(RenderDevice& device, Rasterizer& rasterizer);

    void execute(const CommandQueue& queue);

private:
    void executeClear(const ClearCmd& cmd);
    void executeDrawMesh(const DrawMeshCmd& cmd);

    RenderDevice& device;
    Rasterizer& rasterizer;
};
```

---

## 9.4 最終 flow

```text
Renderer::render(view)
  → collect renderables
  → build DrawMeshCmd
  → queue.push(cmd)

SoftwareBackend::execute(queue)
  → dispatch command
  → call rasterizer
```

這樣會分離：

```text
Renderer frontend:
  決定畫什麼

Backend:
  決定怎麼畫
```

---

## 9.5 對應大型系統

```text
Pixel-Renderer CommandQueue
  ↔ bgfx submit buffer
  ↔ Unreal RHI command list
  ↔ Vulkan command buffer
  ↔ Filament backend command stream
```

---

# 10. UI / Debug Overlay 設計方向

## 10.1 為什麼 UI 是重要主線

UI 不是旁支。它會讓你理解：

```text
input
state
layout
hit testing
draw command
clip rect
compositing
```

這能連到：

```text
Dear ImGui
microui
browser layout / paint / composite
React reconciliation
desktop compositor
Unity UI / Unreal UMG
```

---

## 10.2 第一版 UI 目標

先做 debug UI，不做完整 toolkit。

```cpp
ui_begin_frame();

if (ui_button("Wireframe")) {
    settings.wireframe = !settings.wireframe;
}

ui_slider("Light X", &light.x, -10.0f, 10.0f);
ui_checkbox("Show Depth", &settings.showDepth);

ui_end_frame();
```

---

## 10.3 UI CommandList

```cpp
struct UiDrawRectCmd {
    Rect rect;
    Color color;
};

struct UiDrawTextCmd {
    Vec2 position;
    const char* text;
    Color color;
};

struct UiSetClipCmd {
    Rect clip;
};

using UiCommand = std::variant<UiDrawRectCmd, UiDrawTextCmd, UiSetClipCmd>;
```

Render flow：

```text
scene_queue
  → render 3D scene

ui_queue
  → render UI overlay
```

---

## 10.4 第一版 UI 可控制的 renderer debug state

```text
wireframe on/off
show depth buffer
show normal
show UV
light direction
camera position
clear color
render mode: shaded / depth / normal / uv
```

這會讓 Pixel-Renderer 變成可觀察的學習工具。

---

# 11. Engine Mirror Demo 策略

## 11.1 為什麼需要 Engine Mirror Demo

如果只做 Pixel-Renderer，會有底層理解，但不一定知道商業引擎怎麼暴露功能。

如果只學 Unity / Unreal，容易只會操作，不知道底層原因。

所以策略是：

```text
Pixel-Renderer first-principles implementation
  +
Unity / Unreal mirror demo
```

---

## 11.2 同題雙解範例

### Topic 1：Phong Material

Pixel-Renderer：

```text
vertex shader
normal transform
N·L
specular
fragment shader
```

Unity / Unreal：

```text
Shader Graph 或 custom shader
Material parameters
runtime debug UI 調參
```

學到：

```text
shader formula
material abstraction
artist-facing parameters
engine workflow
```

---

### Topic 2：Shadow Mapping

Pixel-Renderer：

```text
shadow pass
depth map
light view projection
depth compare
bias
PCF
```

Unity / Unreal：

```text
directional light shadow
shadow resolution
cascade
bias
filtering
```

學到：

```text
shadow 原理
engine setting 的原因
performance / quality tradeoff
```

---

### Topic 3：Debug UI

Pixel-Renderer：

```text
immediate-mode UI
UiCommandList
DrawRect / DrawText
clip rect
```

Unity / Unreal：

```text
runtime debug panel
Editor Utility
UMG / UI Toolkit
```

學到：

```text
UI rendering 本質
tooling workflow
engine extension
```

---

# 12. Software GPU / SwiftShader 分支何時開始

## 12.1 現在可以做什麼

現在只看 SwiftShader architecture，不 deep dive source。

目前要理解：

```text
SwiftShader = software implementation of Vulkan graphics API
```

它包含：

```text
Vulkan API layer
Pipeline state
Shader execution
Renderer
Reactor
JIT
SIMD
```

---

## 12.2 什麼時候適合深入

等 Pixel-Renderer 有這些東西後：

```text
IShader
Material / MaterialInstance
CommandQueue
PipelineState
Texture sampling
Z-buffer
Fragment shader
```

再開始看：

```text
SwiftShader Renderer
VertexProcessor
SetupProcessor
PixelProcessor
SamplerCore
Reactor
Routine cache
```

---

## 12.3 中間橋：Toy ShaderVM

不要直接碰 SPIR-V。先做 toy bytecode。

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

這會讓你理解：

```text
shader 可以是一段 program
runtime 可以 interpret 或 compile 它
```

---

# 13. FPGA / Hardware GPU 分支何時開始

## 13.1 不要一開始做完整 GPU

先選最小硬體化單元：

```text
edge-function triangle rasterizer
```

---

## 13.2 Software golden model

Pixel-Renderer 可以輸出 reference：

```text
input triangle
edge function result
barycentric weights
depth value
expected framebuffer
```

FPGA / hardware model 輸出同樣資料，再比對。

---

## 13.3 第一個 FPGA slice

```text
Input:
  triangle vertices
  pixel coordinate

Output:
  inside / outside
  barycentric weights
```

這樣可以先學：

```text
fixed-point representation
pipeline latency
throughput
verification
```

---

# 14. 目錄結構建議

未來可以逐步整理成：

```text
Pixel-Renderer/
│
├─ core/
│   ├─ application.h/cpp
│   ├─ screen_manager.h/cpp
│   ├─ render_device.h/cpp
│   └─ input.h/cpp
│
├─ math/
│   ├─ vec2.h
│   ├─ vec3.h
│   ├─ vec4.h
│   ├─ mat4.h
│   ├─ transform.h
│   └─ camera_math.h
│
├─ render/
│   ├─ renderer.h/cpp
│   ├─ rasterizer.h/cpp
│   ├─ shader.h
│   ├─ material.h/cpp
│   ├─ mesh.h/cpp
│   ├─ texture.h/cpp
│   ├─ pipeline_state.h
│   └─ command_queue.h/cpp
│
├─ scene/
│   ├─ scene.h/cpp
│   ├─ camera.h/cpp
│   ├─ view.h/cpp
│   ├─ renderable.h
│   └─ transform.h
│
├─ backend/
│   ├─ software_backend.h/cpp
│   └─ backend_interface.h
│
├─ ui/
│   ├─ ui_context.h/cpp
│   ├─ ui_widgets.h/cpp
│   ├─ ui_command_list.h/cpp
│   └─ ui_renderer.h/cpp
│
├─ assets/
│   ├─ obj_loader.h/cpp
│   └─ tga_image.h/cpp
│
└─ docs/
    ├─ architecture.md
    ├─ math_notes.md
    ├─ pipeline_notes.md
    ├─ engine_mapping.md
    └─ experiments.md
```

短期不用一次搬完，但這是方向。

---

# 15. 建議學習順序

## 15.1 近期最重要

```text
1. 線性代數：field → vector space → basis → linear map → matrix → affine space → homogeneous coordinates
2. Triangle rasterization：edge function / barycentric / depth
3. Mat4 / MVP / View / Projection
4. IShader / VertexOutput / FragmentInput
5. mini-Filament skeleton
```

---

## 15.2 中期

```text
1. Material / MaterialInstance
2. Texture sampling
3. Perspective-correct interpolation
4. Phong / Blinn-Phong
5. Debug UI
6. CommandQueue
7. Unity / Unreal mirror demo
```

---

## 15.3 後期

```text
1. Shadow mapping
2. Normal mapping / TBN
3. Render sorting / batching
4. PipelineState
5. ShaderVM
6. SwiftShader architecture deep dive
7. FPGA edge rasterizer slice
8. RenderDoc / GPU profiling
```

---

# 16. 近期具體執行計畫

## Week 1：整理 math + triangle pipeline

輸出：

```text
Vec2 / Vec3 / Vec4
Mat4 初版
DrawTriangle + barycentric
Z-buffer
```

學習：

```text
vector space
basis
linear combination
affine combination
barycentric coordinates
```

---

## Week 2：MVP pipeline

輸出：

```text
Model matrix
View matrix / LookAt
Projection matrix
Perspective divide
Viewport transform
```

學習：

```text
linear map
affine transform
homogeneous coordinates
projective transform
```

---

## Week 3：Shader stage data

輸出：

```text
VertexInput
VertexOutput
FragmentInput
IShader
ShaderContext
FlatShader
ColorInterpolationShader
```

學習：

```text
stage input/output
varying interpolation
clip space w
perspective-correct interpolation 前置概念
```

---

## Week 4：mini-Filament skeleton

輸出：

```text
Scene
View
Camera
Mesh
Material
MaterialInstance
Renderer::render(view)
```

學習：

```text
renderer API design
scene-oriented rendering
commercial engine mapping
```

---

# 17. 每完成一個功能要寫的筆記格式

每個功能建議都寫成這種格式：

```md
# Topic: Perspective-Correct Interpolation

## 1. 問題
為什麼 screen-space barycentric 直接插 UV 會錯？

## 2. Naive 方法
直接用 αu0 + βu1 + γu2。

## 3. 失敗原因
projection 後 screen space 不是原本 3D 空間的 affine transform。

## 4. 數學結構
homogeneous coordinates, 1/w, rational-linear interpolation。

## 5. Pixel-Renderer 實作
程式碼與資料流。

## 6. Engine 對應
Unity / Unreal / GPU pipeline 都自動做 perspective-correct interpolation。

## 7. Debug 方法
畫 checker texture，觀察 perspective distortion。

## 8. 延伸
mipmapping, derivatives, texture LOD。
```

這樣每個功能都會同時累積：

```text
數學理解
工程實作
引擎對照
debug 方法
未來延伸
```

---

# 18. 目前最應該避免的陷阱

## 18.1 只做架構，不做 pipeline

如果太早寫 Engine / Scene / Material，但 triangle / z-buffer / MVP 不穩，架構會變空殼。

---

## 18.2 只做底層，不整理抽象

如果一直只寫 DrawTriangle / DrawLine，之後會很難接 Filament / Unity / Unreal 的概念。

---

## 18.3 太早讀大型 source

Filament、SwiftShader、Unreal 都很大。現在應該讀概念與 public API，不要一開始 deep dive。

---

## 18.4 太早做完整 UI framework

先做 debug UI，不要一開始做完整 layout engine。

---

## 18.5 太早追求效能

現在 priority：

```text
正確性 > 可觀察性 > 架構清楚 > 效能
```

等 pipeline 清楚後，再做：

```text
bounding box optimization
tile rasterization
SIMD
multithreading
cache locality
```

---

# 19. 最終收斂

目前 Pixel-Renderer 應該往這個方向長：

```text
first-principles software renderer
  → mini rendering engine skeleton
  → material / shader system
  → command queue / backend separation
  → debug UI / tooling
  → commercial engine mirror demo
  → optional software GPU / FPGA branches
```

最重要的是：

> 每一步都要知道自己正在學哪個底層問題，也要知道這個問題在大型引擎裡會變成什麼抽象。

目前下一個最穩定的路徑：

```text
1. 線性代數主幹接到 Mat4 / MVP
2. 完成 triangle + Z-buffer
3. 定義 shader stage data
4. 建立 Scene / View / Camera / Renderer
5. 建立 Material / MaterialInstance
6. 再做 CommandQueue
7. 再做 Debug UI
8. 再做 Unity / Unreal mirror demo
```

這樣 Pixel-Renderer 就會不只是「我會畫三角形」，而是逐步變成一個能支撐 Rendering Systems 思考的核心專案。

