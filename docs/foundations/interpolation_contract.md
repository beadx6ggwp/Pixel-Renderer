# 插值規約

日期：2026-07-02

這份文件定義 Pixel-Renderer 裡 `barycentric weights`、depth、color、varyings 和 perspective-correct interpolation 的責任邊界。

它要解決的問題不是「重心座標公式怎麼背」，而是：

```text
同一組 barycentric weights,
哪些值可以直接線性插值?
哪些值需要 perspective correction?
depth buffer 裡的 depth 到底是哪一種 depth?
OpenGL-style / Vulkan-style convention 還沒完全定案時, 哪些部分不能混在一起?
```

---

## 1. 目前決策層級

目前最穩定的短期目標是：

```text
screen-space triangle baseline
```

這一階段不需要先決定完整 OpenGL-style 或 Vulkan-style projection matrix。

短期只需要固定：

```text
ScreenVertex.x/y:
  screen-space pixel coordinates

ScreenVertex.z:
  normalized framebuffer depth in [0, 1]

depth test:
  smaller depth is closer

clear depth:
  1.0

triangle sampling:
  pixel center (x + 0.5, y + 0.5)
```

也就是說，`render/raster-baseline` 可以先把 rasterizer 做可信，而不必同時解 MVP / projection convention。

---

## 2. 投影後規約必須是明確邊界

Pixel-Renderer 長期可能會往 Vulkan-style convention 靠近，但學習與推導過程可以先使用 OpenGL-style textbook derivation。

真正不能做的是：

```text
projection matrix 用 OpenGL-style z [-1, 1],
viewport / depth test 卻假設 Vulkan-style z [0, 1],
debug depth view 又用第三套解釋。
```

所以 project policy 應該是：

```text
OpenGL-style derivation 可以用於學習推導。
Vulkan-style convention 可以作為長期 target direction。
每個 implementation branch 都必須選一個明確的 post-projection contract。
不要在同一個 test baseline 裡混用兩套規約。
```

兩種常見 contract：

```text
OpenGL-style:
  clip depth: -Wc <= Zc <= Wc
  NDC z: [-1, 1]
  viewport maps z to framebuffer depth [0, 1]

Vulkan-style:
  clip depth: 0 <= Zc <= Wc
  NDC z: [0, 1]
  viewport maps z directly to framebuffer depth range
```

在 `render/raster-baseline` 階段，輸入已經是 screen-space，所以不用選完整 projection contract。

在 `render/viewport-ndc` 或 `render/perspective` 階段，才需要把這件事變成 explicit decision。

---

## 3. Coverage 和 interpolation 是兩件事

直覺但不夠精確的說法：

```text
barycentric weights 可以判斷 pixel 是否 inside,
所以 barycentric weights 就等於 rasterizer。
```

這個說法不夠精確。

更好的分法是：

```text
coverage:
  判斷 sample point 是否被 triangle 覆蓋

interpolation:
  把 vertex data 搬到被覆蓋的 sample 上
```

短期 target：

```text
coverage:
  edge functions + pixel center + top-left rule

interpolation:
  barycentric weights derived from edge values
```

Top-left rule 是 coverage tie-break，不是 attribute math 本身。

如果 sample 被接受，才用 edge values / area 計算 barycentric weights：

```text
mu0 + mu1 + mu2 ~= 1
```

這裡用 `mu` 表示 screen-space barycentric weights。

---

## 4. Screen-space 線性插值

在 screen-space baseline 裡，以下值可以直接用 `mu` 線性插值：

```text
debug barycentric color
vertex color for 2D / screen-space demos
ScreenVertex.z normalized depth
screen-space debug values
```

公式：

```text
value = mu0 * value0 + mu1 * value1 + mu2 * value2
```

這對 Milestone A 足夠，因為此時沒有 perspective projection。

---

## 5. Depth 插值

Depth buffer 存的是：

```text
framebuffer depth
```

不是任意的 camera-space `z`。

短期 screen-space baseline：

```text
ScreenVertex.z 已經是 [0, 1] 的 framebuffer depth。
interpolated_depth = mu0 * z0 + mu1 * z1 + mu2 * z2
```

未來接上 projection 後：

```text
clip position
  -> perspective divide
  -> NDC depth
  -> viewport depth mapping
  -> framebuffer depth
```

Rasterizer / depth test 應該拿 post-projection 的 framebuffer depth 來比較。

重要澄清：

```text
Depth buffer depth 通常不走和 UV 一樣的 perspective-correct path。
UV / normal / custom varyings 常常需要 perspective correction。
Post-projection depth 已經在 depth buffer 要比較的 space 裡。
```

容易混淆的情況：

```text
如果你把原始 view-space z 當成 user varying 插值，
那不是 depth-buffer depth。
應命名為 view_z 或 camera_z，不要叫 depth。
```

---

## 6. Perspective-correct varying 插值

Projection 會破壞 projection 前 attributes 的 naive screen-space interpolation，例如：

```text
UV
object-space / world-space position
normal before renormalization
texture coordinates
custom shader varyings
```

直覺但會失敗的模型：

```text
screen-space barycentric weights 可以插值所有 vertex attribute。
```

失敗點：

```text
perspective divide 之後，screen-space distance 不再正比於原本 3D distance。
近處 vertices 佔據的 screen area 比遠處 vertices 大。
```

每個 vertex 都要保留 clip-space `w`。

給定 screen-space barycentric weights：

```text
mu0, mu1, mu2
```

計算 perspective-correct weights：

```text
S = mu0 / w0 + mu1 / w1 + mu2 / w2

lambda0 = (mu0 / w0) / S
lambda1 = (mu1 / w1) / S
lambda2 = (mu2 / w2) / S
```

接著插值 projection 前的 attributes：

```text
attr = lambda0 * attr0 + lambda1 * attr1 + lambda2 * attr2
```

等價的實作方式：

```text
interpolate attr_over_w
interpolate one_over_w
recover attr = attr_over_w / one_over_w
```

---

## 7. 不同 attribute 的規則

不同值需要不同 post-processing：

```text
Color:
  第一個 raster baseline 先線性插值 RGB component。
  後續 lighting 應使用 linear float color，再 pack 成 Color32。

Depth:
  depth test 使用 framebuffer depth。
  如果需要 view-space z，應作為獨立 debug / shader varying。

UV:
  screen-space demo 可以直接插值。
  perspective projection 後需要 perspective-correct interpolation。

Normal:
  如果 normal 作為 varying，應做 perspective-correct interpolation。
  lighting 前要重新 normalize。

Barycentric debug:
  直接使用 screen-space mu，將 mu0/mu1/mu2 對應到 RGB。
```

---

## 8. 這份規約要求的測試

最低 deterministic tests：

```text
barycentric sum:
  mu0 + mu1 + mu2 ~= 1

vertex identity:
  sample 剛好在 vertex 上且被接受時，得到該 vertex 的值

screen-space color interpolation:
  midpoint / centroid 產生預期 RGB

screen-space depth interpolation:
  已知 sample 的 depth 符合 weighted z

depth pass/fail:
  smaller incoming depth passes，larger fails

perspective correction numeric case:
  當 w0 != w1 != w2，corrected UV 會不同於 naive UV

orthographic fallback:
  當 w0 == w1 == w2，perspective-correct weights 等於 screen-space weights
```

---

## 9. 實作指引

在 `render/raster-baseline`：

```text
1. 用 edge functions 計算 coverage
2. 計算 screen-space barycentric weights mu
3. 用 mu 插值 color
4. 用 mu 插值 ScreenVertex.z
5. 對 interpolated depth 做 depth test
6. 寫入 final Color32
```

除非 branch 明確擴張成 `render/shader-stage`，否則不要在這個 branch 實作 perspective-correct interpolation。

後續 `render/shader-stage`：

```text
1. vertex stage 輸出 clip position 和 varyings
2. rasterizer 計算 screen-space mu
3. rasterizer 從 post-projection depth 計算 framebuffer depth
4. rasterizer 為 varyings 計算 perspective-correct lambda
5. fragment stage 接收修正後的 interpolants
```

---

## 10. 短版結論

```text
Coverage:
  在 pixel center 評估 edge functions，用 top-left rule 處理 equality。

Screen-space baseline interpolation:
  color 和 normalized depth 直接使用 mu。

Depth buffer:
  儲存 framebuffer depth，不是任意 view-space z。

Perspective-correct interpolation:
  對 projection 前的 varyings 使用 lambda = (mu / w) / sum(mu / w)。

Projection convention:
  OpenGL-style 和 Vulkan-style derivation 都可以用來學習，
  但每個 implementation milestone 都必須選一個 named boundary。
```
