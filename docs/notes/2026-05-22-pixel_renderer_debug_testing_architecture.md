# Pixel-Renderer Debug Visualization 與 Testing 架構規劃

日期：2026-05-22  
定位：Pixel-Renderer 的 runtime debug、bootstrap 檢查工具、自動化測試、golden image test 與工程目錄規劃  
適用範圍：Pixel-Renderer / Rendering Systems Lab / Software Renderer / mini-Filament-style renderer skeleton

---

## 0. 文件目的

這份文件整理 Pixel-Renderer 中 **Debug Visualization** 與 **Testing** 的完整設計。

核心問題是：

> 我需要 debug visualization 來檢查 MVP、LookAt、Perspective、Rasterizer、Depth、UV、Normal 等功能；  
> 但這些 debug visualization 又依賴 renderer 本身正確。  
> 那麼在 renderer 還不穩時，要怎麼檢查？

這是典型的 bootstrap problem。解法不是等 renderer 完全正確後再做 debug tool，而是建立分層工具：

```text
Unit Test / Known Case
  → Numeric Dump / Pipeline Trace
  → 2D Debug Draw
  → Stage-local Visualization
  → Full Debug Visualization
  → Golden Image Regression
```

也就是：

```text
不要一開始用完整 renderer 檢查完整 renderer。
先建立一個個「可信小島」，再用可信小島檢查下一層。
```

---

# 1. Debug Visualization 與 Testing 的本質差異

## 1.1 Debug Visualization 是 runtime microscope

Debug Visualization 的目的：

```text
畫面錯了，我要知道錯在哪一層。
```

它是開發中的觀察工具，用來把 renderer 中間狀態變成畫面。

例如：

```text
Depth View
Normal View
UV View
Barycentric View
Wireframe View
Overdraw View
ClipSpaceW View
```

它回答的是：

```text
現在 pipeline 內部資料看起來合不合理？
```

---

## 1.2 Testing 是 correctness guardrail

Testing 的目的：

```text
我改完 code，要知道以前對的東西有沒有壞掉。
```

它是自動化驗證工具，用來防止 regression。

例如：

```text
Vec3 dot / cross test
Mat4 translation test
LookAt known-case test
Viewport transform test
Barycentric test
Depth buffer test
Golden image test
```

它回答的是：

```text
這個功能是否符合預期？
```

---

## 1.3 兩者比較

| 項目 | Debug Visualization | Testing |
|---|---|---|
| 本質 | runtime microscope | correctness guardrail |
| 使用時機 | 開發中觀察 | 每次修改後驗證 |
| 判斷方式 | 人眼 / 互動觀察 | 自動 pass / fail |
| 適合問題 | 找出 bug 來源 | 防止 regression |
| 輸出 | 畫面、overlay、trace | log、assert、image diff |
| 例子 | Depth View | DepthBuffer unit test |
| 例子 | UV Checker | Perspective golden image |
| 例子 | Pipeline Trace | LookAt maps eye to origin |

兩者都需要。  
只有 debug view，重構時沒有保障。  
只有 tests，出 bug 時不容易理解錯在哪。

---

# 2. Bootstrap Problem：工具本身也依賴 renderer 怎麼辦？

## 2.1 問題描述

當 MVP、LookAt、Perspective、Viewport Transform 都還不穩時，你可能會想：

```text
我想用 Depth View / Wireframe / Normal View 檢查它們。
```

但問題是：

```text
Depth View 依賴 Projection 正確
Wireframe 依賴 Viewport Transform 正確
Normal View 依賴 Transform / Interpolation 正確
UV View 依賴 Perspective-correct Interpolation 正確
```

所以如果畫面錯，不知道是：

```text
debug view 錯
還是被 debug 的 pipeline 錯
```

這就是 bootstrap problem。

---

## 2.2 正確解法：不要一開始檢查整條 pipeline

完整 pipeline 是：

```text
Object Space
  → Model
  → World Space
  → View
  → View Space
  → Projection
  → Clip Space
  → Perspective Divide
  → NDC
  → Viewport Transform
  → Screen Space
  → Rasterization
  → Fragment
  → Framebuffer
```

如果直接看 final image，錯誤可能來自任何一段。

所以要改成：

```text
先每一段單獨驗證。
每段都用已知輸入 → 已知輸出。
```

---

## 2.3 Debug 工具的可信度階梯

```text
Level 0：Math Unit Tests
  完全不依賴 renderer。

Level 1：Numeric Pipeline Trace
  印出 OS / WS / VS / CS / NDC / SS。

Level 2：Screen-space Triangle Mode
  跳過 MVP，只測 rasterizer。

Level 3：NDC Triangle Mode
  只測 NDC → Screen。

Level 4：Orthographic Mode
  測 Model / View / Orthographic，不碰 perspective w。

Level 5：Perspective Mode
  測完整 MVP + perspective divide。

Level 6：Textured Perspective Mode
  測 perspective-correct interpolation。

Level 7：Full Debug Visualization
  Depth / Normal / UV / Barycentric / Overdraw 等完整 views。
```

核心原則：

```text
先建立可信小島，再用可信小島檢查下一層。
```

---

# 3. 可信小島：逐層建立可驗證基礎

## 3.1 可信小島 1：Math Unit Tests

這一層完全不靠畫面。

要先確認：

```text
Vec3
Vec4
Mat4
Dot / Cross
Normalize
Translation
Rotation
Scale
LookAt
Projection
Viewport Transform
```

### Translation 測試

```cpp
Mat4 T = Mat4::Translation(Vec3(3, 4, 5));

Vec4 p(1, 2, 3, 1); // point
Vec4 v(1, 2, 3, 0); // direction

T * p == Vec4(4, 6, 8, 1)
T * v == Vec4(1, 2, 3, 0)
```

這個測試驗證 homogeneous coordinate 的語意：

```text
w = 1 的 point 會被 translation 影響
w = 0 的 direction 不會被 translation 影響
```

### LookAt 測試

假設：

```text
eye    = (0, 0, 5)
target = (0, 0, 0)
up     = (0, 1, 0)
```

如果採用 right-handed camera looking down `-Z`：

```text
View * Vec4(eye, 1)    ≈ (0, 0, 0, 1)
View * Vec4(target, 1) ≈ (0, 0, -5, 1)
```

這比直接看畫面可靠。

### Viewport 測試

假設 framebuffer 是 `800 × 600`，screen origin 是左上角：

```text
NDC (-1,  1) → screen (0, 0)
NDC ( 0,  0) → screen (400, 300)
NDC ( 1, -1) → screen (800, 600)
```

這可以直接抓 y flip 或 screen convention 錯誤。

---

## 3.2 可信小島 2：Screen-space Rasterizer

在 MVP 還不穩之前，不要用 3D triangle 測 rasterizer。

先提供：

```cpp
DrawTriangleScreenSpace(Vec2 a, Vec2 b, Vec2 c, Color color);
```

或更完整：

```cpp
struct ScreenVertex {
    Vec2 positionSS;
    float depth;
    Color color;
};

DrawTriangleScreenSpace(
    const ScreenVertex& a,
    const ScreenVertex& b,
    const ScreenVertex& c
);
```

這一層只測：

```text
Bounding Box
Edge Function
Barycentric Coordinates
Triangle Coverage
Color Write
Depth Test
```

不測：

```text
Model
View
Projection
Perspective Divide
Viewport Transform
```

適合搭配：

```text
Barycentric View
Wireframe View
TriangleID View
Depth View
```

---

## 3.3 可信小島 3：NDC Triangle Mode

下一步手動餵 NDC：

```text
A = (-0.5, -0.5, 0.5)
B = ( 0.5, -0.5, 0.5)
C = ( 0.0,  0.5, 0.5)
```

流程：

```text
Known NDC triangle
  → Viewport Transform
  → Screen-space Triangle
  → Rasterizer
```

這只測：

```text
NDC → Screen
screen origin
y flip
viewport mapping
rasterizer
```

不測：

```text
LookAt
Projection
MVP
```

---

## 3.4 可信小島 4：Orthographic Mode

Perspective 比 orthographic 多了 `w` 和 divide，debug 成本較高。

所以先做：

```text
Object Space
  → Model
  → View
  → Orthographic Projection
  → NDC
  → Screen
```

Orthographic projection 不會因距離造成縮放，因此比較容易驗證。

測試目標：

```text
Mat4
Model transform
LookAt
Camera basis
Viewport
```

---

## 3.5 可信小島 5：Perspective Mode

等前面可信後，再做完整 perspective：

```text
Object Space
  → Model
  → View
  → Perspective Projection
  → Clip Space
  → Perspective Divide
  → NDC
  → Screen
```

先不要用 cube 或 OBJ。  
用 canonical points：

```text
camera at origin
looking down -Z
near = 1
far = 10
fovY = 90°
aspect = 1
```

檢查：

```text
point (0, 0, -1)  → near center
point (0, 0, -10) → far center
point (1, 1, -1)  → near plane corner 附近
```

確認：

```text
clip.w 合理
NDC x/y 合理
NDC z 符合 depth convention
```

---

## 3.6 可信小島 6：Perspective-correct Interpolation

這時才加入 UV / texture。

Naive interpolation：

```text
uv = αuv0 + βuv1 + γuv2
```

Perspective-correct interpolation：

```text
uv =
  (α * uv0 / w0 + β * uv1 / w1 + γ * uv2 / w2)
  /
  (α / w0 + β / w1 + γ / w2)
```

用：

```text
Perspective Checker Quad
```

檢查是否有透視下的 texture distortion。

---

# 4. Pipeline Trace：比 Debug View 更早需要的工具

## 4.1 為什麼需要 Pipeline Trace

在 MVP / LookAt / Perspective 還不穩時，看 final image 不夠。  
你需要直接印出每個 stage 的數值。

Pipeline Trace 的目的：

```text
追蹤一個 vertex 從 Object Space 到 Screen Space 的完整數值變化。
```

---

## 4.2 traceVertex 設計

```cpp
struct PipelineTraceResult {
    Vec4 positionOS;
    Vec4 positionWS;
    Vec4 positionVS;
    Vec4 positionCS;
    Vec3 positionNDC;
    Vec2 positionSS;
};

PipelineTraceResult traceVertex(
    const Vertex& v,
    const Mat4& model,
    const Mat4& view,
    const Mat4& projection,
    const Viewport& viewport
);
```

實作邏輯：

```cpp
PipelineTraceResult traceVertex(
    const Vertex& v,
    const Mat4& model,
    const Mat4& view,
    const Mat4& projection,
    const Viewport& viewport
) {
    PipelineTraceResult out;

    out.positionOS = Vec4(v.position, 1.0f);
    out.positionWS = model * out.positionOS;
    out.positionVS = view * out.positionWS;
    out.positionCS = projection * out.positionVS;

    out.positionNDC = Vec3(
        out.positionCS.x / out.positionCS.w,
        out.positionCS.y / out.positionCS.w,
        out.positionCS.z / out.positionCS.w
    );

    out.positionSS = viewportTransform(out.positionNDC, viewport);

    return out;
}
```

---

## 4.3 Trace 輸出範例

```text
Vertex Trace
------------
OS  = (0.000, 0.000, 0.000, 1.000)
WS  = (0.000, 0.000, 0.000, 1.000)
VS  = (0.000, 0.000, -5.000, 1.000)
CS  = (0.000, 0.000, 4.810, 5.000)
NDC = (0.000, 0.000, 0.962)
SS  = (400.000, 300.000)
```

如果 camera 在 `(0, 0, 5)` 看向 origin，這個結果合理：

```text
origin 在 camera 前方 5 單位
最後落在畫面中心
```

---

## 4.4 traceTriangle

```cpp
struct TriangleTraceResult {
    PipelineTraceResult v0;
    PipelineTraceResult v1;
    PipelineTraceResult v2;
};

TriangleTraceResult traceTriangle(...);
```

用來檢查：

```text
三個頂點是否都在 expected screen region
triangle winding 是否反了
w 是否有負值或接近 0
NDC 是否超出 [-1, 1]
```

---

# 5. Debug Visualization 設計

## 5.1 RenderMode

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

## 5.2 DebugSettings

```cpp
struct DebugSettings {
    RenderMode renderMode = RenderMode::Shaded;

    bool showWireframeOverlay = false;
    bool enableDepthTest = true;
    bool enableBackfaceCulling = false;
    bool enablePerspectiveCorrect = true;
    bool showStats = true;
};
```

---

## 5.3 FragmentInput 應帶 debug-friendly data

```cpp
struct FragmentInput {
    Vec3 positionWS;
    Vec3 normalWS;
    Vec2 uv;
    Color color;

    float depth;
    float invW;

    Vec3 barycentric;
    uint32_t triangleID;
    bool frontFacing;
};
```

這樣 debug view 需要的資料比較容易取得。

---

## 5.4 Debug View 資料來源

| Debug View | 需要資料 |
|---|---|
| Depth | `depth` |
| Normal | `normalWS` |
| UV | `uv` |
| Barycentric | `barycentric` |
| Overdraw | `overdrawBuffer` |
| ClipSpaceW | `invW` or clip-space `w` |
| TriangleID | `triangleID` |
| FrontBackFace | `frontFacing` |

---

## 5.5 debugShade

```cpp
Color debugShade(
    const FragmentInput& in,
    RenderMode mode
) {
    switch (mode) {
    case RenderMode::Depth:
        return depthToColor(in.depth);

    case RenderMode::Normal:
        return normalToColor(in.normalWS);

    case RenderMode::UV:
        return uvToColor(in.uv);

    case RenderMode::Barycentric:
        return barycentricToColor(in.barycentric);

    case RenderMode::ClipSpaceW:
        return invWToColor(in.invW);

    case RenderMode::TriangleID:
        return triangleIDToColor(in.triangleID);

    case RenderMode::FrontBackFace:
        return in.frontFacing ? Color(0, 1, 0) : Color(1, 0, 0);

    case RenderMode::Shaded:
    default:
        return Color(1, 0, 1); // magenta indicates invalid debug path
    }
}
```

正常 shading 則呼叫 material shader：

```cpp
if (debug.renderMode == RenderMode::Shaded) {
    return material.shader->fragment(in, ctx);
} else {
    return debugShade(in, debug.renderMode);
}
```

---

# 6. 各 Debug View 詳細說明

## 6.1 Depth View

### 目的

檢查：

```text
Z-buffer
Projection matrix
near / far plane
depth range
depth compare direction
z-fighting
```

### 顯示方式

傳統 depth convention：

```text
near = 0
far  = 1
depth test = less
```

顯示近亮遠暗：

```cpp
Color depthToColor(float depth) {
    float v = clamp(1.0f - depth, 0.0f, 1.0f);
    return Color(v, v, v);
}
```

### 常見問題

| 現象 | 可能原因 |
|---|---|
| 全白 | depth 沒寫入、depth range 錯、far 太近 |
| 全黑 | depth 值接近 near、projection 錯 |
| 前後反了 | depth compare 方向錯 |
| 遠處幾乎同色 | near/far 比例造成 precision 差 |
| 閃爍 | z-fighting |

---

## 6.2 Normal View

### 目的

檢查：

```text
normal 是否正確
normal transform 是否正確
winding 是否反
non-uniform scale 是否讓 normal 壞掉
```

### 顯示方式

```cpp
Color normalToColor(Vec3 n) {
    n = normalize(n);
    return Color(
        n.x * 0.5f + 0.5f,
        n.y * 0.5f + 0.5f,
        n.z * 0.5f + 0.5f
    );
}
```

### 常見問題

| 現象 | 可能原因 |
|---|---|
| 顏色破碎 | normal interpolation 錯 |
| 模型旋轉後 normal 不轉 | normal 沒乘 model transform |
| non-uniform scale 後 lighting 怪 | 沒用 inverse-transpose normal matrix |
| 背面顏色怪 | winding / culling / normal direction 錯 |

---

## 6.3 UV View

### 目的

檢查：

```text
UV interpolation
perspective-correct interpolation
texture origin
mesh loader UV
```

### 顯示方式

```cpp
Color uvToColor(Vec2 uv) {
    return Color(
        fract(uv.x),
        fract(uv.y),
        0.0f
    );
}
```

或 checker：

```cpp
Color checkerUV(Vec2 uv) {
    int x = static_cast<int>(floor(uv.x * 10.0f));
    int y = static_cast<int>(floor(uv.y * 10.0f));
    bool checker = (x + y) % 2 == 0;
    return checker ? Color(1, 1, 1) : Color(0, 0, 0);
}
```

---

## 6.4 Barycentric View

### 目的

檢查：

```text
triangle coverage
barycentric weights
edge function sign
winding
pixel center rule
```

### 顯示方式

```cpp
Color barycentricToColor(Vec3 bc) {
    return Color(bc.x, bc.y, bc.z);
}
```

正常情況下，三個頂點附近會分別偏 R/G/B，內部平滑漸層。

---

## 6.5 Wireframe View

### 目的

檢查：

```text
vertex transform
viewport transform
mesh topology
triangle indices
culling
```

### 實作方式 A：畫線

```cpp
DrawLine(v0.screen, v1.screen);
DrawLine(v1.screen, v2.screen);
DrawLine(v2.screen, v0.screen);
```

優點：

```text
簡單
可直接用 DrawLine
```

缺點：

```text
可能和 triangle coverage rule 不完全一致
```

### 實作方式 B：barycentric edge

```cpp
float edge = min(bc.x, min(bc.y, bc.z));
if (edge < threshold) {
    return Color(1, 1, 1);
}
```

優點：

```text
和 triangle rasterization 同一套 barycentric 資料
```

缺點：

```text
線寬會受 triangle 大小與 perspective 影響
```

---

## 6.6 Overdraw View

### 目的

檢查：

```text
同一 pixel 被多少 fragments 覆蓋
fill-rate
draw order
transparent objects
UI overlay cost
early-z 的價值
```

### 實作

```cpp
std::vector<uint32_t> overdrawBuffer;
```

每次 fragment 通過 coverage test：

```cpp
overdrawBuffer[index]++;
```

可分兩種：

```text
coverage overdraw:
  triangle 覆蓋到 pixel 就 +1

write overdraw:
  通過 depth test 並寫 color 才 +1
```

第一版建議做 coverage overdraw。

---

## 6.7 ClipSpaceW View

### 目的

檢查：

```text
projection matrix
clip-space w
perspective divide
near-plane problem
perspective-correct interpolation
```

顯示：

```cpp
float v = clamp(abs(invW) * scale, 0.0f, 1.0f);
return Color(v, v, v);
```

---

# 7. Testing 架構

## 7.1 測試分層

```text
1. Math Unit Tests
2. Rasterizer Unit Tests
3. Pipeline / Shader Tests
4. Golden Image Tests
5. Invariant Tests
6. Regression Tests
```

---

## 7.2 Math Unit Tests

測：

```text
Vec3 addition / subtraction
dot product
cross product
length
normalize
Mat4 identity
translation
scale
rotation
matrix multiplication
LookAt
Perspective
Viewport
```

範例：

```cpp
void test_translation_affects_point_not_direction() {
    Mat4 T = Mat4::Translation(Vec3(3, 4, 5));

    Vec4 p(1, 2, 3, 1);
    Vec4 v(1, 2, 3, 0);

    assertVecAlmostEqual(T * p, Vec4(4, 6, 8, 1));
    assertVecAlmostEqual(T * v, Vec4(1, 2, 3, 0));
}
```

---

## 7.3 Rasterizer Unit Tests

測：

```text
edge function
barycentric coordinates
inside triangle
depth compare
bounding box
```

### Barycentric Test

三角形：

```text
A = (0, 0)
B = (2, 0)
C = (0, 2)
```

測：

```text
P = (0, 0) → (1, 0, 0)
P = (2, 0) → (0, 1, 0)
P = (0, 2) → (0, 0, 1)
P = (1, 0) → (0.5, 0.5, 0)
```

Invariant：

```text
α + β + γ ≈ 1
```

---

## 7.4 Depth Buffer Test

```cpp
void test_depth_less_passes() {
    DepthBuffer zbuf(1, 1);
    zbuf.clear(1.0f);

    bool pass1 = zbuf.testAndWrite(0, 0, 0.5f);
    bool pass2 = zbuf.testAndWrite(0, 0, 0.8f);
    bool pass3 = zbuf.testAndWrite(0, 0, 0.3f);

    assert(pass1 == true);
    assert(pass2 == false);
    assert(pass3 == true);
}
```

---

## 7.5 Pipeline Tests

測 stage 組合：

```text
MVP
Projection
Viewport
Shader IO
Perspective-correct interpolation
```

### LookAt Test

```text
View * eye ≈ origin
View * target ≈ point on camera forward axis
```

### Viewport Test

```text
NDC center → screen center
NDC top-left → screen top-left
```

### Projection Test

```text
near center → NDC z near value
far center → NDC z far value
clip.w 合理
```

---

## 7.6 Golden Image Tests

Golden image test 流程：

```text
1. 固定一個場景
2. render output image
3. 和 expected reference image 比對
4. 超過 tolerance 則 fail
5. 若 fail，輸出 diff image
```

第一批 golden tests：

```text
test_001_clear_color
test_002_single_triangle_flat_color
test_003_rgb_barycentric_triangle
test_004_depth_overlap
test_005_wireframe_triangle
test_006_uv_gradient
test_007_perspective_checker
test_008_normal_debug
test_009_depth_debug
```

---

## 7.7 Image Diff

```cpp
struct ImageDiffResult {
    int differentPixels;
    float maxError;
    float meanError;
};
```

比較：

```cpp
float error =
    abs(a.r - b.r) +
    abs(a.g - b.g) +
    abs(a.b - b.b);
```

使用 tolerance：

```cpp
if (error > epsilon) {
    differentPixels++;
}
```

Diff image：

```text
same pixel = black
different pixel = red
```

---

## 7.8 Invariant Tests

Invariant 是理論上永遠應該成立的條件。

### Barycentric

```text
α + β + γ ≈ 1
inside triangle ⇒ α, β, γ >= 0
```

### Normal

```text
length(normal) ≈ 1
```

### Matrix

```text
Identity * v = v
Translation 不影響 direction
Rotation 保持 length
View matrix 把 camera eye 變到 origin
```

### Depth

```text
clear 後 depth = clearDepth
depth fail 不更新 color
depth pass 更新 depth
```

---

# 8. 工程目錄規劃

## 8.1 原則

分清楚三種東西：

```text
Runtime Debug
  開發時在 renderer 內觀察狀態。
  放在 render/debug 或 ui/debug_panel。

Automated Tests
  自動驗證 correctness / regression。
  放在 tests。

Offline Tools
  命令列工具，例如 image_diff / trace / golden update。
  放在 tools。
```

---

## 8.2 建議完整目錄

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
│   ├─ command_queue.h/cpp
│   │
│   └─ debug/
│       ├─ debug_settings.h
│       ├─ render_mode.h
│       ├─ debug_shading.h/cpp
│       ├─ render_stats.h
│       ├─ pipeline_trace.h/cpp
│       └─ debug_draw.h/cpp
│
├─ scene/
│   ├─ scene.h/cpp
│   ├─ camera.h/cpp
│   ├─ view.h/cpp
│   ├─ renderable.h
│   └─ transform.h
│
├─ backend/
│   ├─ backend_interface.h
│   ├─ software_backend.h/cpp
│   └─ framebuffer_export.h/cpp
│
├─ ui/
│   ├─ ui_context.h/cpp
│   ├─ ui_widgets.h/cpp
│   ├─ ui_command_list.h/cpp
│   └─ debug_panel.h/cpp
│
├─ tests/
│   ├─ test_main.cpp
│   ├─ test_framework.h
│   │
│   ├─ unit/
│   │   ├─ test_vec.cpp
│   │   ├─ test_mat4.cpp
│   │   ├─ test_barycentric.cpp
│   │   ├─ test_depth_buffer.cpp
│   │   ├─ test_viewport.cpp
│   │   └─ test_camera.cpp
│   │
│   ├─ pipeline/
│   │   ├─ test_mvp.cpp
│   │   ├─ test_projection.cpp
│   │   ├─ test_shader_io.cpp
│   │   └─ test_perspective_correct.cpp
│   │
│   ├─ golden/
│   │   ├─ golden_test_runner.cpp
│   │   ├─ scenes/
│   │   │   ├─ golden_clear_color.cpp
│   │   │   ├─ golden_single_triangle.cpp
│   │   │   ├─ golden_rgb_barycentric.cpp
│   │   │   ├─ golden_depth_overlap.cpp
│   │   │   └─ golden_perspective_checker.cpp
│   │   │
│   │   ├─ expected/
│   │   │   ├─ clear_color.ppm
│   │   │   ├─ single_triangle.ppm
│   │   │   ├─ rgb_barycentric.ppm
│   │   │   └─ depth_overlap.ppm
│   │   │
│   │   └─ output/
│   │       └─ generated images
│   │
│   └─ fixtures/
│       ├─ meshes/
│       ├─ textures/
│       └─ scenes/
│
├─ tools/
│   ├─ image_diff/
│   │   ├─ image_diff.cpp
│   │   └─ image_diff.h
│   │
│   ├─ trace/
│   │   ├─ trace_vertex.cpp
│   │   └─ trace_pipeline.cpp
│   │
│   ├─ golden/
│   │   ├─ generate_golden.cpp
│   │   └─ update_golden.cpp
│   │
│   └─ viewer/
│       └─ optional_debug_viewer.cpp
│
├─ examples/
│   ├─ 01_screen_triangle.cpp
│   ├─ 02_ndc_triangle.cpp
│   ├─ 03_orthographic_triangle.cpp
│   ├─ 04_perspective_triangle.cpp
│   ├─ 05_textured_quad.cpp
│   └─ 06_debug_views.cpp
│
├─ docs/
│   ├─ debug_visualization.md
│   ├─ testing_strategy.md
│   ├─ pipeline_trace.md
│   ├─ golden_image_tests.md
│   └─ architecture_decisions.md
│
├─ makefile
└─ README.md
```

---

# 9. 目錄分工細節

## 9.1 `render/debug/`

放 runtime debug 系統。

```text
render_mode.h
  RenderMode enum

debug_settings.h
  DebugSettings

debug_shading.h/cpp
  depthToColor / normalToColor / uvToColor / debugShade

render_stats.h
  RenderStats

pipeline_trace.h/cpp
  traceVertex / traceTriangle / printTrace

debug_draw.h/cpp
  debug line / point / rect / axis
```

這些可以被 renderer 或 Debug UI 使用。

---

## 9.2 `tests/`

放自動化測試。

```text
tests/unit/
  純函式與小模組測試

tests/pipeline/
  stage 組合測試

tests/golden/
  image regression tests

tests/fixtures/
  測試用 mesh / texture / scene
```

測試不應依賴人工看圖。

---

## 9.3 `tools/`

放命令列輔助工具。

```text
image_diff/
  比較 expected / output image

trace/
  離線 trace canonical vertices

golden/
  產生或更新 golden image

viewer/
  未來可選 debug viewer
```

---

## 9.4 `examples/`

放手動 demo，對應 bootstrap ladder。

```text
01_screen_triangle
  跳過 MVP，只測 rasterizer

02_ndc_triangle
  測 NDC → Screen

03_orthographic_triangle
  測 Model / View / Ortho

04_perspective_triangle
  測 Perspective

05_textured_quad
  測 UV / Texture / Perspective-correct

06_debug_views
  展示 debug modes
```

---

# 10. 最小可行目錄配置

不用一開始建完整大目錄。  
目前可以先做：

```text
Pixel-Renderer/
│
├─ render/
│   ├─ rasterizer.h/cpp
│   ├─ render_device.h/cpp
│   └─ debug/
│       ├─ render_mode.h
│       ├─ debug_settings.h
│       ├─ debug_shading.h/cpp
│       └─ pipeline_trace.h/cpp
│
├─ tests/
│   ├─ test_main.cpp
│   ├─ test_framework.h
│   └─ unit/
│       ├─ test_mat4.cpp
│       ├─ test_viewport.cpp
│       ├─ test_barycentric.cpp
│       └─ test_depth_buffer.cpp
│
├─ examples/
│   ├─ 01_screen_triangle.cpp
│   └─ 02_ndc_triangle.cpp
│
└─ docs/
    ├─ debug_visualization.md
    └─ testing_strategy.md
```

這已經足以支撐目前階段。

---

# 11. Build Targets 規劃

Makefile 可以逐步長成：

```makefile
make debug
make release
make test
make test-unit
make test-pipeline
make test-golden
make examples
make clean
```

建議 build output：

```text
build/
├─ debug/
│   └─ PixelRenderer_debug.exe
│
├─ release/
│   └─ PixelRenderer_release.exe
│
├─ tests/
│   ├─ unit_tests.exe
│   ├─ pipeline_tests.exe
│   └─ golden_tests.exe
│
├─ examples/
│   ├─ 01_screen_triangle.exe
│   └─ 02_ndc_triangle.exe
│
└─ tools/
    ├─ image_diff.exe
    └─ trace_vertex.exe
```

短期先做到：

```text
make debug
make test
make example-screen-triangle
```

---

# 12. 近期實作順序

## Step 1：`render/debug/render_mode.h`

先定義 RenderMode。

```cpp
enum class RenderMode {
    Shaded,
    Depth,
    Barycentric,
    Wireframe,
    UV,
    Normal,
    Overdraw
};
```

---

## Step 2：`render/debug/debug_shading.h/cpp`

先支援：

```text
Depth
Barycentric
Wireframe
```

---

## Step 3：`render/debug/pipeline_trace.h/cpp`

做：

```text
traceVertex()
traceTriangle()
printTrace()
```

---

## Step 4：`tests/test_framework.h`

先做簡單 assert helpers。

---

## Step 5：`tests/unit/test_mat4.cpp`

先測：

```text
identity
translation
point vs direction
```

---

## Step 6：`tests/unit/test_viewport.cpp`

測：

```text
NDC center → screen center
NDC top-left → screen top-left
```

---

## Step 7：`tests/unit/test_barycentric.cpp`

測：

```text
barycentric sum
inside triangle
vertex weights
```

---

## Step 8：`tests/unit/test_depth_buffer.cpp`

測：

```text
clear depth
less depth passes
larger depth fails
```

---

## Step 9：`examples/01_screen_triangle.cpp`

跳過 MVP，直接測 rasterizer。

---

## Step 10：`examples/02_ndc_triangle.cpp`

只測 viewport transform。

---

## Step 11：PPM output

準備 golden image test 的基礎。

---

## Step 12：Golden image test

等 DrawTriangle + Z-buffer + viewport 穩定後再做。

---

# 13. 對目前階段的優先順序

```text
Priority 1:
  Pipeline Trace
  Screen-space Triangle Mode
  NDC Triangle Mode

Priority 2:
  Mat4 / Viewport / Barycentric / Depth unit tests

Priority 3:
  Barycentric View
  Wireframe View
  Depth View

Priority 4:
  PPM output
  Golden image test

Priority 5:
  Overdraw View
  RenderStats
  Debug UI controls
```

理由：

```text
目前 MVP / LookAt / Perspective 還不穩，
所以要先建立不依賴完整 pipeline 的可信工具。
```

---

# 14. 和 Pixel-Renderer 主線的關係

Debug / Test 系統不是旁支。  
它們會支撐後面所有主線：

```text
DrawTriangle
Z-buffer
MVP
Perspective Projection
Perspective-correct Interpolation
Texture
Phong
Material
CommandQueue
Debug UI
Shadow Mapping
Engine Mirror Demo
```

沒有它們，後面每次出錯都會回到：

```text
畫面怪怪的，但不知道是哪裡錯。
```

有了它們，開發流程會變成：

```text
我知道哪一層可信。
我知道新功能接在哪一層。
我知道畫錯時要降級到哪個 debug mode。
我知道重構後哪些 test 能保護我。
```

---

# 15. 最終總結

Pixel-Renderer 的 debug / test 系統應該分成三類：

```text
Runtime Debug
  render/debug/
  用來看 renderer 內部狀態。

Automated Tests
  tests/
  用來保護 correctness 和 regression。

Offline Tools
  tools/
  用來做 image diff、pipeline trace、golden update。
```

面對 bootstrap problem，核心策略是：

```text
不要一開始用完整 renderer 檢查完整 renderer。
先建立可信小島。
```

最重要的可信小島：

```text
1. Math Unit Tests
2. Pipeline Trace
3. Screen-space Triangle Mode
4. NDC Triangle Mode
5. Orthographic Mode
6. Perspective Mode
7. Golden Image Tests
```

目前最該先做：

```text
render/debug/pipeline_trace.h/cpp
tests/unit/test_mat4.cpp
tests/unit/test_viewport.cpp
tests/unit/test_barycentric.cpp
tests/unit/test_depth_buffer.cpp
examples/01_screen_triangle.cpp
examples/02_ndc_triangle.cpp
```

這樣即使 MVP、LookAt、Perspective 還沒寫對，也能一層一層 bootstrap 出可信 pipeline。
