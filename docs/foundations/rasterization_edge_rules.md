# Rasterization Edge Rules

日期：2026-05-22

這份文件定義 Pixel-Renderer 在 screen-space triangle rasterization 階段使用的 coverage 規則。它是 `docs/foundations/rendering_conventions.md` 的延伸，目標是讓下一條 `render/raster-baseline` branch 可以照著實作，而不是邊寫邊猜邊界點算不算 inside。

這份文件先聚焦 triangle coverage，不處理完整 MVP、texture、perspective-correct interpolation 或 culling。

---

## 1. Problem

Triangle rasterization 的問題不是只問：

```text
這個 pixel 在三角形裡嗎？
```

更精確的問題是：

```text
這個 sample point 是否被這個 triangle 覆蓋？
如果 sample 剛好落在 shared edge 上，應該由哪個 triangle 擁有？
如果 triangle 退化成線或點，應該怎麼處理？
如果 bbox 超出 framebuffer，掃描範圍如何 clamp？
```

如果這些規則不固定，後續會出現：

```text
adjacent triangles leave cracks
adjacent triangles double draw shared edges
wireframe / fill 對不起來
debug view 和 test 結果不穩
float precision 讓邊界 pixel 忽隱忽現
```

---

## 2. Baseline Coordinate Contract

This file assumes the convention from `rendering_conventions.md`:

```text
Screen Space:
  origin top-left
  x increases right
  y increases down

Pixel coordinate:
  integer pixel address (x, y)

Sample point:
  pixel center (x + 0.5, y + 0.5)

Depth:
  [0, 1], smaller is closer

Culling:
  disabled for the short-term screen-space baseline
```

Important:

```text
Coverage is tested at sample points, not at integer pixel corners.
```

---

## 3. Edge Function

For two vertices `a` and `b`, define the 2D edge function:

```text
edge(a, b, p) =
  (p.x - a.x) * (b.y - a.y)
  - (p.y - a.y) * (b.x - a.x)
```

This is a signed area test.

Interpretation:

```text
edge(a, b, p) > 0
  p is on one side of directed edge a -> b

edge(a, b, p) < 0
  p is on the other side

edge(a, b, p) == 0
  p is exactly on the edge
```

Because Pixel-Renderer screen space has `y` pointing down, this sign convention is inverted relative to many textbook diagrams that assume `y` points up.

For the baseline, avoid relying on a hardcoded sign by normalizing against the triangle area:

```text
area = edge(v0, v1, v2)
```

If `area > 0`, inside samples have non-negative edge values for a consistent edge order. If `area < 0`, inside samples have non-positive edge values. The implementation can either:

```text
1. flip the triangle order when area < 0
2. or multiply all edge values by sign(area)
```

Recommended for clarity:

```text
Use signed normalization:

sign = area >= 0 ? +1 : -1
e0 = sign * edge(v1, v2, p)
e1 = sign * edge(v2, v0, p)
e2 = sign * edge(v0, v1, p)
```

Then the generic inside test can start from:

```text
e0 >= 0 && e1 >= 0 && e2 >= 0
```

The top-left rule refines what happens when one of these is exactly zero.

---

## 4. Pixel Center Sampling

For each integer pixel coordinate:

```text
int x;
int y;
```

The coverage sample is:

```text
float px = x + 0.5f;
float py = y + 0.5f;
```

Reason:

```text
Pixels are areas.
Rasterization chooses a representative sample position for each pixel.
The single-sample baseline uses the pixel center.
```

Do not use:

```text
edge(..., {x, y})
```

for triangle coverage unless deliberately testing a corner-sampling experiment.

---

## 5. Bounding Box

Use a half-open integer scan range:

```text
min_x = floor(min(v0.x, v1.x, v2.x))
max_x = ceil(max(v0.x, v1.x, v2.x))

min_y = floor(min(v0.y, v1.y, v2.y))
max_y = ceil(max(v0.y, v1.y, v2.y))
```

Clamp to framebuffer bounds:

```text
x0 = clamp(min_x, 0, width)
x1 = clamp(max_x, 0, width)
y0 = clamp(min_y, 0, height)
y1 = clamp(max_y, 0, height)
```

Scan:

```cpp
for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
        float px = x + 0.5f;
        float py = y + 0.5f;
    }
}
```

Why half-open:

```text
[x0, x1) naturally matches framebuffer coordinates [0, width).
It avoids needing width - 1 / height - 1 special cases.
```

Current prototype note:

```text
The current DrawTriangle scans with <= max_x / <= max_y after clamping to width - 1 / height - 1.
The raster baseline should move to half-open ranges.
```

---

## 6. Top-Left Rule

The naive inside test:

```text
e0 >= 0 && e1 >= 0 && e2 >= 0
```

has a problem:

```text
If two triangles share an edge, a sample exactly on that edge may be accepted by both triangles.
```

The top-left rule fixes ownership of edge samples.

For an edge from `a` to `b`, define:

```text
dy = b.y - a.y
dx = b.x - a.x
```

For the baseline candidate implementation, first orient the triangle consistently, or compute the top-left predicate on the same oriented edges used by the normalized edge tests.

For an oriented edge in Pixel-Renderer's top-left screen space, the baseline predicate is:

```text
An edge is top-left if:
  dy < 0
  or
  dy == 0 && dx > 0
```

Coverage test:

```text
inside edge if:
  edge_value > 0
  or
  edge_value == 0 && edge_is_top_left
```

After sign normalization:

```text
inside =
  insideEdge(e0, edge12) &&
  insideEdge(e1, edge20) &&
  insideEdge(e2, edge01)
```

If the implementation multiplies edge values by `sign(area)` instead of reordering vertices, it must keep edge orientation and top-left classification consistent with that normalization. Do not compute edge values in one orientation and top-left flags in another.

Why this matters:

```text
Two triangles sharing an edge will not both claim the same boundary sample.
One owns the edge; the other excludes it.
```

Implementation note:

```text
The exact top-left predicate depends on edge orientation and screen-space y direction.
When writing tests, include two adjacent triangles sharing an edge.
The test result matters more than memorizing the predicate.
```

---

## 7. Degenerate Triangles

Triangle area:

```text
area = edge(v0, v1, v2)
```

If:

```text
area == 0
```

then the triangle is degenerate.

Baseline policy:

```text
Do not rasterize degenerate triangles in DrawTriangleScreenSpace.
Return early.
```

Reason:

```text
Degenerate triangles do not have a well-defined interior area for triangle fill.
Line or point visualization should be handled by line / point draw paths, not by triangle fill.
```

Future policy may add:

```text
debug mode: highlight degenerate primitives
statistics: count degenerate triangles
```

---

## 8. Float vs Integer Edge Functions

Short-term learning implementation can use `float`:

```text
ScreenVertex uses float x/y/z.
Edge values are computed as float.
```

This is good for:

```text
easy comparison with math notes
debug printing
non-integer screen vertices
simple barycentric interpolation
```

But production-style rasterizers often prefer fixed-point / integer edge functions:

```text
deterministic tie-breaking
faster incremental stepping
closer to hardware rasterization
easier exact top-left behavior
```

Project policy:

```text
Start with float edge functions for clarity.
Keep the rules deterministic.
Later add an integer / fixed-point variant as an experiment.
```

Do not mix two precision models in one baseline test. If an integer variant is added, keep separate tests.

---

## 9. Barycentric Weights

After coverage is accepted, barycentric weights can be computed from edge values:

```text
w0 = e0 / area_abs
w1 = e1 / area_abs
w2 = e2 / area_abs
```

Expected invariant:

```text
w0 + w1 + w2 ~= 1
```

Use barycentric weights for:

```text
depth interpolation
color interpolation
debug barycentric view
future UV / normal / varying interpolation
```

Baseline interpolation:

```text
screen-space linear interpolation only
```

Perspective-correct interpolation belongs to a later pipeline stage.

---

## 10. Minimum Tests

The raster baseline should include deterministic cases for:

```text
pixel center sampling
bbox half-open scan range
clockwise triangle
counter-clockwise triangle
shared edge between two triangles
degenerate triangle returns no pixels
barycentric weights sum to approximately 1
depth interpolation across a simple triangle
```

Useful concrete test shapes:

```text
right triangle with integer vertices
triangle with one horizontal edge
two triangles forming a rectangle
thin triangle with small area
triangle partially outside framebuffer
```

For shared-edge tests:

```text
Render two triangles that form a rectangle.
Every pixel in the rectangle should be covered once.
No cracks.
No double draw on the diagonal.
```

---

## 11. Implementation Target

The first implementation target is:

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

Required behavior:

```text
1. reject degenerate triangles
2. compute half-open bbox
3. sample at pixel center
4. use edge functions for coverage
5. use top-left rule for edge equality
6. compute barycentric weights
7. interpolate depth
8. write pixel only if depth test passes
```

Debug modes should be able to visualize:

```text
barycentric RGB
depth grayscale
wireframe overlay
```

This is the first correctness target for `render/raster-baseline`.
