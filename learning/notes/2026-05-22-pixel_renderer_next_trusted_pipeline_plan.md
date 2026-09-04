# Pixel-Renderer 下一階段實作主線：建立可信 Pipeline

日期：2026-05-22  
定位：Pixel-Renderer 從 planning 進入第一個可落地 milestone 的實作指南  
主題：Screen-space Triangle → Barycentric → Z-buffer → Debug View → Unit Test → NDC → MVP → Renderer Skeleton

---

## 0. 文件目的

前面已經整理了 Pixel-Renderer 的大方向：

```text
Rendering Systems Lab
  → Software Rasterizer
  → Renderer Architecture
  → Material System
  → CommandQueue
  → Debug UI
  → Engine Mirror Demo
  → Software GPU / FPGA optional branches
```

也整理了 Debug / Test 系統如何規劃。

現在要回到實作主線，回答：

> 接下來到底要先做什麼，才能讓 Pixel-Renderer 不只是一直加功能，而是每一層都可信地往上長？

這份文件的核心答案是：

> 先建立第一個可信小島：Screen-space Triangle Pipeline。

也就是先完成：

```text
Screen-space vertices
  → Bounding Box
  → Edge Function
  → Barycentric Coordinates
  → Depth Test
  → Color Write
  → Debug Visualization
  → Unit Tests
```

暫時不要先碰完整 MVP、LookAt、Perspective、Scene、Material、CommandQueue。

---

# 1. 為什麼下一步不是 mini-Filament skeleton？

## 1.1 mini-Filament skeleton 很重要，但不是現在第一步

長期來說，Pixel-Renderer 會長成：

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
CommandQueue
SoftwareBackend
Debug UI
```

這個方向是對的。

但如果現在 triangle rasterization、depth、viewport、MVP 都還不可信，直接建立這些高層 class 會有問題：

```text
Scene 有了，但不知道 triangle 是否畫對。
Camera 有了，但不知道 View matrix 是否對。
Renderer 有了，但不知道 Rasterizer 是否對。
Material 有了，但不知道 FragmentInput 是否可靠。
CommandQueue 有了，但不知道 command 執行結果是否正確。
```

這會變成：

```text
架構很像引擎，但底層 pipeline 不可信。
```

所以現在要先建立底層可信小島。

---

## 1.2 現在真正要解的問題

目前最核心問題不是：

```text
我要怎麼設計 Engine？
我要怎麼設計 Material？
我要怎麼接 Unity / Unreal？
```

而是：

```text
我能不能在不依賴 MVP 的情況下，穩定畫出一個正確的 triangle？
我能不能檢查 barycentric weights？
我能不能驗證 depth buffer？
我能不能用 debug view 看到內部資料？
我能不能用 unit test 保護這些東西？
```

也就是：

```text
Rasterizer 本身先可信。
```

---

# 2. 目前應該避免的過早依賴

短期先不要讓下一步依賴：

```text
Model matrix
View matrix
Projection matrix
Perspective divide
Viewport transform
IShader
Material
Scene
Camera
CommandQueue
OBJ loader
Texture
Phong
Normal map
Shadow mapping
```

原因是這些東西都會把錯誤來源變多。

如果現在直接做：

```text
Object Space → MVP → Screen → Rasterizer
```

畫面一錯，你不知道是：

```text
Mat4 錯
LookAt 錯
Projection 錯
Viewport 錯
Rasterizer 錯
Depth 錯
Color interpolation 錯
```

所以要先把完整 pipeline 拆開。

---

# 3. 下一階段總路線

接下來主線應該是：

```text
Milestone A：Screen-space Triangle Baseline
  不靠 MVP，直接畫 screen-space triangle。

Milestone B：NDC / Viewport Baseline
  只測 NDC → Screen。

Milestone C：Math / Camera Baseline
  Mat4、LookAt、Orthographic、traceVertex。

Milestone D：Perspective Baseline
  Perspective projection、clip w、perspective divide。

Milestone E：IShader / Stage Data
  VertexInput、VertexOutput、FragmentInput、ShaderContext。

Milestone F：Material / Renderer Skeleton
  Material、MaterialInstance、Scene、View、Camera、Renderer。

Milestone G：CommandQueue / Debug UI
  renderer frontend/backend 分離與 runtime debug controls。
```

目前只要專注在 A，最多準備 B。

---

# 4. Milestone A：Screen-space Triangle Baseline

## 4.1 目標

建立第一條完全不依賴 MVP 的 triangle pipeline。

輸入直接是 screen-space vertex：

```cpp
struct ScreenVertex {
    Vec2 positionSS;  // screen-space pixel coordinate
    float depth;      // [0, 1], smaller = nearer
    Color color;
};
```

API：

```cpp
void Rasterizer::DrawTriangleScreenSpace(
    const ScreenVertex& v0,
    const ScreenVertex& v1,
    const ScreenVertex& v2
);
```

這一步只處理：

```text
screen-space coordinate
bounding box
edge function
barycentric coordinates
depth interpolation
depth test
color interpolation
framebuffer write
```

不處理：

```text
MVP
NDC
Perspective
Texture
Normal
Material
Shader
```

---

## 4.2 資料流

```text
ScreenVertex v0, v1, v2
        ↓
Compute Bounding Box
        ↓
Clamp to Framebuffer
        ↓
For each pixel center
        ↓
Compute Barycentric Weights
        ↓
Inside Test
        ↓
Interpolate Depth
        ↓
Depth Test
        ↓
Interpolate Color
        ↓
Write Pixel
```

---

## 4.3 Pseudo Code

```cpp
void Rasterizer::DrawTriangleScreenSpace(
    const ScreenVertex& v0,
    const ScreenVertex& v1,
    const ScreenVertex& v2
) {
    Bounds2i box = computeBoundingBox(
        v0.positionSS,
        v1.positionSS,
        v2.positionSS
    );

    box = clampToFramebuffer(
        box,
        device.width(),
        device.height()
    );

    for (int y = box.minY; y <= box.maxY; ++y) {
        for (int x = box.minX; x <= box.maxX; ++x) {
            Vec2 p(x + 0.5f, y + 0.5f);

            Vec3 bc = computeBarycentric(
                v0.positionSS,
                v1.positionSS,
                v2.positionSS,
                p
            );

            if (bc.x < 0.0f || bc.y < 0.0f || bc.z < 0.0f) {
                continue;
            }

            float depth =
                bc.x * v0.depth +
                bc.y * v1.depth +
                bc.z * v2.depth;

            Color color =
                bc.x * v0.color +
                bc.y * v1.color +
                bc.z * v2.color;

            device.SetPixelDepthTest(x, y, depth, color);
        }
    }
}
```

---

## 4.4 這一步的核心 invariant

```text
1. barycentric α + β + γ ≈ 1
2. inside triangle ⇒ α, β, γ >= 0
3. depth = αz0 + βz1 + γz2
4. color = αc0 + βc1 + γc2
5. depth pass 才能更新 color/depth buffer
6. bounding box 不應越界 framebuffer
```

---

## 4.5 這一步學到什麼

這一步會讓你真正理解：

```text
triangle coverage
pixel center rule
edge function
barycentric coordinates
affine interpolation
Z-buffer
depth convention
framebuffer write
```

這是後面所有 rendering pipeline 的地基。

---

# 5. Milestone A 的工程變更

## 5.1 新增或修改檔案

建議最小變更：

```text
render/
  rasterizer.h/cpp
  debug/
    render_mode.h
    debug_settings.h
    debug_shading.h/cpp

core/
  render_device.h/cpp
    + depth buffer
    + ClearDepth()
    + SetPixelDepthTest()

tests/
  test_main.cpp
  test_framework.h
  unit/
    test_barycentric.cpp
    test_depth_buffer.cpp
    test_bounding_box.cpp

examples/
  01_screen_triangle.cpp
  02_depth_overlap.cpp
```

---

## 5.2 `RenderDevice` 要加入 depth buffer

目前 `RenderDevice` 如果只管理 color framebuffer，下一步要加：

```cpp
class RenderDevice {
public:
    void Clear(Color color);
    void ClearDepth(float depth);

    void SetPixel(int x, int y, Color color);

    bool SetPixelDepthTest(
        int x,
        int y,
        float depth,
        Color color
    );

private:
    int width;
    int height;

    std::vector<Color> colorBuffer;
    std::vector<float> depthBuffer;
};
```

傳統 depth convention：

```text
near = 0
far = 1
smaller depth = nearer
clear depth = 1.0
depth test = newDepth < oldDepth
```

`SetPixelDepthTest`：

```cpp
bool RenderDevice::SetPixelDepthTest(
    int x,
    int y,
    float depth,
    Color color
) {
    int index = y * width + x;

    if (depth < depthBuffer[index]) {
        depthBuffer[index] = depth;
        colorBuffer[index] = color;
        return true;
    }

    return false;
}
```

---

## 5.3 `Rasterizer` 要提供純 screen-space API

```cpp
struct ScreenVertex {
    Vec2 positionSS;
    float depth;
    Color color;
};

class Rasterizer {
public:
    void DrawTriangleScreenSpace(
        const ScreenVertex& v0,
        const ScreenVertex& v1,
        const ScreenVertex& v2
    );

private:
    Vec3 computeBarycentric(
        const Vec2& a,
        const Vec2& b,
        const Vec2& c,
        const Vec2& p
    );

    Bounds2i computeBoundingBox(
        const Vec2& a,
        const Vec2& b,
        const Vec2& c
    );
};
```

---

# 6. Milestone A 的 Unit Tests

## 6.1 `test_barycentric.cpp`

測試固定 triangle：

```text
A = (0, 0)
B = (2, 0)
C = (0, 2)
```

### 頂點測試

```text
P = A → barycentric = (1, 0, 0)
P = B → barycentric = (0, 1, 0)
P = C → barycentric = (0, 0, 1)
```

### 邊上測試

```text
P = (1, 0) → barycentric = (0.5, 0.5, 0)
```

### 內部測試

```text
P = (0.5, 0.5)
α + β + γ ≈ 1
α, β, γ >= 0
```

### 外部測試

```text
P = (2, 2)
至少一個 barycentric component < 0
```

### Test pseudo code

```cpp
void test_barycentric_vertices() {
    Vec2 A(0, 0);
    Vec2 B(2, 0);
    Vec2 C(0, 2);

    Vec3 bcA = computeBarycentric(A, B, C, A);
    Vec3 bcB = computeBarycentric(A, B, C, B);
    Vec3 bcC = computeBarycentric(A, B, C, C);

    assertVecAlmostEqual(bcA, Vec3(1, 0, 0));
    assertVecAlmostEqual(bcB, Vec3(0, 1, 0));
    assertVecAlmostEqual(bcC, Vec3(0, 0, 1));
}
```

---

## 6.2 `test_depth_buffer.cpp`

測：

```text
clear depth = 1.0
new depth = 0.5 → pass
new depth = 0.8 → fail
new depth = 0.3 → pass
```

Pseudo code：

```cpp
void test_depth_less_passes() {
    RenderDevice device(1, 1);
    device.ClearDepth(1.0f);

    bool pass1 = device.SetPixelDepthTest(0, 0, 0.5f, Color::Red());
    bool pass2 = device.SetPixelDepthTest(0, 0, 0.8f, Color::Blue());
    bool pass3 = device.SetPixelDepthTest(0, 0, 0.3f, Color::Green());

    assertTrue(pass1);
    assertFalse(pass2);
    assertTrue(pass3);
}
```

---

## 6.3 `test_bounding_box.cpp`

測：

```text
triangle bbox 是否正確
bbox 是否 clamp 到 framebuffer
負座標 triangle 是否不 crash
超出螢幕 triangle 是否正確裁切 bbox
```

測試案例：

```text
Triangle:
  (10, 20), (30, 15), (25, 50)

Expected bbox:
  minX = 10
  maxX = 30
  minY = 15
  maxY = 50
```

超出畫面：

```text
Triangle:
  (-10, -5), (20, 10), (5, 50)

Framebuffer:
  100 × 100

Clamped bbox:
  minX = 0
  minY = 0
```

---

# 7. Milestone A 的 Debug Views

## 7.1 RenderMode 初版

```cpp
enum class RenderMode {
    Shaded,
    Barycentric,
    Depth,
    Wireframe
};
```

---

## 7.2 Barycentric View

把 barycentric weights 轉成 RGB：

```cpp
Color barycentricToColor(Vec3 bc) {
    return Color(bc.x, bc.y, bc.z);
}
```

預期畫面：

```text
頂點 v0 附近偏紅
頂點 v1 附近偏綠
頂點 v2 附近偏藍
內部平滑漸層
```

能抓：

```text
barycentric 計算錯
edge function sign 錯
triangle coverage 錯
winding 錯
```

---

## 7.3 Depth View

```cpp
Color depthToColor(float depth) {
    float v = 1.0f - clamp(depth, 0.0f, 1.0f);
    return Color(v, v, v);
}
```

預期：

```text
depth 越近越亮
depth 越遠越暗
```

能抓：

```text
depth range 錯
depth compare 錯
depth 沒寫入
near/far mapping 錯
```

---

## 7.4 Wireframe View

第一版直接畫邊：

```cpp
DrawLine(v0.positionSS, v1.positionSS, Color::White());
DrawLine(v1.positionSS, v2.positionSS, Color::White());
DrawLine(v2.positionSS, v0.positionSS, Color::White());
```

用途：

```text
檢查 triangle 頂點位置
檢查 bounding / rasterization 外觀
之後檢查 viewport / MVP
```

---

# 8. Milestone A 的 Examples

## 8.1 `examples/01_screen_triangle.cpp`

目的：

```text
完全跳過 MVP，只測 screen-space rasterizer。
```

Demo：

```cpp
void OnRender() override {
    device.Clear(Color::Black());
    device.ClearDepth(1.0f);

    ScreenVertex v0 {
        Vec2(100, 100),
        0.5f,
        Color(1, 0, 0)
    };

    ScreenVertex v1 {
        Vec2(500, 120),
        0.5f,
        Color(0, 1, 0)
    };

    ScreenVertex v2 {
        Vec2(250, 400),
        0.5f,
        Color(0, 0, 1)
    };

    rasterizer.DrawTriangleScreenSpace(v0, v1, v2);
}
```

預期：

```text
畫面上出現一個 RGB 漸層三角形。
```

---

## 8.2 `examples/02_depth_overlap.cpp`

目的：

```text
測 depth test。
```

Demo：

```text
Triangle A:
  depth = 0.8
  color = blue

Triangle B:
  depth = 0.3
  color = red

兩者重疊。
```

預期：

```text
重疊區顯示 red，因為 0.3 比 0.8 近。
```

---

## 8.3 `examples/03_barycentric_debug.cpp`

目的：

```text
測 Barycentric View。
```

同樣畫一個 triangle，但 render mode 切到：

```text
RenderMode::Barycentric
```

預期：

```text
三個角分別 RGB，內部平滑混合。
```

---

# 9. Milestone B：NDC / Viewport Baseline

## 9.1 為什麼下一步是 NDC

等 screen-space triangle 可信後，下一個不應該直接做 LookAt / Perspective，而是先測：

```text
NDC → Screen
```

因為 viewport transform 牽涉：

```text
screen origin
y flip
width / height mapping
pixel center convention
depth range
```

這些如果不先固定，MVP 後面也會混亂。

---

## 9.2 API

```cpp
struct NDCVertex {
    Vec3 positionNDC; // x,y,z in NDC
    Color color;
};

void DrawTriangleNDC(
    const NDCVertex& v0,
    const NDCVertex& v1,
    const NDCVertex& v2
);
```

流程：

```text
NDCVertex
  → viewportTransform
  → ScreenVertex
  → DrawTriangleScreenSpace
```

---

## 9.3 Viewport Transform

假設：

```text
NDC x,y ∈ [-1, 1]
NDC z ∈ [0, 1]
screen origin = top-left
y grows downward
```

則：

```cpp
Vec2 viewportTransform(Vec3 ndc, int width, int height) {
    float x = (ndc.x * 0.5f + 0.5f) * width;
    float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * height;
    return Vec2(x, y);
}
```

---

## 9.4 `test_viewport.cpp`

測：

```text
NDC (-1,  1) → screen (0, 0)
NDC ( 0,  0) → screen (width/2, height/2)
NDC ( 1, -1) → screen (width, height)
```

---

## 9.5 `examples/03_ndc_triangle.cpp`

輸入：

```text
A = (-0.5, -0.5, 0.5)
B = ( 0.5, -0.5, 0.5)
C = ( 0.0,  0.5, 0.5)
```

預期：

```text
三角形出現在畫面中央附近。
```

這一步只測：

```text
NDC → Screen
Rasterizer
```

---

# 10. Milestone C：Math / Camera Baseline

## 10.1 目標

建立 Mat4、LookAt、Orthographic 的可信基礎。

這時候你的線性代數學習開始接進來。

---

## 10.2 必做項目

```text
Vec4
Mat4
Mat4::Identity
Mat4::Translation
Mat4::Scale
Mat4::RotationX/Y/Z
Mat4 multiplication
LookAt
Orthographic Projection
traceVertex
```

---

## 10.3 `test_mat4.cpp`

測：

```text
identity
translation affects point
translation does not affect direction
scale
rotation preserves length
matrix multiplication order
```

---

## 10.4 `test_camera.cpp`

測 LookAt：

```text
eye = (0, 0, 5)
target = (0, 0, 0)
up = (0, 1, 0)

View * eye ≈ (0, 0, 0, 1)
View * target ≈ (0, 0, -5, 1)
```

前提：

```text
right-handed camera looking down -Z
```

如果你採用不同 convention，要改 expected result。

---

## 10.5 traceVertex

```cpp
struct PipelineTraceResult {
    Vec4 positionOS;
    Vec4 positionWS;
    Vec4 positionVS;
    Vec4 positionCS;
    Vec3 positionNDC;
    Vec2 positionSS;
};
```

用途：

```text
不用靠畫面，直接檢查每個 stage 的數值。
```

---

## 10.6 `examples/04_orthographic_triangle.cpp`

流程：

```text
Object-space triangle
  → Model
  → View
  → Orthographic Projection
  → NDC
  → Screen
  → DrawTriangleScreenSpace
```

這一步先不要 perspective。

---

# 11. Milestone D：Perspective Baseline

## 11.1 目標

接上 perspective projection，但仍使用非常簡單的 geometry。

---

## 11.2 必做項目

```text
Perspective matrix
clip-space position
clip.w
perspective divide
NDC z mapping
Depth mapping
ClipSpaceW trace
```

---

## 11.3 Canonical test setup

```text
camera at origin
looking down -Z
near = 1
far = 10
fovY = 90°
aspect = 1
```

測：

```text
point (0, 0, -1)
point (0, 0, -10)
point (1, 1, -1)
```

確認：

```text
clip.w 合理
NDC x/y 合理
NDC z 符合 convention
```

---

## 11.4 `test_projection.cpp`

測：

```text
near center → expected NDC z
far center → expected NDC z
center point → NDC x,y near 0
point outside frustum → NDC x/y outside [-1, 1]
```

---

## 11.5 `examples/05_perspective_triangle.cpp`

畫一個在 camera 前方的 triangle。

先不要 texture。

預期：

```text
物件能被 perspective projection 正確投到螢幕上。
```

---

# 12. Milestone E：IShader / Stage Data

等 screen/NDC/MVP 可信後，才開始把 shader stage 正式化。

---

## 12.1 目標

從：

```cpp
DrawTriangleScreenSpace(v0, v1, v2)
```

逐步升級到：

```text
VertexInput
  → vertex shader
  → VertexOutput
  → rasterizer interpolation
  → FragmentInput
  → fragment shader
```

---

## 12.2 資料結構

```cpp
struct VertexInput {
    Vec3 positionOS;
    Vec3 normalOS;
    Vec2 uv;
    Color color;
};

struct VertexOutput {
    Vec4 positionCS;
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
    float invW;
    Vec3 barycentric;
};
```

---

## 12.3 IShader

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

## 12.4 第一批 shader

```text
FlatColorShader
VertexColorShader
DepthDebugShader
UVDebugShader
```

不要一開始就做 Phong。

---

# 13. Milestone F：Material / Renderer Skeleton

等 shader stage 明確後，再引入 Material / MaterialInstance / Scene / View / Renderer。

---

## 13.1 Material

```cpp
struct RasterState {
    bool depthTest = true;
    bool depthWrite = true;
    bool cullBackFace = false;
};

class Material {
public:
    IShader* shader = nullptr;
    RasterState rasterState;
};
```

---

## 13.2 MaterialInstance

```cpp
class MaterialInstance {
public:
    Material* material = nullptr;

    Vec3 baseColor = Vec3(1, 1, 1);
    Texture* diffuseTexture = nullptr;
};
```

---

## 13.3 Renderer skeleton

```text
Scene
  → Renderable
      → Mesh
      → Transform
      → MaterialInstance

View
  → Scene + Camera

Renderer::render(view)
  → iterate renderables
  → build ShaderContext
  → run vertex stage
  → rasterize
  → fragment stage
```

---

# 14. Milestone G：CommandQueue / Debug UI

等 Renderer skeleton 可以畫東西後，再做 CommandQueue。

順序：

```text
Renderer direct execution
  → Renderer records DrawMeshCmd
  → SoftwareBackend executes CommandQueue
```

---

## 14.1 CommandQueue

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
```

---

## 14.2 Debug UI

最小控制：

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

# 15. 暫時不要做的事

直到 Milestone A/B/C 穩定前，不要做：

```text
OBJ loader
Texture sampling
Phong
Normal map
Shadow mapping
PBR
CommandQueue
Scene graph
Full UI
ShaderVM
SwiftShader source deep dive
FPGA rasterizer
```

這些之後都可以做，但現在會增加太多未知數。

---

# 16. 建議 4 週實作節奏

## Week 1：Screen-space Triangle Baseline

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
01_screen_triangle
02_depth_overlap
```

---

## Week 2：NDC / Viewport Baseline

輸出：

```text
Viewport
viewportTransform
DrawTriangleNDC
test_viewport
03_ndc_triangle
```

---

## Week 3：Math / Camera Baseline

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
04_orthographic_triangle
```

---

## Week 4：Perspective Baseline

輸出：

```text
Perspective matrix
clip.w trace
perspective divide
test_projection
05_perspective_triangle
```

---

# 17. 每個 milestone 完成標準

## Milestone A 完成標準

```text
能畫 screen-space RGB triangle
能畫 depth overlap
Barycentric view 正常
Depth view 正常
Wireframe view 正常
barycentric/depth/bbox tests pass
```

---

## Milestone B 完成標準

```text
NDC triangle 正確出現在畫面中央
viewport tests pass
y flip 符合 convention
```

---

## Milestone C 完成標準

```text
Mat4 tests pass
LookAt tests pass
traceVertex 可印出 OS/WS/VS/CS/NDC/SS
Orthographic triangle 可正常顯示
```

---

## Milestone D 完成標準

```text
Projection tests pass
Perspective triangle 可正常顯示
clip.w / NDC 數值合理
Depth 在 perspective 下仍正常
```

---

# 18. 目前最重要的判斷

現在不是繼續擴大架構，而是：

```text
先建立第一個可信小島。
```

第一個可信小島是：

```text
Screen-space Triangle Pipeline
```

它包含：

```text
DrawTriangleScreenSpace
Barycentric
Z-buffer
Barycentric / Depth / Wireframe Debug View
Unit Tests
Examples
```

這一步完成後，Pixel-Renderer 才能穩定往下一層推：

```text
NDC
Viewport
Mat4
LookAt
Orthographic
Perspective
IShader
Material
Renderer Skeleton
CommandQueue
Debug UI
```

---

# 19. 最終總結

接下來的主線不是：

```text
馬上做完整 engine skeleton
馬上做 Material system
馬上做 CommandQueue
馬上做 SwiftShader-like architecture
```

而是：

```text
建立可信 pipeline。
```

具體順序：

```text
1. DrawTriangleScreenSpace
2. Barycentric + Bounding Box
3. Z-buffer
4. Debug Views: Barycentric / Depth / Wireframe
5. Unit Tests: barycentric / depth / bounding box
6. Examples: screen triangle / depth overlap
7. DrawTriangleNDC
8. Viewport Transform
9. Mat4 / LookAt / Orthographic
10. Perspective
11. IShader
12. Material / Renderer skeleton
13. CommandQueue
14. Debug UI
```

這樣 Pixel-Renderer 會從：

```text
我在學 graphics，功能一直加。
```

變成：

```text
我在建立一條每一層都可驗證、可觀察、可擴充的 rendering pipeline。
```
