# 測試策略

日期：2026-07-02

這份文件定義 Pixel-Renderer 第一階段的 testing strategy。重點不是一次建立完整 CI / golden image system，而是先讓 rasterizer 的基本規則可以被驗證。

核心原則：

```text
不要把視窗畫面當成 correctness oracle。
先用 deterministic data，視覺輸出放第二層。
```

---

## 1. 為什麼需要這份文件

直覺但不可靠的模型：

```text
執行程式
如果三角形看起來正確，rasterizer 就是正確的
```

失敗點：

```text
triangle coverage bug 可能看起來仍然可以接受
shared-edge bug 可能只會在相鄰三角形出現
depth convention bug 可能被 draw order 掩蓋
color channel bug 可能被誤認成風格差異
viewport 或 projection bug 可能反過來掩蓋 rasterizer bug
```

所以測試要先抓小規則：

```text
edge value
bbox range
sample position
top-left equality
barycentric weights
depth pass/fail
interpolated color
```

---

## 2. 測試分層

第一版分成四層。

```text
Layer 1: pure helper tests
  不需要 framebuffer、Win32、RenderDevice

Layer 2: raster output tests
  render 到小型 CPU buffer，直接檢查 pixels

Layer 3: deterministic demo cases
  可視化 demo，用於人工檢查與 screenshot

Layer 4: golden image tests
  等 raster rules 穩定後再加入
```

目前優先順序：

```text
Layer 1 > Layer 2 > Layer 3 > Layer 4
```

Golden image tests 很有用，但 raster contract 還在變動時太早建立 golden files，之後會變成大量 churn。

---

## 3. 最小 test harness 原則

先從符合目前 build system 的最小 test executable 開始。

第一版可以長這樣：

```text
tests/
  test_raster_core.cpp
```

第一版 harness 可以很簡單：

```text
CHECK(condition)
CHECK_NEAR(a, b, epsilon)
return non-zero on failure
```

不要為了測第一批 raster helpers 就引入大型 testing framework。

如果專案之後移到 CMake，tests 再變成獨立 test targets。

---

## 4. 必要 raster helper tests

### 4.1 Edge Function

測試：

```text
點在一側時 edge value 為正
點在另一側時 edge value 為負
點在 edge 上時 edge value 為 0
CW 和 CCW triangles 可以被一致地 normalize
```

原因：

```text
如果 edge sign 不清楚，inside/outside、culling、top-left rule 都會漂移。
```

### 4.2 Half-Open Bounding Box

測試：

```text
integer vertices 產生 [min, max) scan range
right / bottom bounds 不會多掃一個 pixel
負座標 clamp 到 0
超出 framebuffer 的座標 clamp 到 width / height
```

原因：

```text
Framebuffer coordinates 天然是 [0, width) x [0, height)。
```

### 4.3 Pixel Center Sampling

測試：

```text
pixel (x, y) 的 sample position 是 (x + 0.5, y + 0.5)
在已知 triangle 上，pixel center sampling 和 corner sampling 的 coverage 不同
```

原因：

```text
Triangle rasterization 測的是 sample points，不是 integer pixel addresses。
```

### 4.4 Top-Left Shared Edge

測試：

```text
兩個 triangles 組成一個 rectangle
每個預期 pixel 只被覆蓋一次
沒有 cracks
diagonal 上沒有 double draw
```

原因：

```text
這個測試用來證明 shared-edge ownership 是 deterministic。
```

### 4.5 Degenerate Triangles

測試：

```text
三個 collinear vertices 不產生 filled pixels
重複 vertices 不產生 filled pixels
degenerate primitive 不會除以 0
```

原因：

```text
Triangle fill 需要 non-zero area。
Line / point visualization 應使用獨立路徑。
```

---

## 5. 必要 barycentric tests

### 5.1 Sum Invariant

測試：

```text
mu0 + mu1 + mu2 ~= 1
```

float comparison 使用小 epsilon。

### 5.2 Vertex Identity

測試：

```text
在 v0: mu0 ~= 1, mu1 ~= 0, mu2 ~= 0
在 v1: mu1 ~= 1
在 v2: mu2 ~= 1
```

只有在 sample point 明確放在 vertex，或直接測 math helper 時，才套用這個測試。

### 5.3 Color Interpolation

使用簡單 RGB triangle：

```text
v0 = red
v1 = green
v2 = blue
```

在已知 barycentric point：

```text
color = mu0 * red + mu1 * green + mu2 * blue
```

原因：

```text
這證明 barycentric weights 不只可做 inside test，也能作為 data transport。
```

---

## 6. 必要 depth tests

短期 depth convention：

```text
depth range: [0, 1]
near: 0
far: 1
smaller is closer
clear depth: 1.0
pass if incoming_depth < stored_depth
```

### 6.1 Clear Depth

測試：

```text
ClearDepth(1.0) 後，每個 depth sample 都是 1.0
```

### 6.2 Pass / Fail

測試：

```text
stored = 0.8
incoming = 0.3
result: pass, stored becomes 0.3

stored = 0.3
incoming = 0.8
result: fail, stored stays 0.3
```

### 6.3 Draw Order Independence

Render 兩個 overlapping triangles：

```text
far triangle first, near triangle second
near triangle first, far triangle second
```

預期：

```text
overlapped pixels 的 final color 相同
```

原因：

```text
Z-buffer 的作用是移除 opaque geometry 的 draw-order dependency。
```

---

## 7. Deterministic demo cases

helper tests 之後，加入簡單 demos：

```text
RGB screen-space triangle
two overlapping depth triangles
barycentric debug view
depth grayscale view
wireframe overlay
```

每個 demo 應記錄：

```text
resolution
clear color
clear depth
triangle coordinates
預期 visual property
```

早期 raster tests 不要使用複雜 assets。

---

## 8. Golden image policy

Golden image tests 應該等到這些規則穩定後再做：

```text
pixel center rule 穩定
top-left rule 穩定
depth convention 穩定
color packing 穩定
```

第一批 golden tests 應該偏向：

```text
小解析度
exact match
固定 input
CPU-only path
不含 texture filtering
不依賴 platform window
```

不要太早使用 tolerance。

原因：

```text
對 integer framebuffer output 和 deterministic CPU rasterization 來說，
過大的 tolerance 會藏住 off-by-one、color packing、depth bugs。
```

Tolerance 後續適用於：

```text
floating-point projection differences
texture filtering
不同 compiler optimization paths
SIMD variants
platform-specific output conversion
```

---

## 9. 失敗輸出 artifacts

當 framebuffer 或 golden test 失敗時，報告：

```text
test name
resolution
第一個 mismatched pixel
expected color/depth
actual color/depth
mismatched pixels 數量
可選 actual output image
可選 diff image
```

原因：

```text
只有 "image mismatch" 不足以 debug renderer。
```

---

## 10. Debug views 和 tests

Debug views 不能取代 tests。

它們回答的是不同問題：

```text
tests:
  已知規則是否產生預期結果？

debug views:
  我現在看到的是哪個 internal value？
```

第一批 debug views：

```text
Barycentric View:
  將 mu0, mu1, mu2 對應到 RGB

Depth View:
  將 normalized depth [0, 1] 對應到 grayscale

Wireframe Overlay:
  在 fill 上疊 triangle edges
```

每個 debug view 應說明：

```text
visualizes 哪個 space
讀取哪個 buffer 或 value
可以暴露哪些 bugs
```

---

## 11. `render/raster-baseline` 第一版 checklist

branch 完成前至少要有：

```text
[ ] edge sign / normalization
[ ] bbox half-open range
[ ] pixel center sampling
[ ] CW triangle accepted
[ ] CCW triangle accepted
[ ] top-left shared edge ownership
[ ] degenerate triangle rejected
[ ] barycentric sum ~= 1
[ ] color interpolation known case
[ ] depth clear
[ ] depth pass
[ ] depth fail
[ ] overlapping triangles draw-order independence
```

最低 visual checks：

```text
[ ] RGB triangle
[ ] depth overlap demo
[ ] barycentric view or equivalent debug output
```

---

## 12. 短版結論

```text
先測 rules，不要先測 pictures。
再 render 小型 deterministic cases。
等 raster rules 穩定後才加入 golden images。
platform window smoke tests 要和 renderer correctness tests 分開。
```
