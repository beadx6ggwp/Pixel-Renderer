# 下一步：Raster Baseline

日期：2026-07-02

這份文件只服務下一個近期實作 slice。它不是完整 roadmap，也不是取代 `docs/PROJECT_MAP.md`。

目前建議的下一個 source branch 是：

```text
render/raster-baseline
```

目標是建立第一個可信小島：

```text
screen-space triangle pipeline
```

---

## 1. 目前 source 現況

目前 `src/` 仍是早期 Win32 software renderer prototype：

```text
Application
  owns ScreenManager
  owns RenderDevice
  owns Rasterizer

ScreenManager
  owns Win32 window
  owns DIBSection framebuffer
  presents by BitBlt

RenderDevice
  borrows framebuffer pointer
  Clear()
  SetPixel()

Rasterizer
  DrawLine()
  DrawTriangle()
```

目前 `Rasterizer::DrawTriangle()` 是：

```text
inclusive bounding-box scan
integer pixel coordinate sampling
barycentric inside test
flat fill color
no depth buffer
no top-left rule
no color interpolation
no deterministic tests
```

所以目前不是缺少遠期方向，而是缺少可信 raster foundation。

---

## 2. 為什麼先做 raster baseline

直覺但危險的路線：

```text
先做 Mat4 / Camera / IShader / Material,
畫出比較像 3D renderer 的 demo。
```

失敗點：

```text
如果 triangle coverage / depth / interpolation 不可信,
MVP 畫錯時會不知道錯在 matrix, viewport, depth range, winding, 還是 rasterizer。
```

所以先做：

```text
screen-space triangle
```

讓 rasterizer 自己先能被測、被 debug、被解釋。

---

## 3. 這個 branch 的 convention 決策

這個 branch 不需要解決完整 OpenGL-style vs Vulkan-style projection convention。

固定短期輸入：

```text
ScreenVertex.x/y:
  screen-space pixel coordinates

ScreenVertex.z:
  normalized depth in [0, 1]

depth:
  smaller is closer
  clear = 1.0

coverage:
  pixel center sampling
  edge functions
  top-left rule
```

Projection convention 留到後面的 branch：

```text
render/viewport-ndc
render/perspective
```

那時再明確選：

```text
OpenGL-style derivation with conversion
or
Vulkan-style post-projection convention
```

不要在 `render/raster-baseline` 裡同時處理這個問題。

---

## 4. 範圍

必做：

```text
ScreenVertex
DrawTriangleScreenSpace
edge function helper
pixel-center sampling
half-open bounding box
top-left shared-edge rule
degenerate triangle rejection
barycentric weights
vertex color interpolation
depth interpolation
depth buffer
depth test
small deterministic tests
simple visual demo
```

強烈建議：

```text
Barycentric debug view
Depth debug view
Wireframe overlay
```

自然長出來才做：

```text
small PPM output helper
tiny headless framebuffer test path
```

---

## 5. 明確不在範圍內

`render/raster-baseline` 不包含：

```text
SDL backend
DisplayBackend hierarchy
owned Framebuffer refactor, unless depth/test ownership becomes blocking
Vec4 / Mat4
LookAt
Perspective projection
IShader
Material / MaterialInstance
Scene / View / Camera
CommandQueue
full debug UI
OBJ loader
Texture system
Phong / PBR
Shadow mapping
Reverse-Z
FPGA / hardware model
```

如果其中任何一項變成必要工作，就拆成另一個 branch。

---

## 6. 建議實作順序

### Step 1: 純 raster helper

先加可以不開 Win32 window 就能測的小 helpers：

```text
edge(a, b, p)
triangleArea(v0, v1, v2)
computeBoundingBoxHalfOpen(...)
isTopLeftEdge(a, b)
insideEdge(value, is_top_left)
computeBarycentric(...)
```

原因：

```text
如果 helper rules 可以測，後續 visual bugs 的可能原因會少很多。
```

### Step 2: 最小 test harness

加入最小但有用的 test path：

```text
edge sign
half-open bbox
pixel center sampling
top-left shared edge
degenerate triangle
barycentric sum
depth pass / fail
```

不要一開始就做完整 golden image system。

### Step 3: ScreenVertex 和 DrawTriangleScreenSpace

目標形狀：

```cpp
struct ScreenVertex {
    float x;
    float y;
    float z;
    uint32_t color;
};

void DrawTriangleScreenSpace(const ScreenVertex& v0,
                             const ScreenVertex& v1,
                             const ScreenVertex& v2);
```

新的 path 穩定前，現有 `DrawTriangle(...)` 可以先保留成 compatibility wrapper 或 demo path。

### Step 4: Color interpolation

用 screen-space barycentric weights 插值 RGB channels。

用它 render：

```text
red / green / blue triangle
barycentric debug view
```

### Step 5: Depth buffer

加入 depth storage 和 depth operations：

```text
ClearDepth(1.0)
SetPixelDepthTest(...)
incoming_depth < stored_depth
```

depth ownership 盡量不要放進 `ScreenManager`。

如果這件事開始卡住，就停下來拆：

```text
arch/render-target
```

### Step 6: Deterministic demo

新增或調整一個 demo，顯示：

```text
RGB screen-space triangle
two overlapping triangles with depth
wireframe overlay if available
```

---

## 7. 完成條件

`render/raster-baseline` is done when:

```text
ScreenVertex exists
DrawTriangleScreenSpace exists
pixel-center sampling is used
half-open bbox is used
edge-function coverage is used
top-left shared-edge rule has a deterministic test
degenerate triangles are rejected
barycentric weights are available for interpolation
vertex colors interpolate visibly
depth buffer exists
depth test passes/fails correctly
current Win32 demo still runs
tests or deterministic checks cover the core rules
```

更完整的完成條件：

```text
Barycentric View works
Depth View works
Wireframe Overlay works
```

---

## 8. 什麼時候拆出 `arch/render-target`

如果以下任一項開始阻塞，就拆出獨立 architecture branch：

```text
depth buffer ownership is unclear
headless tests require renderer-owned pixel storage
ScreenManager-owned DIBSection makes testing awkward
RenderDevice needs to outlive or rebind framebuffer views
pixel format / pitch needs a stable renderer-side contract
```

建議 branch：

```text
arch/render-target
```

該 branch 的範圍：

```text
Framebuffer owns color/depth memory
RenderDevice writes Framebuffer or FramebufferView
Win32 present path reads/copies from Framebuffer
no SDL yet
no full DisplayBackend hierarchy yet
```

---

## 9. branch 完成後要更新的 docs

如果這個 branch 合進主線，更新：

```text
docs/foundations/rendering_conventions.md
docs/foundations/rasterization_edge_rules.md
docs/foundations/interpolation_contract.md
docs/verification/testing_strategy.md
docs/roadmap/next_steps.md
README.md if current features changed
```

Only update `notes/journal` if you want to preserve the learning trace or tutorial narrative.

---

## 10. 短版結論

```text
下一步：
  讓 screen-space triangle rasterization 可信。

現在先不做：
  MVP, IShader, Material, CommandQueue, SDL, full DisplayBackend.

核心原則：
  graphics-first outcome,
  engineering-aware implementation.
```
