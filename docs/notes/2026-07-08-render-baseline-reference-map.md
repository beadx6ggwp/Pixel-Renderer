# Render baseline reference map

日期: 2026-07-08

這篇筆記只服務 `render/raster-baseline` 這條近期實作線。範圍是:

```text
screen-space triangle rasterization
edge-function coverage
pixel-center sampling
top-left shared-edge rule
barycentric weights
color/depth interpolation
depth buffer
small deterministic tests
debug views
```

暫時不處理:

```text
FPGA
soft GPU
SIMT
full material system
OpenGL/Vulkan API implementation
full engine architecture
texture system
perspective-correct interpolation as implementation target
```

這些方向都重要, 但不是目前 source branch 的 blocker。

## 先講結論

現在最值得讀的是三組資料:

```text
1. Project-local stable docs
   -> 決定 Pixel-Renderer 這條 branch 的 contract

2. Fabian Giesen + Direct3D rasterization rules
   -> 決定 edge function / top-left / sub-pixel / fill rule 的實作心智模型

3. Scratchapixel + TinyRenderer
   -> 補直覺, barycentric / depth / implementation flow
```

如果只能讀最小集合:

```text
docs/roadmap/next_steps.md
docs/foundations/rasterization_edge_rules.md
docs/foundations/interpolation_contract.md
docs/verification/testing_strategy.md
Fabian Giesen - Triangle rasterization in practice
Direct3D 11.3 Functional Spec - 3.4 Rasterization Rules
Scratchapixel - Rasterization Stage
Scratchapixel - Depth Buffer and Depth Interpolation
```

這些已經足夠支撐第一版:

```text
DrawTriangleScreenSpace
edge helper tests
top-left shared-edge test
color interpolation
depth pass/fail tests
```

## 使用原則

### 1. 本 repo docs 是 contract

外部資料是參考, 不是直接覆蓋 Pixel-Renderer 的規則。

例如 Direct3D 或 OpenGL 的 pixel-center convention 不一定和我們短期文件的座標寫法完全一樣。遇到衝突時, 先看:

```text
docs/foundations/rendering_conventions.md
docs/foundations/rasterization_edge_rules.md
docs/foundations/interpolation_contract.md
docs/roadmap/next_steps.md
docs/verification/testing_strategy.md
```

外部資料用來回答:

```text
這個規則為什麼存在?
其他系統怎麼處理?
我們的 test 應該如何抓 bug?
```

不是用來臨時改 project convention。

### 2. 先 correctness, 再 optimization

現在的核心不是讓 rasterizer 快, 而是讓 coverage / depth / interpolation 可測。

讀 Giesen 時會看到 integer overflow, sub-pixel, bias, incremental stepping, guard-band clipping。這些都很有價值, 但第一版不要一次全部實作。

第一版只需要:

```text
float or simple integer edge function
clear orientation normalization
pixel-center sampling
half-open bbox
top-left equality rule
small deterministic tests
```

### 3. 不要讓 resource reading 擴張 branch scope

讀到 texture, shader, MVP, Vulkan, software GPU, FPGA 時, 先記下來, 不要塞進 `render/raster-baseline`。

目前 branch 完成條件仍是:

```text
screen-space triangle correctness island
```

## Pipeline 對照圖

目前要建立的可信 pipeline 是:

```text
ScreenVertex
  -> triangle setup
  -> half-open bounding box
  -> pixel-center sample points
  -> edge-function coverage
  -> top-left equality rule
  -> barycentric weights
  -> color interpolation
  -> depth interpolation
  -> depth test
  -> color/depth write
  -> debug/test output
```

對應閱讀:

```text
triangle setup / edge coverage:
  Giesen, Scratchapixel, Pineda paper

fill rules:
  Direct3D rasterization rules

barycentric / interpolation:
  Scratchapixel, local interpolation_contract.md

depth:
  Scratchapixel depth lesson, local testing_strategy.md

implementation arc:
  local next_steps.md, TinyRenderer, local tutorial-soft-renderer chapters

debug/test discipline:
  local testing_strategy.md
```

## Project-local resources

### 1. `docs/roadmap/next_steps.md`

Local path:

```text
docs/roadmap/next_steps.md
```

Role:

```text
這是 render baseline 的 task contract。
```

它回答:

```text
這條 branch 要做什麼?
什麼不做?
完成條件是什麼?
如果 depth ownership 卡住, 什麼時候拆 arch/render-target?
```

導讀:

先讀這幾段:

```text
1. 目前 source 現況
3. 這個 branch 的 convention 決策
4. 範圍
5. 明確不在範圍內
6. 建議實作順序
7. 完成條件
```

要抽出的 implementation decision:

```text
ScreenVertex.x/y 是 screen-space pixel coordinates
ScreenVertex.z 在 [0, 1]
smaller depth is closer
clear depth = 1.0
coverage 使用 pixel center, edge functions, top-left rule
MVP / IShader / Material / texture 都不在這條 branch
```

讀這份文件時要避免的誤讀:

```text
它不是完整 project roadmap。
它也不是 current source truth。
真的寫 source 前仍要看 active branch 的 src/。
```

### 2. `docs/foundations/rasterization_edge_rules.md`

Local path:

```text
docs/foundations/rasterization_edge_rules.md
```

Role:

```text
這是 triangle coverage 的 stable contract。
```

它回答:

```text
edge function 怎麼定義?
pixel sample 在哪?
bbox 為什麼用 half-open range?
shared edge 上的 sample 該由哪個 triangle 擁有?
degenerate triangle 怎麼處理?
edge functions 和 barycentric weights 如何連接?
```

導讀:

先讀:

```text
2. Baseline Coordinate Contract
3. Edge Function
4. Pixel Center Sampling
5. Bounding Box
6. Top-Left Rule
7. Degenerate Triangles
9. Barycentric Weights
10. Minimum Tests
```

要抽出的 implementation decision:

```text
edge(a, b, p) 是 signed area test
pixel sample = (x + 0.5, y + 0.5)
scan range = [x0, x1) x [y0, y1)
top-left rule 只處理 equality case
degenerate triangle return early
edge values 可以作為 barycentric numerators
```

這份文件應該比外部 tutorial 更優先。外部文章用來理解它, 不是改寫它。

### 3. `docs/foundations/interpolation_contract.md`

Local path:

```text
docs/foundations/interpolation_contract.md
```

Role:

```text
把 coverage 和 data interpolation 分開。
```

它回答:

```text
coverage test 接受 sample 後, barycentric weights 拿來做什麼?
color/depth interpolation 在 screen-space baseline 裡怎麼做?
perspective-correct interpolation 何時才進來?
```

導讀:

讀這份時要抓一個分界:

```text
coverage:
  這個 sample 是否屬於 triangle?

interpolation:
  如果屬於, 這個 sample 的 color/depth/varying 是多少?
```

這是非常重要的 boundary。naive renderer 常把:

```text
inside test
barycentric calculation
color fill
depth write
```

全部混在一個 loop 裡, 結果後面很難測。

要抽出的 implementation decision:

```text
first baseline uses screen-space linear interpolation
depth z in [0, 1]
smaller z is closer
perspective-correct interpolation deferred
```

### 4. `docs/verification/testing_strategy.md`

Local path:

```text
docs/verification/testing_strategy.md
```

Role:

```text
這是第一階段測試策略。
```

它回答:

```text
哪些規則要先用 helper test 驗證?
哪些可以等 visual demo?
為什麼不要一開始就做 golden image?
depth / barycentric / shared-edge tests 要怎麼拆?
```

導讀:

先讀:

```text
2. 測試分層
4. 必要 raster helper tests
5. 必要 barycentric tests
6. 必要 depth tests
7. Deterministic demo cases
11. render/raster-baseline 第一版 checklist
```

要抽出的 test decision:

```text
Layer 1: pure helper tests first
Layer 2: small CPU buffer output tests
Layer 3: deterministic visual demo
Layer 4: golden image later
```

這份文件要和 source implementation 同步看。每加一個 helper, 就該問:

```text
它有沒有對應的 deterministic test?
```

### 5. `docs/tutorial-soft-renderer/theory/ch09_edge_function.html`

Local path:

```text
docs/tutorial-soft-renderer/theory/ch09_edge_function.html
```

Role:

```text
本 repo 自己的 edge-function 教學脈絡。
```

它適合用來回想:

```text
edge function 為什麼是 half-plane test?
為什麼 edge function 和 signed area 有關?
為什麼它比「拿 barycentric 當 inside test」更接近 production rasterizer mental model?
```

讀法:

```text
先當 teaching note 看, 不要直接當 current source contract。
真正的 branch contract 還是 rasterization_edge_rules.md。
```

### 6. `docs/tutorial-soft-renderer/theory/ch10_barycentric.html`

Local path:

```text
docs/tutorial-soft-renderer/theory/ch10_barycentric.html
```

Role:

```text
補 barycentric intuition。
```

它適合用來回答:

```text
為什麼三個 weights 可以描述 triangle 內的位置?
為什麼 weights sum 應該是 1?
為什麼 barycentric weights 可以做 color/depth interpolation?
```

對 source 的抽取:

```text
computeBarycentric(...) helper
CHECK_NEAR(w0 + w1 + w2, 1)
RGB triangle interpolation demo
```

### 7. `docs/tutorial-soft-renderer/theory/ch11_bounding_box.html`

Local path:

```text
docs/tutorial-soft-renderer/theory/ch11_bounding_box.html
```

Role:

```text
補 bbox scan 的 naive -> optimized transition。
```

它適合用來回答:

```text
為什麼不掃整張 framebuffer?
為什麼 bbox 是 triangle rasterizer 的第一個 practical optimization?
```

對 source 的抽取:

```text
computeBoundingBoxHalfOpen(...)
clamp bbox to framebuffer
test negative coordinates and over-bound coordinates
```

### 8. `docs/tutorial-soft-renderer/theory/ch12_zbuffer.html`

Local path:

```text
docs/tutorial-soft-renderer/theory/ch12_zbuffer.html
```

Role:

```text
補 depth buffer 的直覺。
```

它適合用來回答:

```text
為什麼 painter's algorithm 不夠?
為什麼 z-buffer 可以讓 opaque triangles 不依賴 draw order?
```

對 source 的抽取:

```text
ClearDepth(1.0)
DepthTest(incoming < stored)
overlap triangles draw-order independence test
```

## External resources

### 9. Fabian Giesen: Triangle rasterization in practice

Link: [Triangle rasterization in practice](https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/)

Role:

```text
這是 render/raster-baseline 最重要的外部實作導讀。
```

它回答:

```text
如何從 orient2d / edge function 寫出 triangle rasterizer?
為什麼 naive bbox + edge test 還不夠?
integer overflow 要怎麼想?
sub-pixel precision 為什麼會改變 bit budget?
fill rule / top-left rule 如何用 per-edge bias 落到 code?
```

建議閱讀段落:

```text
The basic rasterizer
Issues with this approach
Integer overflows
Sub-pixel precision
Fill rules
```

要抽到 Pixel-Renderer 的東西:

```text
orient2d / edge function 是核心
bbox scan 是合理 first implementation
fill rules 是 correctness, 不是 optional polish
top-left 可以轉成 per-edge equality bias / predicate
fast code 之前先保證 rule 正確
```

對照到目前 branch:

```text
Giesen 的 code 常用 integer sample / integer coordinate context。
Pixel-Renderer 目前 docs 指定 pixel center = x + 0.5, y + 0.5。
所以不要直接照抄 sample convention。
```

不要現在吸收:

```text
guard-band clipping
full fixed-point implementation
incremental stepping optimization
2x2 block traversal
```

這些等第一版 correctness tests 過了再說。

### 10. Fabian Giesen: A trip through the Graphics Pipeline 2011

Link: [A trip through the Graphics Pipeline 2011](https://fgiesen.wordpress.com/2011/07/09/a-trip-through-the-graphics-pipeline-2011-index/)

Role:

```text
建立現代 GPU pipeline 的 stage mental model。
```

它回答:

```text
graphics pipeline 不只是 draw triangle function。
前後有 vertex processing, clipping, setup, rasterization, interpolation, pixel/fragment processing, output merger。
```

對 `render/raster-baseline` 的讀法:

```text
只讀和 rasterization / interpolation / pixel processing 有關的篇章。
先不要把整個 pipeline 都實作。
```

要抽到 Pixel-Renderer 的東西:

```text
把 triangle setup, coverage, interpolation, depth/output write 看成不同 stage。
這有助於未來做 debug trace。
```

不要現在吸收:

```text
full GPU pipeline architecture
driver-level resource model
shader compiler concerns
```

### 11. Scratchapixel: The Rasterization Stage

Link: [The Rasterization Stage](https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage.html)

Role:

```text
補直覺和幾何可視化。
```

它回答:

```text
rasterization 到底在解什麼問題?
pixel center sample 是什麼?
coverage test 是什麼?
edge function 和 barycentric coordinates 的關係是什麼?
```

建議閱讀段落:

```text
Rasterization: What Are We Trying to Solve?
The Edge Function
Barycentric Coordinates section in same lesson
```

要抽到 Pixel-Renderer 的東西:

```text
coverage test 和 shading/data interpolation 是兩個不同步驟
edge function 是 inside/outside test
barycentric weights 是後續 color/depth interpolation 的資料通道
```

這篇很適合搭配你自己的 `rasterization_edge_rules.md` 讀:

```text
Scratchapixel 解釋直覺
repo docs 固定 convention
```

注意:

```text
Scratchapixel 的 coordinate/sign convention 不一定完全等於 Pixel-Renderer。
讀概念, 不要直接拿 sign predicate。
```

### 12. Scratchapixel: Barycentric Coordinates

Link: [Barycentric Coordinates](https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates.html)

Role:

```text
補 barycentric 的數學直覺。
```

它回答:

```text
point inside triangle 時, 三個 weights 怎麼描述位置?
weights 為什麼可用來 interpolate vertex attributes?
```

要抽到 Pixel-Renderer 的東西:

```text
barycentric sum invariant
vertex identity tests
color interpolation known-case test
```

建議寫成 tests:

```text
at v0: w0 ~= 1, w1 ~= 0, w2 ~= 0
at centroid: w0 ~= w1 ~= w2 ~= 1/3
for RGB vertices: color ~= weighted sum
```

注意:

```text
這篇不是專門講 GPU fill rules。
shared-edge ownership 還是看 Direct3D / local docs。
```

### 13. Scratchapixel: Visibility Problem, Depth Buffer, and Depth Interpolation

Link: [Visibility Problem, Depth Buffer, and Depth Interpolation](https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation.html)

Role:

```text
補 depth buffer 的直覺和 z interpolation。
```

它回答:

```text
為什麼只依 draw order 畫三角形會錯?
z-buffer 如何解決 visibility?
depth 要如何隨 triangle surface interpolation?
```

要抽到 Pixel-Renderer 的東西:

```text
depth buffer 是 per-pixel stored nearest depth
clear depth = far value
incoming depth pass/fail test
overlapping triangles should become draw-order independent
```

對照本 repo convention:

```text
Pixel-Renderer short-term depth range = [0, 1]
smaller is closer
clear = 1.0
pass if incoming < stored
```

如果 Scratchapixel 使用不同 camera-space z convention, 以本 repo 文件為準。

### 14. Scratchapixel: Perspective Correct Interpolation and Vertex Attributes

Link: [Perspective Correct Interpolation and Vertex Attributes](https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/perspective-correct-interpolation-vertex-attributes.html)

Role:

```text
這是之後要讀, 不是現在要實作的內容。
```

它回答:

```text
為什麼 screen-space linear interpolation 對 texture/attributes 會錯?
為什麼要 interpolate 1/w, attr/w?
```

對 `render/raster-baseline` 的使用方式:

```text
只讀前面 motivation。
在筆記裡記下: perspective-correct interpolation deferred。
不要現在實作。
```

為什麼現在不做:

```text
render/raster-baseline 還沒有 MVP / clip-space / viewport transform。
沒有 w, 就不該硬塞 perspective-correct path。
```

但它能幫你避免一個錯誤:

```text
不要把 screen-space linear interpolation 誤認成最終 renderer 的完整 attribute interpolation model。
```

### 15. Direct3D 11.3 Functional Specification: Rasterization Rules

Link: [Direct3D 11.3 Functional Specification](https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm)

Role:

```text
這是 top-left / sample coverage 的規格級參考。
```

建議閱讀位置:

```text
3.3 Coordinate Systems
3.4 Rasterization Rules
3.4.1 Coordinate Snapping
3.4.2 Triangle Rasterization Rules
3.4.2.1 Top-Left Rule
```

它回答:

```text
sample location 落在 triangle interior 時怎麼辦?
sample location 正好落在 edge 時怎麼辦?
top edge / left edge 的規格語意是什麼?
為什麼 adjacent triangles 不應該 double draw shared edge?
```

要抽到 Pixel-Renderer 的東西:

```text
top-left rule 是 shared-edge deterministic ownership
不是為了視覺風格
不是為了 performance
是為了避免 cracks / double draw / draw-order-dependent artifacts
```

注意:

```text
D3D spec 的 coordinate snapping / sample convention 很細。
第一版 Pixel-Renderer 可以不實作 fixed-point snapping。
但 shared-edge test 應該採用同樣精神。
```

### 16. Direct3D 9 Rasterization Rules

Link: [Direct3D 9 Rasterization Rules](https://learn.microsoft.com/en-us/windows/win32/direct3d9/rasterization-rules)

Role:

```text
比 D3D11 functional spec 更短, 適合快速理解 top-left convention。
```

它回答:

```text
Direct3D 如何用 top-left filling convention 決定 triangle 覆蓋哪些 pixels?
pixel center 為什麼是 decisive point?
rect 被切成 triangles 時, shared edge 如何分配?
```

使用方式:

```text
先讀這篇抓直覺。
再回 D3D11.3 spec 看更正式規格。
```

注意:

```text
D3D9 pixel coordinate convention 和 D3D10+ 有歷史差異。
不要把 D3D9 的 coordinate details 直接搬進 Pixel-Renderer。
只取 top-left/shared-edge 的概念。
```

### 17. OpenGL 4.6 Core Specification

Link: [OpenGL 4.6 Core Specification](https://registry.khronos.org/OpenGL/specs/gl/glspec46.core.pdf)

Role:

```text
用來做 convention comparison, 不作第一版 implementation guide。
```

適合讀的時機:

```text
render/viewport-ndc
render/perspective
projection convention comparison
fragment/sample semantics comparison
```

現在可以先查:

```text
rasterization chapter
fragment generation
multisampling/sample coverage
depth range
```

但第一版不要陷進:

```text
full OpenGL state machine
clip-space convention
gl_FragCoord exact semantics
multisampling
```

對 Pixel-Renderer 的價值:

```text
之後比較 OpenGL-style textbook derivation vs Vulkan-style explicit convention。
現在只要知道它是 future reference。
```

### 18. Vulkan Specification: Rasterization

Link: [Vulkan Specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)

Role:

```text
用來理解 modern explicit pipeline state, 不是現在的 source guide。
```

適合讀的時機:

```text
viewport/NDC branch
depth range convention
front-face/culling state
sample locations
pipeline state split
```

現在不要讀完整 Vulkan spec。它會把 branch 帶去:

```text
descriptor sets
render passes
pipeline barriers
synchronization
SPIR-V
```

這些都不是 `render/raster-baseline`。

### 19. TinyRenderer

Link: [TinyRenderer](https://github.com/ssloy/tinyrenderer)

Role:

```text
最小 software renderer implementation arc 的對照。
```

它回答:

```text
從 SetPixel 開始, 怎麼逐步長出 line, triangle, barycentric, z-buffer, model loading, shading?
一個小 renderer 的 implementation order 可以長什麼樣?
```

建議讀:

```text
Triangle rasterization
Primer on barycentric coordinates
Hidden faces removal
```

要抽到 Pixel-Renderer 的東西:

```text
小步實作節奏
每一步可視化 output
先做簡單 deterministic image
```

不要照抄:

```text
file/image format choices
coordinate convention
raster fill rule
exact code style
```

TinyRenderer 是 learning course, 不是 API-accurate rasterizer spec。

### 20. Juan Pineda: A Parallel Algorithm for Polygon Rasterization

Reference:

```text
Juan Pineda, "A Parallel Algorithm for Polygon Rasterization", Computer Graphics, 1988.
```

Role:

```text
edge-function rasterization 的歷史根源。
```

它回答:

```text
為什麼 edge function 適合 parallel rasterization?
為什麼 edge function 可以 incremental evaluation?
為什麼 triangle coverage 可以被拆成三個 half-plane tests?
```

現況:

```text
Scratchapixel 連到的 PDF mirror 目前可能不穩。
如果找不到 PDF, 先讀 Scratchapixel 和 Giesen 已經足夠實作第一版。
```

要抽到 Pixel-Renderer 的東西:

```text
edge function 不是 CPU-era scanline trick
它是 GPU-friendly / hardware-friendly coverage formulation
```

不要現在吸收:

```text
parallel hardware mapping
full block/patch rasterizer
FPGA implications
```

這些留給後續 hardware/golden-model notes。

## Suggested reading order

### Round 0: 先定 branch contract

讀:

```text
docs/roadmap/next_steps.md
docs/foundations/rasterization_edge_rules.md
docs/foundations/interpolation_contract.md
docs/verification/testing_strategy.md
```

目標:

```text
知道現在要實作什麼, 不實作什麼。
```

輸出:

```text
source TODO checklist
test checklist
```

### Round 1: Coverage correctness

讀:

```text
Giesen - Triangle rasterization in practice
Scratchapixel - The Rasterization Stage
Direct3D 9 Rasterization Rules
Direct3D 11.3 Functional Spec 3.4
```

目標:

```text
能清楚解釋:
  edge value > 0
  edge value == 0
  top-left equality
  shared-edge ownership
```

輸出:

```text
edge helper
bbox helper
isTopLeftEdge helper
insideEdge helper
shared-edge test
degenerate test
```

### Round 2: Barycentric and interpolation

讀:

```text
Scratchapixel - Barycentric Coordinates
docs/foundations/interpolation_contract.md
docs/tutorial-soft-renderer/theory/ch10_barycentric.html
```

目標:

```text
能把 barycentric weights 從 inside test 分離出來, 用於 data interpolation。
```

輸出:

```text
barycentric sum test
vertex identity test
RGB triangle interpolation
barycentric debug view
```

### Round 3: Depth

讀:

```text
Scratchapixel - Visibility Problem, Depth Buffer, and Depth Interpolation
docs/tutorial-soft-renderer/theory/ch12_zbuffer.html
docs/verification/testing_strategy.md
```

目標:

```text
建立 z-buffer correctness island。
```

輸出:

```text
ClearDepth
DepthTest
overlap triangles draw-order independence test
depth grayscale debug view
```

### Round 4: Implementation rhythm

讀:

```text
TinyRenderer triangle / barycentric / z-buffer lessons
local tutorial-soft-renderer ch09-ch12
```

目標:

```text
校準 implementation step size。
```

輸出:

```text
one small source branch
small tests
one deterministic visual demo
no full engine expansion
```

### Round 5: Future-only convention comparison

只在 `render/raster-baseline` 快完成時才讀:

```text
OpenGL 4.6 rasterization sections
Vulkan rasterization sections
Scratchapixel perspective-correct interpolation
```

目標:

```text
為後續 viewport/NDC/perspective branch 做準備。
```

輸出:

```text
future docs notes, not current source changes
```

## Resource priority table

```text
P0 - must read before source work:
  docs/roadmap/next_steps.md
  docs/foundations/rasterization_edge_rules.md
  docs/foundations/interpolation_contract.md
  docs/verification/testing_strategy.md
  Giesen - Triangle rasterization in practice

P1 - read during implementation:
  Scratchapixel - The Rasterization Stage
  Scratchapixel - Barycentric Coordinates
  Scratchapixel - Depth Buffer and Depth Interpolation
  Direct3D 9 Rasterization Rules
  Direct3D 11.3 Functional Spec 3.4

P2 - read after first pass works:
  TinyRenderer
  local tutorial-soft-renderer ch09-ch12
  Giesen graphics pipeline series

P3 - future branch only:
  OpenGL 4.6 spec
  Vulkan spec
  Scratchapixel perspective-correct interpolation
  Pineda original paper
```

## What not to read now

先不要把這些放進目前 branch:

```text
NyuziRaster
Vortex
SwiftShader source
Mesa llvmpipe source
GPU ISA docs
FPGA rasterizer papers
full Vulkan tutorial
Unreal / Unity renderer source
texture filtering papers
PBR / BRDF resources
shadow mapping resources
```

原因不是它們不重要, 而是它們會把問題從:

```text
triangle coverage/depth correctness
```

擴張成:

```text
full renderer architecture / GPU architecture / engine architecture
```

目前真正缺的是第一個可信 raster pipeline, 不是更多遠期方向。

## How to turn reading into implementation tasks

讀完每一組資料後, 都要落到一個具體 artifact。

```text
Giesen / D3D rules
  -> top-left shared-edge deterministic test

Scratchapixel edge function
  -> edge(a,b,p) helper + sign/orientation tests

Scratchapixel barycentric
  -> barycentric helper + RGB interpolation test

Scratchapixel depth
  -> depth buffer + pass/fail/draw-order tests

TinyRenderer
  -> small visual demo, not architecture copy

OpenGL/Vulkan specs
  -> future convention comparison note, not current branch code
```

## Recommended source checklist after reading

第一批 code 不需要大。讀完上面資料後, 最合理的 source checklist 是:

```text
[ ] ScreenVertex
[ ] edge(a, b, p)
[ ] signed area / orientation normalization
[ ] half-open bbox helper
[ ] pixel-center sampling helper or convention comment
[ ] top-left predicate
[ ] insideEdge(edgeValue, isTopLeft)
[ ] degenerate rejection
[ ] barycentric weights
[ ] color interpolation
[ ] depth buffer storage
[ ] ClearDepth(1.0)
[ ] depth test incoming < stored
[ ] DrawTriangleScreenSpace
```

第一批 tests:

```text
[ ] edge sign
[ ] bbox half-open
[ ] pixel center difference from corner sampling
[ ] CW / CCW normalization
[ ] shared-edge rectangle no crack / no double draw
[ ] degenerate triangle no pixels
[ ] barycentric sum ~= 1
[ ] RGB interpolation known case
[ ] depth pass
[ ] depth fail
[ ] overlapping triangles draw-order independence
```

第一批 visual demos:

```text
[ ] RGB triangle
[ ] two overlapping depth triangles
[ ] barycentric RGB debug view
[ ] depth grayscale debug view
```

## Short final guidance

目前最好的讀法不是:

```text
把所有 graphics resources 都看完再寫 code
```

而是:

```text
讀一組 rule
  -> 寫一個 helper
  -> 寫一個 deterministic test
  -> 再讀下一組 rule
```

這樣才符合 `render/raster-baseline` 的目的:

```text
先建立可信小島, 再長出 pipeline。
```
