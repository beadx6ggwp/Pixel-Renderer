# Pixel-Renderer 隱性主幹補強文件

日期：2026-05-22  
定位：補足 Pixel-Renderer 長期可維護、可除錯、可擴充、可對照商業引擎的工程支柱  
適用範圍：Pixel-Renderer / Rendering Systems Lab / Engine Mirror Demo

---

## 0. 這份文件的目的

前一份 roadmap 已經整理了 Pixel-Renderer 的主要發展方向：

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

但要讓 Pixel-Renderer 真正長期走下去，只知道功能順序還不夠。  
還需要一組「隱性主幹」。

這些主幹包括：

```text
1. Rendering conventions
2. Math-to-renderer mapping
3. Debug visualization
4. Testing / golden images
5. Material / Scene / Renderer data boundary
6. Resource lifetime
7. Profiling metrics
8. Commercial engine mapping
9. Project narrative
10. Documentation structure
```

這些東西不一定直接產生漂亮畫面，但會決定專案能不能從 toy renderer 變成真正可解釋、可維護、可擴充的 rendering systems lab。

---

# 1. Rendering Conventions：renderer 的憲法

## 1.1 為什麼 convention 很重要

很多 renderer bug 不是演算法不會，而是 convention 沒有統一。

例如：

```text
同樣是 projection matrix：
  有些系統用 right-handed
  有些系統用 left-handed
  有些 NDC z 是 [-1, 1]
  有些 NDC z 是 [0, 1]
  有些 matrix 用 column-major
  有些用 row-major
  有些 vector 放右邊
  有些 vector 放左邊
```

如果一開始沒有明確規定，後面會在這些功能裡反覆遇到奇怪 bug：

```text
MVP transform
backface culling
depth test
shadow mapping
normal mapping
screen-space UI
clip space
viewport transform
```

所以 Pixel-Renderer 應該早期建立一份：

```text
docs/rendering_conventions.md
```

它的地位類似 renderer 的「憲法」。

---

## 1.2 Coordinate Spaces

必須明確定義每個 space。

```text
Object Space / Local Space
  Mesh 原始頂點所在的空間。

World Space
  物件經過 model transform 後所在的全域場景空間。

View Space / Camera Space
  以 camera 為原點、camera 方向為基準的空間。

Clip Space
  Projection matrix 輸出後的 homogeneous space，座標為 (x, y, z, w)。

NDC
  Normalized Device Coordinates，clip space 做 perspective divide 後得到。

Screen Space
  對應 framebuffer / pixel coordinate 的空間。
```

建議在程式命名上也保留空間資訊：

```cpp
Vec3 positionOS;  // object space
Vec3 positionWS;  // world space
Vec3 positionVS;  // view space
Vec4 positionCS;  // clip space
Vec3 positionNDC;
Vec2 positionSS;  // screen space
```

這樣會比單純寫：

```cpp
Vec3 position;
```

清楚很多。

---

## 1.3 Handedness

必須決定：

```text
Pixel-Renderer 使用 right-handed 還是 left-handed coordinate system？
```

常見差異：

```text
OpenGL 傳統習慣：
  right-handed view space
  camera looks down -Z
  NDC z range = [-1, 1]

Direct3D / Vulkan 常見習慣：
  常使用 z range = [0, 1]
  handedness 可由 projection / view convention 決定
```

對 Pixel-Renderer 建議：

```text
短期：
  選一套最容易理解的 convention，並固定下來。

推薦：
  right-handed world/view space
  camera looks toward -Z
  NDC x/y in [-1, 1]
  NDC z 可先採 [0, 1] 或 [-1, 1]，但必須明確寫下。
```

重點不是哪一套絕對正確，而是：

> 整個 renderer 只能有一套明確 convention。

---

## 1.4 Matrix Convention

必須決定：

```text
Matrix memory layout:
  row-major or column-major?

Vector convention:
  column vector or row vector?

Transform order:
  clip = Projection * View * Model * position
  還是
  clip = position * Model * View * Projection?
```

建議採用一套並寫死。

例如：

```text
Pixel-Renderer Convention:

- 使用 column vector
- vector 放在右邊
- matrix multiplication order:

  positionCS = Projection * View * Model * positionOS

- transform composition:

  MVP = Projection * View * Model
```

即：

```cpp
Vec4 positionCS = projection * view * model * Vec4(positionOS, 1.0f);
```

如果內部 memory layout 是 row-major，也不等於數學上一定是 row vector convention。  
必須明確區分：

```text
memory layout 是資料怎麼放在 memory
math convention 是矩陣乘法語意
```

這點很重要，因為 C++ array layout 和 shader / math notation 常常被混淆。

---

## 1.5 Depth Convention

必須決定：

```text
Depth range:
  [0, 1] or [-1, 1]?

Depth compare:
  smaller is nearer?
  larger is nearer?

Clear depth value:
  1.0 or 0.0?

Future Reverse-Z:
  是否保留？
```

傳統 depth：

```text
near → 0
far  → 1
depth test: newDepth < oldDepth
clear depth: 1.0
```

Reverse-Z：

```text
near → 1
far  → 0
depth test: newDepth > oldDepth
clear depth: 0.0
```

建議：

```text
短期先用傳統 depth：
  near = 0
  far = 1
  depth test = less
  clear depth = 1.0

文件中註記：
  未來可加入 Reverse-Z 作為進階章節。
```

---

## 1.6 Screen Space Convention

必須決定：

```text
screen origin 在左上還是左下？
y 軸往下還是往上？
pixel center 是整數座標，還是 x + 0.5 / y + 0.5？
```

Win32 DIB / 一般 framebuffer 通常較自然用：

```text
origin = top-left
x increases right
y increases downward
```

但 NDC 通常是：

```text
x increases right
y increases upward
```

所以 viewport transform 必須處理 y flip。

典型 mapping：

```text
screenX = (ndcX * 0.5 + 0.5) * width
screenY = (1.0 - (ndcY * 0.5 + 0.5)) * height
```

如果使用 pixel center rule，則 rasterization 時應該測：

```text
sample position = (x + 0.5, y + 0.5)
```

---

## 1.7 Triangle Convention

必須決定：

```text
front face 是 clockwise 還是 counter-clockwise？
edge function 正負號如何定義？
shared edge 如何處理？
```

建議文件中明確寫：

```text
- 使用 counter-clockwise 作為 front face
- edge function > 0 表示在邊的某一側
- triangle coverage 使用 pixel center sample
- backface culling 初期可以先關閉
- 之後加入 culling 時再使用 winding convention
```

Edge rule 之後會牽涉 shared edge 是否重複畫 pixel。  
短期不一定要實作 top-left rule，但要知道這是未來會遇到的問題。

---

## 1.8 建議文件模板

`docs/rendering_conventions.md` 可以長這樣：

```md
# Rendering Conventions

## Coordinate Spaces

Object Space → World Space → View Space → Clip Space → NDC → Screen Space

## Handedness

- World/View space:
- Camera forward:
- NDC z range:

## Matrix Convention

- Vector convention:
- Transform order:
- Memory layout:
- Example:

## Depth Convention

- Depth range:
- Near value:
- Far value:
- Compare function:
- Clear value:

## Screen Space

- Origin:
- X direction:
- Y direction:
- Pixel center:

## Triangle Convention

- Front face:
- Edge function sign:
- Coverage rule:
- Backface culling:

## Notes

Known differences from OpenGL / Direct3D / Vulkan / Unity / Unreal.
```

---

# 2. Math-to-Renderer Mapping：不要只學數學，也要知道它接到哪裡

## 2.1 數學結構與 graphics representation 要分開

你目前從 field、vector space 開始學線性代數，這是很好的路線。

但 graphics 裡常常把不同數學對象都塞進同一個型別：

```cpp
Vec3 position;
Vec3 normal;
Vec3 direction;
Vec3 color;
Vec2 uv;
```

它們型別相似，但語意不同。

---

## 2.2 不同資料的數學語意

| 資料 | 常見型別 | 數學語意 | 注意事項 |
|---|---|---|---|
| position | `Vec3` | point in affine space | 會受 translation 影響 |
| direction | `Vec3` | vector | 不受 translation 影響 |
| normal | `Vec3` | normal vector / dual-like object | non-uniform scale 下不能直接乘 model matrix |
| color | `Vec3` / `Color` | signal / radiometric quantity | 不是幾何 vector |
| uv | `Vec2` | parameter domain coordinate | 不是 3D 空間 |
| tangent | `Vec3` | surface tangent direction | TBN basis 一部分 |
| depth | `float` | projected depth | 依 depth convention 解讀 |
| barycentric | `Vec3` | affine coordinates | 係數和應為 1 |

---

## 2.3 Point vs Vector

數學上，point 和 vector 不一樣。

```text
Point + Vector = Point
Point - Point = Vector
Vector + Vector = Vector
Point + Point = 通常沒有幾何意義
```

但 C++ graphics code 常常都用 `Vec3` 表示。

短期可以接受，但文件要標明：

```cpp
Vec3 positionOS; // semantically point
Vec3 normalOS;   // semantically direction / normal
Vec3 lightDirWS; // semantically direction
```

如果之後想更嚴謹，可以引入：

```cpp
struct Point3 { float x, y, z; };
struct Vec3   { float x, y, z; };
```

但早期不用過度工程化。

---

## 2.4 Normal Transform 是重要坑

Naive 方法：

```cpp
normalWS = model * normalOS;
```

這在 pure rotation 或 uniform scale 時大致可以。

但如果 model 有 non-uniform scale，會錯。

正確概念：

```text
normal 要保持和 surface tangent 垂直。
non-uniform scale 會改變 tangent space。
所以 normal 需要使用 inverse transpose matrix。
```

公式：

```text
normalMatrix = transpose(inverse(mat3(model)))
normalWS = normalize(normalMatrix * normalOS)
```

這是線性代數在 renderer 中非常實際的例子。

應該列入未來 `docs/math_to_renderer.md` 的重點。

---

## 2.5 Homogeneous Coordinates 的語意

`Vec4.w` 不是只是多一個 float，而是語意標籤。

```text
Point:
  (x, y, z, 1)

Direction:
  (x, y, z, 0)

Clip-space position:
  (x, y, z, w)
```

Translation matrix 對 point 有效，對 direction 無效：

```text
[ R  t ] [p] = R p + t
[ 0  1 ] [1]

[ R  t ] [v] = R v
[ 0  1 ] [0]
```

這也解釋了：

```text
為什麼 normal / direction 不該被 translation 影響。
```

---

## 2.6 Barycentric Coordinates 是 affine coordinates

Triangle 三頂點 A, B, C，點 P 可表示：

```text
P = αA + βB + γC
α + β + γ = 1
```

若：

```text
α >= 0
β >= 0
γ >= 0
```

則 P 在 triangle 內部或邊界。

這不是一般 linear combination，而是 affine combination。

在 Pixel-Renderer 中，barycentric coordinates 可以用於：

```text
inside test
color interpolation
depth interpolation
uv interpolation
normal interpolation
world position interpolation
```

但要注意：

```text
screen-space barycentric 直接插值 UV 會產生 perspective distortion。
```

所以需要 perspective-correct interpolation。

---

## 2.7 Projection 不是一般 affine transform

Perspective projection 的核心是：

```text
把 3D point 映射到 clip space，保留 w。
之後做 perspective divide：x/w, y/w, z/w。
```

所以不要太早丟掉 `w`。

Renderer pipeline 中應該保存：

```cpp
VertexOutput {
    Vec4 positionCS;
    float invW;
    ...
};
```

因為後面需要：

```text
1/w trick
perspective-correct interpolation
depth reconstruction
clip testing
```

---

## 2.8 建議文件模板

`docs/math_to_renderer.md` 可以長這樣：

```md
# Math to Renderer Mapping

## Field → float

- Math field vs floating-point approximation
- Precision issues
- Depth precision

## Vector Space → directions

- direction
- normal
- tangent
- velocity

## Affine Space → positions

- point vs vector
- model transform

## Basis → coordinate space

- object / world / view / clip / NDC / screen

## Linear Map → rotation / scale

- Mat4
- transform composition

## Homogeneous Coordinates

- point w=1
- direction w=0
- clip-space w

## Barycentric Coordinates

- affine combination
- inside test
- interpolation

## Important Bugs

- normal matrix
- perspective-correct interpolation
- precision
```

---

# 3. Debug Visualization：把 renderer 變成學習儀器

## 3.1 Debug view 不是附加功能

Software renderer 最大優勢之一是：

> 你可以把每個中間結果畫出來。

如果沒有 debug visualization，你只能看到最終畫面。  
一旦錯了，很難知道是：

```text
matrix 錯
projection 錯
depth 錯
rasterization 錯
normal 錯
UV 錯
shader 錯
```

所以 Debug View 應該是核心功能。

---

## 3.2 建議 RenderMode

```cpp
enum class RenderMode {
    Shaded,
    Wireframe,
    Depth,
    Normal,
    UV,
    Barycentric,
    Overdraw,
    ClipSpaceW,
    TriangleID,
    FrontBackFace
};
```

---

## 3.3 各 Debug View 的用途

| Debug View | 顯示內容 | 幫助抓的 bug |
|---|---|---|
| `Shaded` | 正常 shading | 最終效果 |
| `Wireframe` | triangle edges | viewport transform、mesh topology |
| `Depth` | depth buffer 灰階 | z-buffer、projection、near/far |
| `Normal` | normal 映射成 RGB | normal transform、winding、TBN |
| `UV` | uv 映射成顏色 | UV interpolation、perspective-correct |
| `Barycentric` | αβγ → RGB | triangle coverage、edge function |
| `Overdraw` | 每個 pixel 被寫幾次 | fill-rate、sorting、UI 疊加 |
| `ClipSpaceW` | w 值視覺化 | projection、perspective divide |
| `TriangleID` | 每個 triangle 不同顏色 | primitive assembly、index buffer |
| `FrontBackFace` | front/back face 顏色不同 | winding、culling |

---

## 3.4 Debug UI 控制項

Debug UI 最小可以提供：

```text
Render mode dropdown
Wireframe toggle
Depth view toggle
Show normals toggle
Light direction slider
Camera position
Near / far plane
Projection FOV
Clear color
Backface culling toggle
Perspective-correct interpolation toggle
```

這些控制項會讓 Pixel-Renderer 成為互動式學習工具。

---

## 3.5 Overdraw Visualization

Overdraw 是很好的性能直覺工具。

可以維護一個：

```cpp
std::vector<int> overdrawBuffer;
```

每次 fragment 通過 coverage test 時增加：

```cpp
overdrawBuffer[pixelIndex]++;
```

然後顯示成 heatmap。

早期不需要精美 color mapping，簡單灰階即可：

```text
0 = black
1 = dark gray
2 = brighter
3+ = white
```

這能讓你理解：

```text
draw order
fill-rate
transparent object
UI overlay
early-z 的價值
```

---

## 3.6 建議文件模板

`docs/debug_visualization.md`：

```md
# Debug Visualization

## Purpose

Why debug views are core to Pixel-Renderer.

## Render Modes

- Shaded
- Wireframe
- Depth
- Normal
- UV
- Barycentric
- Overdraw
- ClipSpaceW

## Implementation Notes

How each render mode maps internal data to color.

## Bugs Each View Can Catch

Table of debug view → bug category.

## UI Controls

List of runtime toggles and sliders.

## Future Extensions

- RenderDoc-like frame capture
- per-pixel inspection
- command list viewer
```

---

# 4. Testing Strategy：不要只靠「看起來對」

## 4.1 為什麼 renderer 需要測試

Graphics code 很容易出現：

```text
畫面看起來差不多，但其實 convention 錯
某個角度剛好沒出 bug
一改 projection，shadow 壞掉
一改 interpolation，texture 扭曲
```

所以測試要早點建立。

---

## 4.2 Unit Tests

先測純數學與小函式。

### Math tests

```text
Vec3 addition
Vec3 dot
Vec3 cross
Vec3 normalization
Mat4 identity
Mat4 multiplication
Mat4 translation
Mat4 rotation
Mat4 perspective
LookAt matrix
```

### Raster tests

```text
edge function sign
barycentric sum
inside triangle test
depth compare
viewport transform
```

### Shader tests

```text
FlatShader output
ColorInterpolationShader output
UV interpolation
normal transform
```

---

## 4.3 Golden Image Tests

固定輸入場景，輸出圖片，和 reference image 比較。

測試案例：

```text
test_001_single_triangle.png
test_002_rgb_barycentric_triangle.png
test_003_depth_overlap.png
test_004_perspective_checker.png
test_005_backface_culling.png
test_006_normal_debug_view.png
test_007_uv_debug_view.png
```

比較方式可以分階段：

```text
初期：
  手動看圖

中期：
  pixel-by-pixel exact comparison

後期：
  tolerance-based comparison
  diff image output
```

浮點運算可能造成微小差異，所以 tolerance-based 比較比較實際。

---

## 4.4 Invariant Tests

Invariant tests 檢查應該永遠成立的條件。

例如：

```text
barycentric α + β + γ ≈ 1
inside pixel requires α, β, γ >= 0
normalized normal length ≈ 1
depth buffer update only happens when depth test passes
clip-space w should not be zero before divide
viewport output should be inside screen for visible NDC
```

可以在 debug build 加上 assert 或 debug logging。

---

## 4.5 Regression Tests

每修一個 bug，都新增一個最小測試。

例如：

```text
Bug:
  UV perspective interpolation was wrong.

Add:
  test_perspective_checker_texture.png
```

這樣專案不會每次重構又回到同樣 bug。

---

## 4.6 建議文件模板

`docs/testing_strategy.md`：

```md
# Testing Strategy

## Goals

- correctness
- regression prevention
- convention validation
- visual debugging

## Unit Tests

Math, rasterizer, shader, material.

## Golden Image Tests

List of fixed scenes and expected outputs.

## Invariant Tests

Runtime assertions and debug checks.

## Regression Tests

Every fixed bug should become a test.

## Future

- CI
- image diff
- tolerance comparison
- debug frame capture
```

---

# 5. Data Boundary：Material / Scene / Renderer 的邊界

## 5.1 為什麼邊界重要

如果所有東西都直接互相呼叫，短期方便，長期會亂。

Naive 寫法：

```cpp
rasterizer.DrawTriangle(v0, v1, v2, shader);
```

問題：

```text
Scene 不存在
Material 不存在
Renderer 不知道自己負責什麼
Shader 和 object parameters 混在一起
之後無法接 CommandQueue
無法對照 Unity / Unreal / Filament
```

---

## 5.2 建議邊界

### Scene

負責：

```text
存放要被 render 的 renderable objects
```

不負責：

```text
不做 rasterization
不做 shading
不做 depth test
```

### View

負責：

```text
指定 scene + camera + viewport + render settings
```

不負責：

```text
不擁有 scene
不執行 draw
```

### Renderer

負責：

```text
根據 View 收集 draw intent
建立 render command 或直接呼叫 backend
```

不負責：

```text
不擁有 mesh data
不負責 window
不直接處理 OS present
```

### Material

負責：

```text
共享的 rendering behavior
shader + raster state + parameter layout
```

### MaterialInstance

負責：

```text
per-object parameter values
baseColor / texture / roughness / custom params
```

### Rasterizer

負責：

```text
triangle coverage
interpolation
per-fragment execution hook
depth test request
```

### RenderDevice

負責：

```text
framebuffer
depth buffer
SetPixel
Clear
```

---

## 5.3 對照商業引擎

```text
Pixel-Renderer Scene
  ↔ Unity Scene / Unreal World / Filament Scene

Renderable
  ↔ Unity MeshRenderer / Unreal StaticMeshComponent / Filament Renderable

Material
  ↔ Unity Shader+Material / Unreal Material / Filament Material

MaterialInstance
  ↔ Unity MaterialPropertyBlock or Material instance / Unreal Material Instance / Filament MaterialInstance

Renderer
  ↔ Filament Renderer / Unreal renderer module / Unity render pipeline

CommandQueue
  ↔ Unreal RHI command list / Vulkan command buffer / bgfx submit buffer
```

---

## 5.4 建議文件模板

`docs/data_boundaries.md`：

```md
# Data Boundaries

## Scene

Owns / references renderables.

## View

Binds scene and camera.

## Renderer

Builds draw intent.

## Material

Shared rendering behavior.

## MaterialInstance

Per-object parameters.

## Rasterizer

Triangle-level execution.

## RenderDevice

Framebuffer and depth buffer.

## Mapping to Engines

Unity / Unreal / Filament correspondence.
```

---

# 6. Resource Lifetime：不要太晚才想 ownership

## 6.1 為什麼 lifetime 會變重要

早期你可能會寫：

```cpp
struct Renderable {
    Mesh* mesh;
    MaterialInstance* material;
};
```

這很簡單，但之後會遇到：

```text
Mesh 誰擁有？
Material 誰釋放？
Texture 誰管理？
Scene 裡能不能存 raw pointer？
CommandQueue 裡能不能存 pointer？
如果物件被刪掉，但 command queue 還有它的 pointer，怎麼辦？
```

這是所有 engine 都會遇到的問題。

---

## 6.2 分階段策略

### Stage 1：Raw pointer for simplicity

短期：

```cpp
Mesh* mesh;
MaterialInstance* material;
```

條件：

```text
所有物件生命週期由 main / demo 控制
CommandQueue 當幀立刻執行
不跨幀保存 pointer
```

文件要明確寫：

```text
這是簡化，不是最終設計。
```

---

### Stage 2：Handle-based resources

中期：

```cpp
struct MeshHandle {
    uint32_t id;
};

struct MaterialHandle {
    uint32_t id;
};
```

由 ResourceManager 管理：

```cpp
class ResourceManager {
public:
    MeshHandle createMesh(...);
    Mesh& getMesh(MeshHandle handle);
    void destroyMesh(MeshHandle handle);
};
```

---

### Stage 3：Generational handle

後期防止 stale handle：

```cpp
struct Handle {
    uint32_t index;
    uint32_t generation;
};
```

如果舊 handle 指向已刪除資源，可以偵測出來。

---

## 6.3 CommandQueue 與 lifetime

CommandQueue 最好不要存會跨幀失效的 raw pointer。

短期可以：

```text
queue 當幀建立，當幀執行，當幀清空
```

中期應該：

```text
Command 裡存 handle 或資料快照
```

例如：

```cpp
struct DrawMeshCmd {
    MeshHandle mesh;
    MaterialInstanceHandle material;
    Mat4 model;
};
```

或對某些資料直接 snapshot：

```cpp
struct DrawTriangleCmd {
    Vertex vertices[3];
    MaterialParams params;
};
```

tradeoff：

```text
handle:
  command 小，但需要 backend 查資源

snapshot:
  command 大，但執行時不受外部 lifetime 影響
```

---

## 6.4 建議文件模板

`docs/resource_lifetime.md`：

```md
# Resource Lifetime

## Current Stage

Raw pointers for simplicity.

## Ownership Rules

Who owns Mesh, Material, Texture, Scene, Renderable?

## CommandQueue Lifetime

Commands are built and consumed in the same frame.

## Future Handle System

MeshHandle, TextureHandle, MaterialHandle.

## Future Generational Handles

Prevent stale references.

## Tradeoffs

Pointer vs handle vs snapshot.
```

---

# 7. Profiling Metrics：先不追效能，但要建立觀測習慣

## 7.1 正確優先順序

目前不要太早追求效能。優先順序應該是：

```text
Correctness
  → Observability
  → Clean data flow
  → Architecture
  → Performance
```

但你可以很早就開始收集 metrics。

---

## 7.2 建議收集的 Metrics

```cpp
struct RenderStats {
    int drawCalls = 0;
    int trianglesSubmitted = 0;
    int trianglesRasterized = 0;
    int fragmentsGenerated = 0;
    int fragmentsDepthPassed = 0;
    int pixelsWritten = 0;
    int textureSamples = 0;
    int commandsExecuted = 0;
    float frameTimeMs = 0.0f;
};
```

---

## 7.3 每個 metric 的意義

| Metric | 意義 | 對應大型引擎概念 |
|---|---|---|
| drawCalls | draw command 數量 | CPU submission cost |
| trianglesSubmitted | 輸入 triangle 數 | geometry load |
| trianglesRasterized | 通過 culling / clipping 的 triangle | visibility |
| fragmentsGenerated | coverage 產生的 fragments | fill-rate |
| fragmentsDepthPassed | 通過 depth test 的 fragments | overdraw / early-z |
| pixelsWritten | 實際寫入 color buffer | framebuffer bandwidth |
| textureSamples | texture sample 次數 | texture bandwidth |
| commandsExecuted | backend command 數 | command buffer |
| frameTimeMs | frame time | performance budget |

---

## 7.4 Debug UI 顯示範例

```text
Frame: 16.7 ms
Draw Calls: 12
Triangles: 240
Fragments: 82,340
Depth Passed: 41,200
Overdraw Ratio: 2.0x
Texture Samples: 41,200
Commands: 18
```

Overdraw ratio 可以先簡化：

```text
overdraw = fragmentsGenerated / pixelsWritten
```

雖然不完全精準，但足夠建立直覺。

---

## 7.5 和商業 profiling 工具的對照

這會幫你理解：

```text
RenderDoc
PIX
Nsight
Unity Profiler
Unreal Insights
```

這些工具到底在看什麼。

Pixel-Renderer 的 `RenderStats` 是簡化版。

---

## 7.6 建議文件模板

`docs/profiling_metrics.md`：

```md
# Profiling Metrics

## Goal

Build performance intuition before optimization.

## Metrics

- draw calls
- triangle count
- fragment count
- depth pass count
- overdraw
- texture samples
- frame time

## Debug UI

How to display metrics.

## Mapping to Real Engines

RenderDoc / PIX / Nsight / Unity Profiler / Unreal Insights.

## Future Optimizations

- culling
- sorting
- batching
- early-z
- tiling
- SIMD
```

---

# 8. Commercial Engine Mapping：每個功能都要對照大型引擎

## 8.1 為什麼要制度化對照

你不只想從零寫 renderer，也想能靈活使用商業引擎。

所以每做一個 Pixel-Renderer 功能，都應該問：

```text
這個東西在 Unity / Unreal / Filament 裡叫什麼？
使用者看到的是什麼介面？
engine 內部大概怎麼包？
artist / engineer 會怎麼調？
performance tradeoff 是什麼？
對應哪種職位能力？
```

---

## 8.2 對照表範例

| Pixel-Renderer | Filament | Unity | Unreal | 對應職位 |
|---|---|---|---|---|
| `Material` | `Material` | Shader + Material | Material | Graphics / TA |
| `MaterialInstance` | `MaterialInstance` | Material instance / MaterialPropertyBlock | Material Instance | TA / Tools |
| `Scene` | `Scene` | Scene | World / Level | Client / Engine |
| `View` | `View` | Camera + Render Pipeline context | ViewFamily / View | Graphics |
| `Renderer` | `Renderer` | SRP Renderer | Renderer module | Graphics / Engine |
| `CommandQueue` | backend command stream | CommandBuffer | RHI Command List | Engine / Graphics |
| `RenderDevice` | backend target | RenderTexture / backbuffer | Render Target | Graphics |
| `Debug UI` | debug tooling | IMGUI / UI Toolkit | UMG / Slate / Editor Utility | Tools / TA / Client |

---

## 8.3 每個功能的對照筆記格式

```md
# Engine Mapping: MaterialInstance

## Pixel-Renderer

MaterialInstance stores per-object material parameters.

## Filament

MaterialInstance is created from Material and parameters are set through APIs.

## Unity

Material / MaterialPropertyBlock provide similar concepts.

## Unreal

Material Instance allows overriding parameters from a parent Material.

## Workflow

Artist or engineer adjusts exposed parameters.

## Performance Notes

Parameter changes may affect batching, shader variants, or constant buffer updates.

## Related Roles

- Technical Artist
- Graphics Engineer
- Tools Engineer
```

---

# 9. Project Narrative：讓作品能被理解

## 9.1 為什麼作品敘事重要

未來申請研究所、找工作、跟老師或工程師討論時，不能只說：

```text
我寫了一個 software renderer。
```

這太容易被理解成小玩具。

更好的說法是：

> 我用 Pixel-Renderer 作為 Rendering Systems Lab。它從 `SetPixel()` 開始，逐步實作 rasterization、MVP、shader、material、command queue、debug UI。每個功能都對照商業引擎中的 abstraction，例如 Filament 的 MaterialInstance、Unreal 的 RHI、Unity 的 material workflow。這讓我能同時理解底層 rendering principle 與現代引擎設計。

---

## 9.2 作品 README 的建議結構

```md
# Pixel-Renderer

A first-principles software renderer and Rendering Systems Lab.

## Motivation

Why build a renderer from scratch?

## Current Features

- framebuffer
- line drawing
- triangle rasterization
- barycentric coordinates
- Z-buffer
- MVP
- IShader
- Material / MaterialInstance
- Debug UI
- CommandQueue

## Architecture

Diagram of Application → Renderer → Rasterizer → RenderDevice.

## Learning Goals

- graphics pipeline
- renderer architecture
- UI/compositor
- software GPU
- commercial engine mapping

## Engine Mapping

How concepts correspond to Filament / Unity / Unreal.

## Roadmap

Short / mid / long-term.

## Debug Views

Depth, normal, UV, barycentric, overdraw.

## Tests

Unit tests and golden image tests.

## Notes

Links to docs.
```

---

## 9.3 對外介紹短版

```text
Pixel-Renderer is my first-principles Rendering Systems Lab.
It starts from a software framebuffer and gradually implements the GPU rendering pipeline by hand: triangle rasterization, barycentric interpolation, Z-buffer, MVP transforms, shader abstraction, materials, command queues, and debug UI.

The goal is not only to render images, but to understand how modern engines organize rendering concepts such as Scene, View, Camera, MaterialInstance, RenderCommand, and backend execution. Each feature is mapped to commercial engine concepts in Filament, Unity, Unreal, and related systems.
```

---

# 10. Documentation Structure：該補哪些文件

## 10.1 建議 docs 目錄

```text
docs/
│
├─ architecture.md
├─ rendering_conventions.md
├─ pipeline_flow.md
├─ math_to_renderer.md
├─ debug_visualization.md
├─ testing_strategy.md
├─ data_boundaries.md
├─ resource_lifetime.md
├─ profiling_metrics.md
├─ engine_mapping.md
├─ architecture_decisions.md
└─ experiments.md
```

---

## 10.2 每份文件的目的

| 文件 | 目的 |
|---|---|
| `architecture.md` | 整體模組與資料流 |
| `rendering_conventions.md` | coordinate / depth / matrix / triangle convention |
| `pipeline_flow.md` | vertex → raster → fragment → framebuffer |
| `math_to_renderer.md` | 線性代數如何接到 renderer |
| `debug_visualization.md` | debug view 與可觀察性 |
| `testing_strategy.md` | unit / golden image / invariant tests |
| `data_boundaries.md` | Scene / View / Material / Renderer 邊界 |
| `resource_lifetime.md` | pointer / handle / ownership |
| `profiling_metrics.md` | frame stats / overdraw / draw calls |
| `engine_mapping.md` | 對照 Filament / Unity / Unreal |
| `architecture_decisions.md` | 記錄重要設計決策 |
| `experiments.md` | 小實驗紀錄 |

---

# 11. Architecture Decision Records

## 11.1 為什麼需要 ADR

ADR = Architecture Decision Record。

它的作用是記錄：

```text
當初為什麼這樣設計？
當時有哪些選項？
選擇後的代價是什麼？
未來什麼情況可能改掉？
```

這對長期專案很重要，因為幾週後你可能忘記當初為什麼這樣寫。

---

## 11.2 ADR 範例：Material / MaterialInstance

```md
# ADR-001: Separate Material and MaterialInstance

## Status

Accepted

## Context

The initial renderer passed `IShader` directly into `DrawTriangle`.
This works for simple demos but mixes shared shader behavior with per-object parameters.

## Decision

Introduce:

- `Material`: shared rendering behavior, shader, raster state
- `MaterialInstance`: per-object parameter values

## Consequences

Positive:

- Closer to Filament / Unreal material model
- Allows many objects to share one material
- Per-object parameters become explicit

Negative:

- Slightly more indirection
- Requires lifetime management

## Future

Add `ParameterBlock`, texture bindings, and material reflection later.
```

---

## 11.3 建議先寫的 ADR

```text
ADR-001: Separate Material and MaterialInstance
ADR-002: Use column-vector MVP convention
ADR-003: Use top-left screen origin
ADR-004: Use traditional depth before Reverse-Z
ADR-005: Start with raw pointers before ResourceHandle
ADR-006: Add View before CommandQueue
ADR-007: Use Debug UI as core observability layer
```

---

# 12. 近期最該補的三份文件

如果時間有限，優先補：

```text
1. docs/rendering_conventions.md
2. docs/pipeline_flow.md
3. docs/math_to_renderer.md
```

原因：

## 12.1 `rendering_conventions.md`

防止所有座標、矩陣、深度規約混亂。

## 12.2 `pipeline_flow.md`

讓你每次加功能都知道插在哪一段 pipeline。

## 12.3 `math_to_renderer.md`

把現在學的 field / vector space / linear map / affine space 接到實作，不讓線性代數變成孤立科目。

---

# 13. 近期執行順序

建議短期不是狂加新功能，而是：

```text
Step 1:
  寫 docs/rendering_conventions.md 初版

Step 2:
  寫 docs/pipeline_flow.md 初版

Step 3:
  寫 docs/math_to_renderer.md 初版

Step 4:
  在程式裡加入 RenderStats

Step 5:
  加入 Depth / Barycentric / UV debug view

Step 6:
  開始建立簡單 golden image output

Step 7:
  再回到 mini-Filament skeleton
```

這樣可以避免架構長歪。

---

# 14. 和 Pixel-Renderer Roadmap 的關係

這份文件補的是 roadmap 的基礎支撐。

```text
Roadmap 文件：
  告訴你要做哪些功能、順序如何。

本文件：
  告訴你每個功能背後需要固定哪些規約、測試、觀測、文件與敘事。
```

兩者關係：

```text
Pixel-Renderer Roadmap
  → 功能主線

Pixel-Renderer Hidden Spine
  → 工程主幹 / 可維護性 / 可解釋性
```

缺一不可。

---

# 15. 總結

你現在不缺更多方向。  
你更需要把 Pixel-Renderer 的「隱性主幹」建立起來。

最重要的九個支柱：

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
```

這些支柱會讓 Pixel-Renderer 從：

```text
我在學 graphics
```

變成：

```text
我在建立一個可解釋、可測試、可擴充、可對照商業引擎的 Rendering Systems Lab。
```

目前最值得立刻做的不是增加更多大功能，而是補三份基礎文件：

```text
docs/rendering_conventions.md
docs/pipeline_flow.md
docs/math_to_renderer.md
```

這三份文件會讓後面的 triangle、MVP、Material、CommandQueue、Debug UI、Unity/Unreal mirror demo 都更穩。
