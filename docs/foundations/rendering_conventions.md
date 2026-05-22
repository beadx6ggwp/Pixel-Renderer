# Rendering Conventions

日期：2026-05-22

這份文件是 Pixel-Renderer 的 rendering convention 憲法。它定義 renderer 裡各種隱性假設：座標系、矩陣、depth、pixel sampling、triangle winding、color format、Framebuffer memory。

目的不是一次把所有功能做完，而是讓後續 code、test、debug view、文件使用同一套語言。

---

## 1. Why This Exists

software renderer 最容易出現的 bug 不是語法錯，而是不同模組使用了不同的隱性規約。

典型例子：

```text
Rasterizer 以為 y axis points down.
Math library 以為 y axis points up.
Camera 以為 smaller z is nearer.
Depth buffer 以為 larger z is nearer.
Triangle culling 在 NDC 判斷 CCW.
Screen-space rasterizer 在 y-flipped screen 判斷 CCW.
```

結果是：

```text
畫面可能仍然有東西，
但你不知道錯在 MVP、viewport、winding、depth，還是 rasterizer。
```

所以這份文件先定義共識。

---

## 2. Reference API Policy

Pixel-Renderer 的規約可以參考 Vulkan，但不要為了模仿 Vulkan 而過早架構化。

Target policy:

```text
Use Vulkan as a reference for post-projection rendering conventions:
  clip / NDC depth range
  viewport transform
  framebuffer coordinates
  explicit render state vocabulary

Do not copy Vulkan API structure too early:
  no premature Vk-like object graph
  no command buffer abstraction before raster core is trustworthy
  no pipeline state explosion before there are real states to control
```

Reason:

```text
Vulkan is useful as a precise external reference.
Pixel-Renderer is still a learning renderer, so each abstraction must earn its place.
```

Important boundary:

```text
Vulkan specifies the graphics API contract from shader outputs through rasterization and framebuffer behavior.
It does not force a single Object / World / View Space handedness for the user's math layer.
```

So Pixel-Renderer will use:

```text
Vulkan-inspired post-projection conventions.
Learning-friendly first-principles math before projection.
```

---

## 3. Status Labels

這份文件會用兩種標記：

```text
Current prototype:
  目前 Win32 prototype 實際行為。

Target convention:
  後續重構與測試應該收斂到的規約。
```

如果 current 和 target 不一致，代表之後需要被重構或用測試固定下來。

---

## 4. Coordinate Spaces

Target convention:

```text
OS  Object Space
    mesh local coordinates

WS  World Space
    object placed in world

VS  View Space
    camera-relative coordinates

CS  Clip Space
    homogeneous coordinates before perspective divide

NDC Normalized Device Coordinates
    after perspective divide

SS  Screen Space
    framebuffer pixel coordinates
```

Pipeline:

```text
OS
  -> Model matrix
  -> WS
  -> View matrix
  -> VS
  -> Projection matrix
  -> CS
  -> perspective divide
  -> NDC
  -> viewport transform
  -> SS
  -> rasterization
  -> framebuffer
```

Short-term exception:

```text
短期 screen-space triangle baseline 可以直接輸入 SS coordinates。
```

也就是：

```text
ScreenVertex
  x, y: screen-space pixel coordinates
  z: normalized depth
```

這是為了先測 rasterizer，不讓 MVP 變成額外錯誤來源。

### 4.1 Traditional CG / OpenGL vs Vulkan NDC

你在傳統 CG 書上常看到的 NDC 通常是 OpenGL-style：

```text
OpenGL-style textbook NDC:
  x in [-1, 1]
  y in [-1, 1]
  z in [-1, 1]
```

Vulkan-style NDC 則是：

```text
Vulkan-style NDC:
  x in [-1, 1]
  y in [-1, 1]
  z in [0, 1]
```

所以不要記成「Vulkan 沒有 [-1, 1] 的 NDC 方形」。

比較精確的說法是：

```text
x/y 的 NDC square 仍然是 [-1, 1] x [-1, 1]。
真正不同的是 depth z range：OpenGL-style 是 [-1, 1]，Vulkan-style 是 [0, 1]。
```

另外還有一個容易混淆的點：

```text
NDC y range
  still [-1, 1]

Framebuffer y direction
  Vulkan-style viewport with positive height maps y = -1 to the top
  and y = +1 to the bottom
```

也就是：

```text
NDC 是 clip space perspective divide 後的 normalized coordinate。
top / bottom 是 viewport transform 到 framebuffer 之後才有的解釋。
```

### 4.2 Why Vulkan Uses This Convention

Spec fact:

```text
Vulkan clips depth with:
  0 <= Zc <= Wc

After perspective divide:
  Zd = Zc / Wc

Therefore:
  Zd in [0, 1]
```

OpenGL-style teaching usually uses:

```text
OpenGL depth clip:
  -Wc <= Zc <= Wc

After perspective divide:
  Zd in [-1, 1]
```

Engineering interpretation:

```text
Vulkan is not trying to preserve OpenGL's historical clip/depth convention.
It exposes a lower-level, explicit API contract closer to modern GPU / framebuffer depth usage.
Depth buffers and viewport depth ranges are naturally expressed as [0, 1].
```

The viewport depth transform becomes direct:

```text
framebuffer_depth = minDepth + ndc_z * (maxDepth - minDepth)
```

For learning, the important takeaway is not "Vulkan is more correct". The important takeaway is:

```text
Different APIs choose different post-projection contracts.
Projection matrix, clipping, depth test, and debug visualization must all agree with the same contract.
```

Translation rule:

```text
If a book derives OpenGL-style z in [-1, 1],
convert to Vulkan-style z in [0, 1] with:

z_vk = z_gl * 0.5 + 0.5
```

But do not blindly apply this everywhere. Apply it only at a named boundary:

```text
projection matrix
clip-space conversion
viewport / depth mapping
```

---

## 5. Screen Space

Current prototype:

```text
origin: top-left
x axis: right
y axis: down
pixel address: frame_buffer + y * pitch + x * 4
```

Target convention:

```text
Screen Space origin is top-left.
x increases to the right.
y increases downward.
integer pixel coordinate (x, y) names the pixel cell.
pixel center is (x + 0.5, y + 0.5).
```

Important distinction:

```text
pixel coordinate:
  the integer address of a pixel cell

sample position:
  the continuous point tested for coverage
```

For triangle rasterization:

```text
coverage should be tested at pixel center:
(x + 0.5, y + 0.5)
```

Current mismatch:

```text
目前 DrawTriangle 使用 integer (x, y) 直接代入 barycentric formula。
```

Target:

```text
Triangle tests should use pixel center sampling.
Line rasterization can keep integer pixel stepping because line drawing is a discrete algorithm.
```

---

## 6. Viewport Transform

Target convention:

```text
NDC x in [-1, 1] maps to screen x in [0, width]
NDC y in [-1, 1] maps to screen y in [0, height]
NDC z in [0, 1] maps to framebuffer depth in [minDepth, maxDepth]
```

Formula:

```text
screen_x = (ndc_x * 0.5 + 0.5) * width
screen_y = (ndc_y * 0.5 + 0.5) * height
depth    = minDepth + ndc_z * (maxDepth - minDepth)
```

This is Vulkan-inspired:

```text
viewport x, y name the upper-left corner
positive viewport height maps NDC y = -1 to the top
positive viewport height maps NDC y = +1 to the bottom
NDC / depth z uses [0, 1], not OpenGL's historical [-1, 1]
```

Clarification:

```text
This does not mean NDC y is no longer [-1, 1].
It means the Vulkan-style viewport maps that y range into framebuffer coordinates with top-left origin.
```

Learning note:

```text
Many math derivations use +Y up before projection.
That is fine, but the projection / viewport boundary must explicitly convert into this post-projection convention.
```

Vulkan also allows negative viewport height to flip y, but Pixel-Renderer should not rely on that for the baseline. Keep the baseline simple and explicit.

---

## 7. Handedness

Target convention:

```text
World Space: right-handed
View Space: right-handed
Camera forward: -Z
```

Meaning:

```text
In view space, objects in front of the camera have negative z before projection.
```

Reason:

```text
This matches common first-principles graphics derivations and keeps cross product orientation intuitive.
```

Vulkan boundary:

```text
Vulkan does not require this World / View Space convention.
Pixel-Renderer chooses it for learning.
The Vulkan-inspired part starts at clip / NDC / viewport / framebuffer conventions.
```

Practical rule:

```text
Do not hide handedness conversion in random helper functions.
If a y flip or z remap is needed, it should be visible in projection, viewport, or a named conversion step.
```

---

## 8. Matrix Convention

Target semantic convention:

```text
vectors are column vectors
transform application is M * v
```

Full transform:

```text
clip_position = Projection * View * Model * object_position
```

Application order:

```text
object_position
  -> Model
  -> View
  -> Projection
```

Code storage convention:

```text
Matrix memory layout is an implementation detail.
```

The important distinction:

```text
semantic convention:
  what multiplication means

storage convention:
  how matrix elements are stored in memory
```

When `Mat4` is implemented, the file should explicitly define:

```text
row-major or column-major storage
operator indexing rule
matrix multiplication order
vector multiplication side
```

Until then, documentation and tests should use the semantic convention:

```text
clip = P * V * M * position
```

---

## 9. Depth Convention

Current prototype:

```text
Vertex has z.
DrawTriangle currently ignores z.
No depth buffer exists yet.
```

Short-term target:

```text
depth range: [0, 1]
near: 0
far: 1
smaller depth is closer
clear depth: 1.0
depth test: incoming_depth < stored_depth
```

This follows the Vulkan-style depth convention:

```text
clip-space depth satisfies 0 <= Zc <= Wc
NDC depth is Zd = Zc / Wc
NDC depth range is [0, 1]
viewport depth range is controlled by minDepth / maxDepth
```

For screen-space baseline:

```text
ScreenVertex.z is already normalized depth in [0, 1].
```

For future MVP pipeline:

```text
clip -> NDC -> viewport should produce the same normalized depth convention.
```

Reverse-Z:

```text
Reverse-Z is a later explicit experiment, not the default convention.
```

If Reverse-Z is enabled later, it must change the full contract:

```text
near / far mapping
clear depth
depth comparison
projection matrix
debug visualization
tests
```

Do not mix regular depth and Reverse-Z inside the same test baseline.

---

## 10. Triangle Winding and Culling

Target convention:

```text
Canonical front face is counter-clockwise in NDC.
```

Important learning note:

```text
Winding is meaningful only relative to a chosen coordinate convention.
If y direction changes, the sign of a 2D signed area may change.
```

Short-term screen-space baseline:

```text
backface culling is disabled.
Rasterizer should accept both clockwise and counter-clockwise input triangles.
```

When culling is added:

```text
culling decision should happen in a documented space.
```

Recommended:

```text
For learning, first disable culling and make coverage correct.
Then add explicit culling state later, similar in spirit to Vulkan's frontFace / cullMode.
Do not bake hidden culling assumptions into DrawTriangleScreenSpace.
```

---

## 11. Pixel Coverage Rule

Target convention:

```text
Triangle coverage uses edge functions sampled at pixel centers.
```

Pixel center:

```text
sample_x = x + 0.5
sample_y = y + 0.5
```

Bounding box should use half-open ranges:

```text
int x0 = clamp(floor(min_x), 0, width);
int x1 = clamp(ceil(max_x), 0, width);

for (int x = x0; x < x1; ++x) {
    float sample_x = x + 0.5f;
}
```

Same for y:

```text
int y0 = clamp(floor(min_y), 0, height);
int y1 = clamp(ceil(max_y), 0, height);

for (int y = y0; y < y1; ++y) {
    float sample_y = y + 0.5f;
}
```

Why half-open:

```text
It avoids off-by-one confusion at the right and bottom bounds.
```

Top-left rule:

```text
Shared edges between adjacent triangles should not double-draw or leave gaps.
```

This requires a deterministic tie-break rule for samples exactly on an edge.

Target:

```text
Use the top-left rule for edge equality.
```

Detailed formulas should live in:

```text
docs/foundations/rasterization_edge_rules.md
```

---

## 12. Barycentric Coordinates

Target convention:

```text
barycentric weights are used for interpolation.
edge functions are preferred for coverage.
```

Meaning:

```text
coverage:
  determine whether a sample is inside the triangle

interpolation:
  compute attributes at that sample
```

For a triangle:

```text
w0 + w1 + w2 = 1
```

For screen-space linear interpolation:

```text
value = w0 * v0 + w1 * v1 + w2 * v2
```

Perspective-correct interpolation:

```text
not part of the short-term screen-space baseline
```

When added, it must be documented as a separate interpolation contract:

```text
interpolate attr / w
interpolate 1 / w
recover attr = (attr_over_w) / (one_over_w)
```

---

## 13. Color Convention

Current prototype:

```text
color type: uint32_t
logical color examples: 0x00RRGGBB
Win32 DIB: 32-bit BI_RGB, top-down
memory write: *(uint32_t*)pixel = color
```

On little-endian Windows memory, logical `0x00RRGGBB` maps to bytes:

```text
BB GG RR 00
```

This matches Win32 32-bit DIB expectations for BI_RGB.

Target convention:

```text
Public renderer color should be logical Color32.
Use explicit pack/unpack helpers instead of scattering raw hex assumptions.
```

Proposed logical format:

```text
Color32: 0x00RRGGBB
```

Future extension:

```text
ColorRGBA8
ColorFloat
linear color operations
sRGB presentation conversion
alpha blending
```

Rule:

```text
Lighting math should eventually operate on linear float colors.
Framebuffer presentation may use packed 8-bit color.
```

---

## 14. Framebuffer Memory Convention

Current prototype:

```text
Framebuffer memory is owned by ScreenManager through Win32 DIBSection.
height is negative in BITMAPINFO, so DIB is top-down.
pitch = width * 4.
RenderDevice borrows raw pointer and pitch.
```

Target convention:

```text
Framebuffer owns color and depth storage.
RenderDevice writes to Framebuffer.
DisplayBackend presents Framebuffer.
```

Target fields:

```text
width
height
color buffer
depth buffer
color pitch / stride if needed
depth pitch / stride if needed
pixel format
```

Rule:

```text
Renderer code should not assume platform-owned framebuffer memory.
```

Platform-specific conversion belongs to:

```text
DisplayBackend
```

---

## 15. Clipping and Out-of-Bounds

Current prototype:

```text
SetPixel silently rejects pixels outside [0, width) x [0, height).
DrawTriangle clamps bounding box to screen.
```

Target convention:

```text
SetPixel can keep bounds checks as a safety guard.
Rasterizer should still compute a clipped bounding box before scanning.
```

Future clipping stages:

```text
screen bbox clamp
scissor / clip rect
near-plane clipping
homogeneous clip-space clipping
```

Important distinction:

```text
bounds check:
  prevents memory write errors

geometric clipping:
  changes primitive shape or coverage before rasterization
```

Do not treat `SetPixel` bounds checks as a replacement for real clipping.

---

## 16. Debug Visualization Convention

Debug views should expose internal values, not just produce another pretty render mode.

Recommended names:

```text
RenderMode::Shaded
RenderMode::Wireframe
RenderMode::Barycentric
RenderMode::Depth
RenderMode::Overdraw
RenderMode::UV
RenderMode::Normal
```

Short-term debug views:

```text
Barycentric:
  map w0, w1, w2 to RGB

Depth:
  map normalized depth [0, 1] to grayscale

Wireframe:
  draw triangle edges over fill
```

Debug view must state which space it visualizes:

```text
screen-space barycentric
post-viewport depth
view-space normal
world-space normal
```

---

## 17. Test Convention

Tests should use the same conventions as rendering code.

Minimum deterministic cases:

```text
pixel address test
bbox half-open range test
barycentric sum test
inside / outside triangle test
shared edge rule test
depth less-than pass / fail test
viewport y-flip test
color pack / unpack test
```

Golden image tests should document:

```text
image size
clear color
depth clear value
coordinate convention
color format
allowed tolerance
```

---

## 18. Current Gaps to Fix

Known current gaps:

```text
1. DrawTriangle samples integer (x, y), not pixel center.
2. DrawTriangle accepts only one fill color and does not interpolate attributes.
3. No depth buffer exists yet.
4. No top-left rule is implemented.
5. Framebuffer ownership still lives in ScreenManager / Win32 DIBSection.
6. Color format is implicit raw uint32_t.
7. Matrix / MVP conventions are not implemented yet.
```

These gaps are acceptable for the current prototype, but future rendering work should close them deliberately.

---

## 19. Short-Term Policy

For the next raster baseline, use:

```text
Reference:
  Vulkan-inspired where it clarifies post-projection conventions
  learning-first where API complexity is not yet needed

Screen Space:
  origin top-left, x right, y down

Viewport:
  NDC x [-1, 1] -> screen x [0, width]
  NDC y [-1, 1] -> screen y [0, height]
  NDC z [0, 1] -> depth [minDepth, maxDepth]

Triangle sampling:
  pixel center (x + 0.5, y + 0.5)

Depth:
  [0, 1], smaller is closer, clear to 1.0

Winding:
  accept both CW and CCW while culling is disabled

Coverage:
  edge function + top-left rule target

Color:
  logical 0x00RRGGBB for packed Color32

Framebuffer:
  target owned Framebuffer, current Win32 DIBSection until refactor
```

This gives the next branch a concrete contract:

```text
render/raster-baseline should make triangle behavior match this document.
```

---

## 20. External References

Primary Vulkan references for the convention choices above:

```text
Vulkan Specification: Fixed-Function Vertex Post-Processing
  https://github.khronos.org/Vulkan-Site/spec/latest/chapters/vertexpostproc.html
  viewport transform, perspective division, framebuffer coordinates

Vulkan Reference: VkViewport
  https://docs.vulkan.org/refpages/latest/refpages/source/VkViewport.html
  viewport x/y/width/height/minDepth/maxDepth rules, negative height note

Vulkan Guide: Depth
  https://github.khronos.org/Vulkan-Site/guide/latest/depth.html
  Vulkan depth clipping, NDC depth range [0, 1], viewport depth range
```

Use these as references for precise rendering contracts, not as a reason to copy Vulkan's full API architecture.
