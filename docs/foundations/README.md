# Foundations

這裡放 renderer 的基本規約與概念對照。這類文件應該短、穩定、可回查，避免每次寫 code 前重新討論座標系、矩陣、深度或數學語意。

目前入口：

```text
rendering_conventions.md
rasterization_edge_rules.md
interpolation_contract.md
```

後續建議再補：

```text
framebuffer_pixel_format.md
render_state.md
trace_dump_format.md
profiling_metrics.md
asset_resource_boundary.md
pipeline_flow.md
math_to_renderer.md
```

## Current Files

### `rendering_conventions.md`

定義 renderer 的憲法：

```text
coordinate spaces
handedness
matrix convention
depth convention
screen origin
pixel center rule
triangle winding
color format
```

這是第一優先。沒有這份文件，後續的 DDA、Bresenham、triangle、MVP、depth、debug view 很容易各自使用不同隱性假設。

### `rasterization_edge_rules.md`

整理 triangle rasterizer 的 correctness 細節：

```text
pixel center sampling
top-left rule
shared edge gap / double draw
degenerate triangle
integer edge function
float precision / epsilon policy
bbox half-open range
```

### `interpolation_contract.md`

整理 barycentric weights 從 coverage 變成 data transport 時的規約：

```text
coverage vs interpolation
screen-space linear interpolation
depth buffer depth vs view-space z
perspective-correct varying interpolation
OpenGL-style / Vulkan-style projection convention boundary
```

這份文件先讓 `render/raster-baseline` 不被 MVP / projection convention 卡住：

```text
ScreenVertex.z 先視為 normalized framebuffer depth [0, 1].
完整 OpenGL-style 或 Vulkan-style post-projection contract 留到 viewport / perspective branch 明確選定。
```

## Planned Files

### `framebuffer_pixel_format.md`

整理 framebuffer 與 display backend 之間的 pixel memory contract：

```text
logical color format
memory byte order
RGB / BGR / RGBA / BGRA
pitch / stride
top-down vs bottom-up rows
linear color vs sRGB
alpha convention
depth format
clear values
```

### `render_state.md`

整理 draw call 需要攜帶或綁定的 render state：

```text
depth test
depth write
culling mode
blend mode
viewport
scissor / clip rect
wireframe mode
debug render mode
```

### `trace_dump_format.md`

整理用文字追蹤 pipeline 的格式：

```text
vertex trace: OS -> WS -> VS -> CS -> NDC -> SS
triangle trace: bbox, edge values, area, barycentric
pixel trace: depth before / after, interpolated values
command trace: command queue dump
```

### `profiling_metrics.md`

整理最小 profiling 指標：

```text
draw calls
triangle count
shaded pixels
rejected pixels
depth failed pixels
overdraw
frame time
raster time
present time
```

### `asset_resource_boundary.md`

整理 asset loading 與 runtime resource 的邊界：

```text
Mesh
VertexBuffer-like data
IndexBuffer-like data
Texture
Sampler
Resource handle
loading representation vs runtime representation
ownership / lifetime
```

### `pipeline_flow.md`

整理 pipeline 每一層的資料流：

```text
current Win32 prototype flow
screen-space triangle flow
NDC / viewport flow
future shader pipeline flow
future material / renderer flow
```

### `math_to_renderer.md`

把線性代數接到 renderer 實作：

```text
field -> scalar / interpolation weight
vector space -> direction / normal
affine space -> position
basis -> coordinate space
matrix -> transform
homogeneous coordinates -> clip / projection
barycentric coordinates -> triangle interpolation
```
