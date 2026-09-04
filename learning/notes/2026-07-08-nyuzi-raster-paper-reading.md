# NyuziRaster paper reading

Paper:

- Jeff Bush, Mohammad A. Khasawneh, Khaled Z. Mahmoud, Timothy N. Miller.
- "NyuziRaster: Optimizing Rasterizer Performance and Energy in the Nyuzi Open Source GPU."
- ISPASS 2016.
- Local PDF: `C:/Users/david/Desktop/learnCG/paper/NyuziRaster.pdf`

## 一句話結論

這篇不是教「怎麼在 CPU 上畫三角形」的 paper。它真正回答的是：

> 如果一顆 GPU 像 Larrabee / Nyuzi 一樣，想用 general-purpose wide SIMD core 取代固定功能 rasterizer，rasterization 這件事到底值不值得硬體化？

paper 的結論很直接：

- software rasterizer 可以做得更好，但仍然會把 rasterization 放在 rendering critical path 上。
- 一個很小的 hardware rasterizer coprocessor 就能移除這段 critical path。
- 對 Nyuzi 來說，最佳硬體設計 H5 比 Larrabee-style software rasterizer 有約 26% higher throughput，energy 至少少 20%。
- H5 相對整個 Nyuzi core 很小，45nm ASIC estimate 約為 core 面積的 `1/88`，power 約為 `1/237`。

對 Pixel-Renderer 最重要的啟發不是「現在就做硬體」，而是：

> rasterizer 的真正輸出可以先被定義成 coverage mask / fragment work，而不是直接等同於 `SetPixel()`。這個邊界會讓未來 software golden model 和 FPGA raster block 對得起來。

## 這篇在解決什麼問題

### 背景: Larrabee 的假設

Larrabee 不是傳統 GPU。它的設計思想是：

- 用許多 general-purpose x86-like cores。
- 每個 core 有 wide vector ALU。
- 許多過去固定功能的 graphics pipeline stage，可以改用軟體在 vector unit 上做。
- rasterization 也被放進 software path。

這個想法的吸引力是很明顯的：

- hardware fixed-function block 比較 rigid。
- software 可以根據 triangle shape、tile、special case 選不同 code path。
- shader / raster / clipping 等工作都可以留在可程式化 core 上。

但 failure point 是：

- rasterization 是每個 triangle 都會碰到的 pipeline stage。
- 即使單次計算不重，總呼叫次數非常高。
- 若 software rasterizer 有 branch、recursion、setup overhead、mask generation overhead，這些成本會反覆出現在 rendering critical path。

paper 引用先前 Nyuzi / Nyami 相關研究指出，software rasterization 會讓 benchmark runtime 增加約 `10%` 到 `30%`。這就讓 rasterizer 成為值得優化的目標。

### Nyuzi 為什麼適合研究這件事

Nyuzi 是 Larrabee-like 的 open source GPGPU / GPU research platform：

- synthesizable RTL。
- 可以跑在 FPGA。
- 有 functional simulator。
- 有 Verilator cycle-precise simulation。
- 有 LLVM-based C/C++ compiler toolchain。
- 有 rendering library。
- 有 video / memory controller 等較完整 SoC 組件。

這點很重要。這篇不是只在軟體模擬器裡調參，而是能把 rasterizer 改成 RTL coprocessor，做 area / delay / power / performance tradeoff。

## Nyuzi 的 rendering model

Nyuzi 和 Larrabee 一樣，是 tile-based 的 parallel rendering model。

主流 GPU 常見流程是：

```text
triangle
  -> fixed-function rasterizer
  -> fragments grouped into warp / wavefront
  -> shader cores run fragment shader
```

Nyuzi 的分工比較像：

```text
framebuffer tiles
  -> render threads dynamically take tiles
  -> each thread rasterizes and shades triangles/fragments in its tile
```

也就是說，它的 parallel unit 更偏 tile，而不是像現代 GPU 那樣由 hardware rasterizer 產生 fragment groups 再餵給 shader cores。

paper 強調 Nyuzi 和 mobile tile-based GPU 也不同：

- 很多 mobile GPU 會一次處理一個 tile，tile serial，tile 內 fragments / triangles parallel。
- Nyuzi 可以 parallel process multiple tiles。
- 每個 tile thread 內，triangles 大致 serial，但 fragments 用 vector 方式處理。

這個背景會影響 rasterizer 設計：rasterizer 不是單純畫整張圖，而是對某個 `triangle + tile` 產生 patch masks。

## Rasterization 的基本模型

paper 使用 edge equation 判斷 coverage。

每個 triangle edge 可以表示成：

```text
Ax - By + C = 0
```

對某個 pixel sample point：

```text
inside if all three edge equations >= 0
```

paper 的 Figure 3 描述的是：

```text
triangle edges
  -> intersect with 4x4 pixel patches
  -> output 16-bit mask
  -> mask controls vector lanes for shading
```

也就是 rasterizer 不是一次吐出單一 pixel，而是吐出一個 `4x4 patch mask`。

這和 Pixel-Renderer 目前的 per-pixel `SetPixel()` mental model 不一樣：

```text
Pixel-Renderer now:
  loop y
    loop x
      if inside triangle:
        SetPixel(x, y)

Nyuzi-style raster work:
  for each 4x4 patch:
    compute 16-bit coverage mask
    shade active lanes
```

這個差異是這篇對未來 FPGA golden model 很有用的地方。若 future hardware 也以 patch mask 為輸出，software renderer 可以先定義 mask correctness。

## Rasterization 被拆成哪些 phase

paper 把 rasterization 拆成五個 phase：

1. `Setup`
   - 從 triangle vertices 算 edge parameters `A, B, C`。
   - 算 triangle bounding box。

2. `Iteration`
   - 在 tile 或 `tile ∩ triangle bounding box` 中走訪 patches。

3. `Mask computation`
   - 對每個 `4x4 patch` 和三條 triangle edges 做交集測試。
   - 產生 16-bit mask。

4. `Rejection`
   - 若 mask 全 0，代表 patch empty，可以跳過。

5. `Clipping`
   - reject clipping rectangle 外的 patches。

這個 phase split 是 paper 的核心之一，因為後面所有 design 都是在問：

```text
哪些 phase 放硬體？
哪些 phase 留軟體？
硬體要支援多少 triangle contexts？
sharing rasterizer 時，是 concurrent sharing 還是 serial lock sharing？
```

## Naive model: 全部用 software 做

最直覺的 Larrabee-style 想法是：

```text
wide vector ALU already exists
  -> rasterization can be just another software kernel
  -> no dedicated raster hardware needed
```

這不是愚蠢的想法，因為 software 確實有優點：

- 可以針對不同 triangle shape 寫 special cases。
- 可以在很大的 triangle 上做 hierarchical trivial accept / reject。
- 可以根據 workload 調整 algorithm。

但 rasterization 的壓力在於：

- triangle 數很多。
- small triangle 很常見。
- 每個 triangle 的 setup / traversal overhead 都會累積。
- vector ALU 若沒填滿 lane，效率會下降。
- recursive algorithm 對 large trivial regions 很好，但 small triangles 的 recursion overhead 變得明顯。

所以問題不是 software rasterizer 做不到，而是：

> 用 general-purpose core 做 rasterization，是否浪費了太多能量和 critical-path cycles？

## Recursive rasterizer vs scanning rasterizer

paper 比較了兩個主要 algorithm family。

### Recursive rasterizer

Larrabee-style recursive rasterizer 會把 tile 分層：

```text
64x64 tile
  -> 16x16 subtile
  -> ...
  -> 4x4 patch
```

它會判斷 region 和 triangle 的關係：

- entirely outside: trivial reject。
- entirely inside: trivial accept。
- partial overlap: 繼續 subdivide。

這個方法對大型 triangle 很合理：

- 大片空白可以一次 reject。
- 大片覆蓋可以一次 accept。
- 不需要逐 patch 掃完所有地方。

但對 small triangles 或細碎 workload：

- recursion control overhead 變得相對大。
- state transition 多。
- real graphics workload 裡很多 triangle 不大，這個 overhead 會顯現。

paper 的 L1 就是 baseline recursive software rasterizer。

### Scanning rasterizer

paper 另外設計 iterative scanning algorithm：

```text
triangle bounding box ∩ tile
  -> scan 4x4 patches
  -> compute mask per patch
  -> reject empty patch
```

缺點是：

- 對某些大 triangle 或稀疏 overlap，可能掃太多 patch。
- 若掃整個 tile，空白 patch 會浪費很多 cycles。

但關鍵改進是限制 scanning region：

```text
scan only intersection of tile and triangle bounding box
```

這讓 small triangle 變得便宜很多。paper 的結論是：

> 對硬體 accelerator 而言，scanning algorithm 比 recursive algorithm 更適合，因為它在 real graphics workloads 中能在給定時間內處理更多 triangles。

這對 Pixel-Renderer 的 current rasterizer design 很有關係。Pixel-Renderer 近期要做的 `edge-function + bounding box + top-left rule`，其實就是走向 GPU-like scanning mental model，而不是 classic recursive Larrabee path。

## Hardware rasterizer architecture

paper 把 rasterizer 做成 Nyuzi SoC 裡的一個 I/O coprocessor。

簡化流程：

```text
Nyuzi core
  -> memory-mapped I/O write triangle state
  -> rasterizer computes patch masks
  -> software dequeues non-empty patch
  -> shader/vector code shades active lanes
```

hardware block 內大致包含：

```text
I/O
edge setup
bounding box
thread and triangle state RAM
edge mask computation
state machine
thread scheduler
```

這個設計不是把整個 rendering pipeline 都硬體化。它只把 rasterization 裡很固定、很常被呼叫、很適合小 state machine 的部分抽出來。

這是重要的 first-principles point：

```text
如果一件事:
  1. 每個 frame 大量重複
  2. 控制流程固定
  3. data width 小
  4. 結果可被壓縮成 mask
  5. general-purpose core 做它會卡住 critical path

那它很適合固定功能硬體。
```

Rasterization 正好符合。

## Designs: L1, R1, H1-H6, S variants

paper 實作並比較了多個版本。這些名字要抓住概念，不需要背代號。

### L1 - software recursive baseline

L1 是 Abrash / Larrabee-style recursive rasterizer 的 software implementation。

它是所有 experiment 的 baseline。

### R1 - recursive algorithm in hardware

R1 把 recursive rasterizer 做成 hardware pushdown finite-state machine。

但 paper 後來不主推 R1，因為 H2 類的 scanning path 更 compact、更 scalable，而且 H2 比 R1 快很多。

重點：

- 不是「只要硬體化就好」。
- algorithm 和 hardware shape 要合。
- recursive control 對硬體不一定是最自然的形狀。

### H1 - scan entire 64x64 tile

H1 是 scanning family 的 upper-bound prototype：

- 對每個 triangle 掃整個 `64x64 tile`。
- 以 `4x4 patch` 為單位。
- setup 很快。
- mask 每 patch 一 cycle。

但這是概念上界，不是 synthesizable design，而且每個 rasterizer 只支援一個 concurrent triangle。

問題也很明顯：

```text
triangle 不碰到 tile
  -> 仍然可能花大量 cycles 掃完整 tile
```

### H2 - scan bbox intersection

H2 改成只掃：

```text
triangle bounding box ∩ tile
```

這是 paper 裡很關鍵的方向，因為它避免 H1 在 empty area 上浪費 cycles。

H2 是 algorithmic proof-of-concept，但不是最終 synthesizable design。

### H3 - synthesizable H2-like design

H3 是把 H2 變成 synthesizable version：

- setup 約 7 cycles。
- mask computation 分成三條 edge，各一 cycle。
- 再加 reject/accept 和 coordinate advance。
- 每個 Nyuzi thread 需要 dedicated rasterizer。

這表示 H3 速度不錯，但 sharing/scalability 還不是最好。

### H4 - time-sharing with triangle context RAM

H4 嘗試讓一個 rasterizer 支援多個 triangle contexts：

- triangle state 存在 RAM。
- state machine 在 active triangles 間切換。
- 一個 H4 rasterizer 可以同時管理很多 triangles。

paper 說 single H4 rasterizer 平均可支援約 `356` concurrent triangles。

但問題是 critical path 太慢。加入 H4 會降低 Nyuzi clock speed，讓其他 computation 也變慢。

這是硬體設計很重要的 tradeoff：

> accelerator 不能只看自己的 throughput。若它拖慢全 SoC clock，整體反而可能輸。

### H5 - optimized timing version

H5 是 paper 最推薦的 hardware rasterizer：

- 基於 H4。
- 調整後可以維持和 Nyuzi core 相同的 FPGA clock speed。
- setup 從 7 cycles 拉到 14 cycles。
- 但 scalability 幾乎不受影響。
- 平均 projection 約 `283` concurrent triangles。

H5 的精神是：

```text
多花幾個 local cycles
  -> 換取不拖慢 global clock
  -> overall system 更好
```

這點對未來 FPGA 很重要。hardware block 不是越 aggressive 越好，能不能 meet timing 往往比單 block cycles 更關鍵。

### H6 - serial lock sharing

H6 嘗試模仿某種「多 thread serial share 一個 rasterizer」：

```text
thread locks rasterizer
  -> process whole triangle
  -> unlock
  -> shade patches
```

結果不好，原因有兩個：

- Nyuzi uncached peripheral bus 很低頻寬。
- threads 競爭 rasterizer lock，浪費很多時間 spinning。

這是一個很好的反例：

> sharing hardware 不等於好。若 sharing protocol 讓 thread 等 lock，concurrency 會被破壞。

H5 類 concurrent context design 比 H6 類 serial lock design 更合理。

### Software/hardware split variants

paper 也做了 phase split 實驗，觀察哪些 phase 留在 software 會造成多少 overhead。

最重要的是 S6：

- S6 是 scanning algorithm 全部用 software 實作。
- S6 已經比 L1 好很多。
- 但 H5 仍比 S6 快約 `5.1%`，energy 更好。

這告訴我們：

```text
algorithm improvement first
  -> software scanning already removes much overhead
  -> hardware still wins, because power/area overhead is tiny and it runs in parallel
```

## Workloads

paper 用兩類 workload：

### Conformance tests

- `blend`
- `clip`
- `depth`
- `fill`
- `mipmap`
- `texture`
- `triangle`

這些比較像 correctness / feature tests。

### Benchmarks

- `teapot`
- `sponza`
- `quake`

這些比較能代表 real graphics workloads。

paper 明確說 benchmarks 從 hardware acceleration 得到比較穩定的改善；conformance tests 大多不太受影響。

一個原因是 conformance tests 平均每 patch 有更多 active pixels：

- conformance tests 約 `15.4 pixels/patch`
- benchmarks 約 `7.6 pixels/patch`

active lanes 越多，vector ALU 越有效率。real benchmark 裡 patch mask 更 sparse，software overhead 更容易浮現。

## Performance 結果

以 L1 baseline normalized runtime = `1.000`：

```text
H5 runtime ratio: 0.793
S6 runtime ratio: 0.834
```

換句話說：

- H5 cycles 約少 `20.7%`。
- throughput 約高 `26%`。
- S6 software scanning 已經比 L1 好，runtime ratio `0.834`。
- H5 比 S6 只快約 `5.1%`，但它的 energy 和 scalability 更好。

Table III 的 geometric mean 大致是：

```text
cycles / triangle
  L1: 10340
  H5: 8202
  S6: 8622

cycles / 4x4 patch
  L1: 2066
  H5: 1638
  S6: 1722

cycles / pixel
  L1: 274.5
  H5: 217.7
  S6: 228.9
```

要注意這些數字不是 Pixel-Renderer 可以直接拿來比較的 CPU runtime。它們是在 Nyuzi RTL simulation / architecture context 下的 cycles。

但 trend 很重要：

```text
recursive software rasterizer
  -> scanning software rasterizer
  -> compact hardware scanning rasterizer
```

每一步都在移除 rasterization overhead。

## Area / delay / power 結果

Table IV 的重點：

```text
Nyuzi core:
  area: 1,800,523 um2
  delay: 1.75 ns
  power: 1.71 W

H5 rasterizer:
  area: 20,441 um2
  delay: 1.75 ns
  power: 7.21 mW
```

因此 paper 說：

```text
H5 area ~= 1/88 of Nyuzi core
H5 power ~= 1/237 of Nyuzi core
```

這是整篇最有力的硬體論點。

如果固定功能硬體只佔極小面積和 power，卻能讓 general-purpose core 少做高頻重複工作，那它通常是划算的。

FPGA 上 H5 大約：

```text
LEs: 3463
FFs: 1889
delay: 15.6 ns
```

H4 雖然概念好，但 delay 是 `19.1 ns`，會拖慢 Nyuzi clock。H5 犧牲 local setup cycles 換 timing closure。

## Energy 結果

paper 對平均 benchmark runtime 的 energy estimate：

single core:

```text
L1: 187.11 mJ
S6: 156.02 mJ
H5: 149.04 mJ
```

所以：

- S6 比 L1 少約 `16.6%` energy。
- H5 比 L1 少約 `20.3%` energy。

10 cores estimate:

```text
H5: 148.48 mJ
```

paper 的論點是，core 數增加時，單一 shared rasterizer 的相對面積成本會更小，energy benefit 更明顯。

注意 paper 也誠實說它沒有 gate-level simulation，所以 switching activity estimate 有限制。它認為 rasterizer power ratio 是 conservative estimate。

## Discussion 的核心

paper 的 discussion 可以濃縮成幾句話：

1. Larrabee recursive rasterizer 很優雅，但不是最有效率。
2. 對 small triangles，iterative scanning 更好。
3. scanning 必須限制在 `4x4 patches inside triangle bounding box`，否則會浪費時間 reject empty patches。
4. software scanning 已經比 recursive software 好很多。
5. hardware rasterizer 仍更好，因為面積/power 成本太小。
6. rasterization 應該從 GPU critical path 中移出。

這是 fixed-function hardware 的第一性原理解釋：

```text
general-purpose compute 很珍貴
  -> 不應拿來反覆做高度規則、低變化、高頻率的工作
  -> 這些工作適合變成 tiny fixed-function blocks
```

現代 GPU 仍然保留 fixed-function rasterizer，原因就在這裡。

## 和 Pixel-Renderer 的關係

### 現在可以吸收的部分

Pixel-Renderer 近期不該跳去做 Nyuzi H5 hardware rasterizer。現在更有價值的是吸收它的 raster contract：

```text
triangle setup:
  vertices -> edge equations -> bounding box

coverage:
  pixel-center sample
  top-left / half-open rule
  inside test

work granularity:
  current: per-pixel loop
  future: 2x2 or 4x4 patch mask

output:
  current: SetPixel
  future: coverage mask + interpolants + depth test input
```

所以對目前 source 的實作優先順序仍然是：

1. screen-space triangle baseline。
2. edge-function coverage。
3. top-left rule。
4. color interpolation。
5. depth interpolation。
6. depth buffer。
7. deterministic tests。
8. debug view for coverage / barycentric / depth。

paper 的價值是幫我們看見：

> 為什麼 edge-function rasterizer 不只是「另一種 CPU 寫法」，而是和 GPU/hardware rasterizer 的資料流一致。

### 中期可以加入的 debug / test ideas

可以從這篇抽出幾種 test case：

- large fill triangle。
- tiny triangle。
- thin / sliver triangle。
- triangle crossing tile or patch boundary。
- two adjacent triangles sharing an edge。
- triangle completely outside bounding region。
- triangle partially clipped。
- high overdraw / depth occlusion。

也可以加 debug counters：

```text
triangles submitted
pixels tested
pixels covered
empty bbox count
empty patch count
covered patch count
avg active pixels per patch
```

這些 counters 對 software renderer 也有用。它們會讓你看到：

```text
每個 triangle 的成本到底花在哪裡？
coverage test?
interpolation?
depth test?
framebuffer writes?
```

### 長期 FPGA golden model 的啟發

如果未來要在 FPGA 上重現 NV20-like programmable GPU pipeline，這篇提供一個很實用的 golden model boundary：

```text
software golden model:
  fixed-point edge setup
  top-left rule
  patch traversal order
  4x4 coverage mask
  depth/interpolant reference

hardware raster block:
  same input triangle state
  same viewport/sample convention
  same edge bias convention
  same patch coordinates
  output same mask sequence
```

這比「最後 framebuffer image 一樣」更好 debug，因為 framebuffer mismatch 太晚了。更好的比對層級是：

```text
triangle input
  -> edge coefficients
  -> bbox
  -> patch coordinate stream
  -> coverage masks
  -> interpolated fragment values
  -> depth test result
  -> final color
```

如果每一層都有 trace，FPGA block 和 C++ renderer 才容易對。

## 不要誤讀的地方

### 不要把 Nyuzi 結論直接套到一般 CPU renderer

Nyuzi 有 wide vector ALU、tile-based render threads、RTL-level hardware context。Pixel-Renderer 現在是 Win32 DIB + scalar-ish C++ renderer。

所以 paper 的 `26% throughput` 不代表 Pixel-Renderer 改成 4x4 patch 一定也快 26%。

能直接吸收的是 design principle：

```text
coverage generation 是獨立 phase
patch mask 是合理輸出
edge equation 是硬體友善表示
```

### 不要太早做 hardware-like abstraction

現在 Pixel-Renderer 還在建立 trusted raster pipeline。若現在就抽成：

```text
RasterFrontend
PatchScheduler
MaskGenerator
FragmentQueue
ShaderCore
```

會過早。正確節奏是先讓單一路徑可靠：

```text
edge-function triangle
  -> exact convention
  -> tests
  -> depth/interpolation
  -> debug trace
```

等 correctness 和 trace 穩定後，再把 patch/mask boundary 抽出來。

## 建議閱讀順序

如果要快速讀懂 paper，可以照這個順序：

1. Abstract
   - 抓問題設定: Larrabee-style software rasterization overhead。

2. Figure 2, Figure 3, Figure 4
   - 看 rasterizer coprocessor 放在哪。
   - 看 `4x4 patch mask` 是什麼。
   - 看 hardware block 的 phase split。

3. Section IV Experimental Design
   - 抓 `Setup / Iteration / Mask computation / Rejection / Clipping`。

4. H1-H6 descriptions
   - 看每個設計如何 trade area, timing, scalability。

5. Figure 6, Figure 7, Table III
   - 看 L1 / S6 / H5 的 performance relationship。

6. Table IV, Table V
   - 看 area / delay / power。

7. Discussion and Conclusions
   - 抓 fixed-function hardware 為什麼值得。

## 對 Pixel-Renderer 的具體 next note / task

這篇後續可以拆成兩個可執行方向：

### Source-side small step

在 renderer source 裡先不要加 hardware abstraction，只加小型 deterministic counters 或 debug trace：

```text
triangle bbox
edge coefficients
covered pixel count
tested pixel count
```

等 triangle / depth 正確後，再考慮：

```text
4x4 patch coverage debug view
```

### Docs-side stable extraction

等 source 真的開始支援 patch-level trace 時，再把這篇萃取成 stable docs：

```text
docs/verification/pipeline_trace.md
docs/foundations/rasterization_edge_rules.md
docs/architecture/future_hardware_rasterizer_boundary.md
```

現在先留在 learning note 就好，不要過早放進 `main` 的 stable project docs。

