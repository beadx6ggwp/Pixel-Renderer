# 2026-04-08 對話紀錄

## 專案目標

> 用純 C++ 從零手刻一個軟體光柵化渲染器，不依賴任何圖形 API，逐步推導並實作完整 GPU 渲染管線（Framebuffer → MVP → 可程式化 Shader），再整合前後端分離架構（Command Queue）與 UI 系統（參考 microui / jserv/libiui 自製），最終建成一個具備渲染前端、後端、UI 的小型引擎雛形。

---

## 開發板選擇分析

### 三種板子的差異

| 開發板 | 晶片 | 架構 | 有無 Hardcore CPU |
|---|---|---|---|
| **Tang Nano 9K** | Gowin GW1NR-9 | 純 FPGA + 片上 PSRAM | ❌ 無（可用軟核如 RISC-V） |
| **Arty A7-100T** | Xilinx Artix-7 | 純 FPGA | ❌ 無（可跑 MicroBlaze 軟核） |
| **Zynq7020** | Xilinx Zynq-7020 | **ARM Cortex-A9（PS） + FPGA（PL）** | ✅ 有，真正的 SoC |

### 兩種路線

- **純 FPGA 路線**（Tang Nano 9K、Arty A7-100T）：CPU 要自己在 FPGA 邏輯裡實作軟核（soft-core），適合學習從底層蓋起
- **SoC 路線**（Zynq7020）：ARM 跑 Linux/驅動，FPGA 做硬體加速核心，PS ↔ PL 透過 AXI 溝通

### 自製 NV 架構顯卡的建議路線

| 階段 | 開發板 | 目標 |
|---|---|---|
| 0. 熟悉 FPGA 基礎 | Tang Nano 9K（$20） | HDMI 時序、點一個像素到螢幕，便宜試錯 |
| 1. 主力開發 | Arty A7-100T | 完整 GPU 管線，對應 Jserv 課程 + Raster-I 架構 |
| 2. 進階（選修） | Zynq7020 | 跑 Linux + GPU 驅動，學 DMA / 驅動程式開發 |

---

## Raster-I 分析

**結論：NV10（GeForce 256, 1999）等級——硬體 T&L + 固定 Phong，沒有可程式 Shader**

| 功能 | 有無 |
|---|---|
| 硬體頂點變換 T&L | ✅ 固定電路 |
| 固定光柵化（Edge Function / Pineda style） | ✅ 8 條並行 interpolator pipeline |
| 固定 Phong 光照 | ✅ Hardwired，不可改 |
| Tile-Based 架構（TBDR） | ✅ 64×32 tile |
| MSAA 4x | ✅ |
| Texture Sampling | ⚠️ Optional，BRAM 幾乎滿了 |
| 可程式 Vertex Shader | ❌ 明確標注為未來工作 |
| 可程式 Fragment Shader | ❌ 明確標注為未來工作 |
| GPGPU ISA | ❌ 明確標注為未來工作 |

**資源消耗（A7-100T）**：LUT 69%、BRAM 97%、DSP 88%

**注意**：Raster-I 用的是 **TBDR**（類 PowerVR / Apple GPU），不是 NVIDIA 的 **IMR（Immediate Mode Rendering）**架構。

---

## 真實 NVIDIA 顯卡架構

```
┌─────────────────────────────────────────────────────┐
│              應用層 / Game / 3D App                  │
│         glDrawArrays()  /  vkCmdDraw()              │
└─────────────────┬───────────────────────────────────┘
                  │ 標準化 API 呼叫
┌─────────────────▼───────────────────────────────────┐
│           圖形 API 層                                 │
│   OpenGL / Vulkan / DirectX / Metal                 │
└─────────────────┬───────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────┐
│           驅動程式層  (NVIDIA 閉源核心)               │
│   • 把 API 呼叫翻譯成 GPU 硬體指令集                 │
│   • 把 GLSL Shader 編譯成 GPU 原生機器碼             │
└─────────────────┬───────────────────────────────────┘
                  │ PCIe 匯流排傳輸
┌─────────────────▼───────────────────────────────────┐
│           GPU 硬體層                                  │
│   Command Processor → SM → Rasterizer → ROP         │
│   Texture Unit → VRAM → Display Engine              │
└─────────────────────────────────────────────────────┘
```

**對應到你的專案**：

| NVIDIA 真實系統 | Pixel-Renderer |
|---|---|
| GLSL vertex shader | `IShader::vertex()` |
| GLSL fragment shader | `IShader::fragment()` |
| OpenGL `glDrawArrays()` | `queue.push(DrawTriangleCmd)` |
| GPU Driver（翻譯指令） | `CommandQueue::execute_all()` |
| Rasterizer Unit | `Rasterizer::DrawTriangle()` |
| ROP / Z-Test | `RenderDevice::SetPixel()` + Z-Buffer |
| VRAM Framebuffer | `FramebufferConfig::buffer` |
| Display Engine + BitBlt | `ScreenManager::BitBlt()` |

---

## 早期 NV 架構世代對照

| 世代 | 架構 | 特徵 | FPGA 可行性 |
|---|---|---|---|
| NV10（GeForce 256, 1999） | 固定管線 T&L | 硬體矩陣乘法，無可程式 Shader | ✅ 可行 |
| NV20（GeForce 3, 2001） | 可程式 VS + 固定 FS | VS 1.1，~128 指令集 | ⚠️ 難但可做 |
| NV30（GeForce FX, 2003） | 完整可程式 VS + PS | PS 2.0，浮點精度 | 🔴 很難 |
| G80（GeForce 8, 2006） | 統一 Shader / CUDA | 現代 NVIDIA 起點 | 🔴 不可能單 FPGA |

**務實目標：NV20 世代**（可程式 VS + 固定光柵化 + 可程式 FS）

---

## 固定管線 vs 可程式化管線

### 固定管線（Fixed-Function Pipeline）

硬體電路寫死，只能調整輸入參數，不能改變計算邏輯。

```
頂點 → [矩陣乘法電路] → [光照電路(只能算 Phong)] → [紋理電路] → 像素
            ↑                    ↑                      ↑
        只能調參數            只能調參數              只能調參數
```

早期 OpenGL 1.x 的 `glLightfv()`、`glMaterialfv()` 就是這個概念。

### 可程式化管線（Programmable Pipeline）

硬體裡有一顆可以執行任意指令的小處理器，你傳入的是程式碼。

```
頂點 → [Vertex Shader 處理器] → [光柵化（固定）] → [Fragment Shader 處理器] → 像素
               ↑                                              ↑
        你傳入一段程式碼                                 你傳入一段程式碼
```

### 光柵化為什麼永遠是固定電路

「把三角形轉成像素」邏輯永遠一樣：包圍盒掃描、重心座標判斷、Z-Test。做成固定電路速度最快、面積最小。真正需要可程式化的只有：**頂點怎麼變換** 和 **像素顏色怎麼算**。

---

## 自製 NV20 級 GPU 的挑戰

### 挑戰 1：Shader Unit 本質是一顆 CPU，需要設計 ISA

```
// 類 NV20 VS 1.1 Assembly
DP4  R0.x, v0, c0   // dot product：頂點位置 × MVP 矩陣 row0
DP4  R0.y, v0, c1
DP4  R0.z, v0, c2
DP4  R0.w, v0, c3
MOV  oPos, R0        // 輸出到裁切空間
MAD  R1, v2, c8, c9  // 光照計算：N·L
```

NV20 的暫存器設計：
- `v0~v15`：Input registers（頂點屬性，只讀）
- `R0~R11`：Temp registers（計算用，讀寫）
- `c0~c95`：Constant registers（MVP 矩陣等，CPU 寫入）→ 對應你的 `uniforms`
- `oPos`：Output position
- `o0~o7`：Output registers → 對應你的 `varyings`

### 挑戰 2：ISA 設計的關鍵決策

**純量 vs 向量**
- 純量（RISC-V 風格）：電路簡單，MVP 需要 16 條指令
- 向量（NV20 風格）：一條指令處理完整 vec4，但需要 swizzle 電路

**指令格式**：一定選固定長度（32-bit），可變長在 FPGA 上幾乎不可行

### 挑戰 3：軟體和硬體是不同的計算模型

你的軟體 `for loop` 不能直接搬到 FPGA，要重新設計成流水線狀態機（Pipeline FSM）。

### 挑戰 4：A7-100T 放不下「完整 GPU 管線」

切換到簡單 IMR（移除 TBDR、關 MSAA、降解析度）後，A7-100T 約剩 50~60% 資源可用，夠放單核 Shader Unit。

---

## 並行度的三個層次

```
Level 1：指令級並行（ILP）
  流水線化，4 stage pipeline
  提速：~2x

Level 2：資料並行（SIMD）
  一條指令同時處理 N 個像素
  4-wide SIMD → 需要 4 組 DSP block
  提速：~4x（NV20 的做法）

Level 3：執行緒並行（SIMT）
  N 個 thread 同時執行，各自有獨立暫存器組
  等 texture fetch 延遲時切換 thread（隱藏延遲）
  GeForce 3 引入，G80 完整實作為 Warp（32 threads）
```

**A7-100T 務實目標**：Level 1（流水線）+ Level 2（4-wide SIMD），不做 Level 3。

---

## 工具鏈路徑

### Shader 編譯鏈

```
【最小可行路線】
手寫 Shader Assembly（.vsa 格式）
    → Python Assembler（~200 行）
    → ISA binary（.bin）
    → FPGA 執行

【未來升級路線】
GLSL 原始碼
    → glslang（前端）
    → SPIR-V
    → 你的後端編譯器
    → 你的 ISA binary
```

### 硬體驗證流程

```
Step 1：仿真優先（Simulation First）
  C++ 軟渲染器產生「黃金向量」（Golden Vector）
  Vivado Simulator 跑 HDL，比對輸出

Step 2：ILA（Integrated Logic Analyzer）
  Vivado 內建硬體示波器，插入探針抓波形

Step 3：逐像素對拍
  FPGA 渲染完一幀 → 透過 UART 傳回 PC
  PC 軟渲染器渲染同一幀，逐像素比對

Step 4：漸進式整合
  先驗證光柵化（不接 Shader）
  再驗證 Shader Unit（不接光柵化）
  最後串接
```

---

## 顯卡與渲染技術演進史

### 第零代：CPU 純軟體渲染（1980s~1996）

```
代表作：
  id Software《Wolfenstein 3D》(1992)
  id Software《DOOM》(1993)           — BSP Tree + 假 3D
  id Software《Quake》(1996)          — 真正的 3D，純 CPU 軟渲

Carmack 的貢獻：
  BSP Tree                → 你的 Frustum Culling 前身
  Surface Caching         → Lightmap 的起源
```

### 第一代：2D 加速卡 → 固定 3D 管線（1995~1998）

```
1995：S3 Virge、Matrox Mystique
  功能：硬體加速 2D（BitBlt）
  對應你的 ScreenManager::BitBlt()

1996：3dfx Voodoo 1（第一張真正的 3D 加速卡）
  功能：硬體三角形光柵化 + 貼圖採樣 + Z-Test
  API：Glide（3dfx 私有 API）
  對應你的 Rasterizer::DrawTriangle()

1997：OpenGL 1.0 普及、Direct3D 3.0
  第一次有跨廠商標準 API
  對應你的 CommandQueue 第一次有了標準格式
```

### 第二代：T&L 硬體化（1999~2000）

```
1999：NVIDIA GeForce 256
  革命：Transform & Lighting (T&L) 移到硬體
  之前：CPU 做矩陣乘法（幾乎佔滿 CPU 資源）
  之後：顯卡內建固定電路做矩陣乘法
  對應你的 math_utils.h 裡的 mat4*vec4 → 電路實現

  API：OpenGL 1.2、Direct3D 7
  設定：glMatrixMode(GL_MODELVIEW); glLoadMatrixf(...)
  → 只能傳矩陣數值，不能改計算方式（固定管線）
```

### 第三代：可程式化 Vertex Shader（2001）

```
2001：NVIDIA GeForce 3（NV20）
  革命：Vertex Shader 可程式化（VS 1.1）
  最多 128 條指令
  指令集：MAD、DP4、MOV、MUL、ADD、RCP
  暫存器：12 temp、16 input、96 constant
  → 這就是你要為 FPGA 設計的 ISA

  Fragment 端：還是固定管線（register combiner）

  你的軟體對應：
    IShader::vertex()  =  GeForce 3 的 VS Unit
    uniforms           =  Constant Registers (c0~c95)
    varyings           =  varying output (o0~o7)

同年 ATI Radeon 8500：
  可程式 VS + 可程式 FS（PS 1.4，但只有 8-bit integer 精度）
```

### 第四代：Fragment 也可程式化（2002~2003）

```
2002：NVIDIA GeForce FX（NV30）
  Fragment Shader 可程式化（PS 2.0）
  浮點精度（16-bit / 32-bit float）← 重大突破
  最多 1024 條 Fragment 指令

  你的 IShader::fragment() 裡的 Phong 計算
  = NV30 在每個像素執行的 PS 程式碼

2002：ATI Radeon 9700（R300）
  DirectX 9 第一張完整支援的顯卡
  NV30 設計失敗（發熱大、慢），這場戰役 ATI 贏了

  API：DirectX 9、GLSL（第一版）
  第一次可以用高階語言寫 Shader，不用寫 Assembly
```

### 第五代：統一著色器架構（2006）

```
2006：NVIDIA GeForce 8800 GTX（G80）
  革命：Unified Shader Architecture

  之前（分離架構）：
    VS Unit N 個 + FS Unit M 個，比例固定，常有一邊閒置

  之後（統一架構）：
    128 個「Stream Processor」全部一樣
    複雜場景跑頂點、簡單場景跑像素，動態分配

  引入 CUDA（2007）：
    Stream Processor 可跑任意並行程式
    GPU 從「圖形專用」→「通用並行計算器」

  架構概念：
    SM（Streaming Multiprocessor）：32 個 SP
    Warp：32 個 thread 一起執行（SIMT 正式確立）
```

### 渲染技術平行演進

**光照模型**
```
Flat Shading (1970s)   → 每個三角形一個顏色
Gouraud (1971)         → 逐頂點光照，插值到像素（Ch28）
Phong (1975)           → 逐像素光照（Ch30）
Texture Mapping (1974) → 貼圖替代純色（Ch29）
Bump Mapping (1978)    → 法線貼圖前身
Normal Mapping (1990s) → 用貼圖儲存法線（Ch31）
PBR (2010s)            → 基於物理的渲染
```

**陰影技術**
```
無陰影 (pre-1996)
Shadow Volume (1977)   → 幾何方法，精確但費時
Shadow Mapping (1978)  → 兩趟渲染（Ch33）
PCF (1987)             → 軟陰影
PCSS (2005)            → 接觸漸硬陰影
CSM (2006)             → 大場景分層陰影
RTRT (2018~)           → RTX 即時光追陰影
```

**渲染架構**
```
Forward Rendering      → O(objects × lights)
Deferred (2004)        → G-Buffer，O(pixels × lights)
Tiled Deferred (2010)  → 螢幕切 tile，O(pixels × local lights)
Clustered (2015)       → 3D 空間分格
Forward+ (2012)        → Forward + Tile 預剔除
```

**抗鋸齒**
```
No AA → SSAA → MSAA (Ch34) → FXAA → SMAA → TAA → DLSS → FSR
```

### 近代：光追與 AI（2018~）

```
2018：RTX 2000（Turing）
  RT Core：BVH 遍歷硬體化
  Tensor Core：矩陣乘法加速（AI 推理）
  DLSS 1.0：AI 超取樣

2020：RTX 3000（Ampere）
  DLSS 2.0：真正可用的 AI 超解析度

2022：RTX 4000（Ada Lovelace）
  DLSS 3.0：AI 生成中間幀（Frame Generation）

2023~：
  Path Tracing 實用化
  Nanite（UE5）：無限多邊形
  Lumen（UE5）：即時全域光照
```

---

## 你的軟體渲染器在歷史上的位置

```
1992  Quake 軟渲           ← 你在做的事（但更系統化、有完整理論框架）
1996  3dfx Voodoo          ← 你的 Rasterizer
1999  GeForce 256 T&L      ← 你的 math_utils MVP
2001  GeForce 3 可程式 VS  ← 你的 IShader::vertex()
2002  GeForce FX 可程式 FS ← 你的 IShader::fragment()
2006  G80 統一架構          ← FPGA 路線的遠期目標
2018  RTX 光追              ← 知識邊界的盡頭
```

---

## 知識地圖

```
你目前的位置
     │
     ▼
[ 完成軟體渲染器 ]          ← 你正在做
  IShader、CommandQueue、Texture、Phong、Shadow
     │
     ▼
[ 數位邏輯基礎 ]
  Verilog / SystemVerilog
  組合邏輯、時序邏輯、流水線
     │
     ▼
[ 自製 RISC-V CPU ]         ← Jserv 課程 / ICLAB Project 1
  理解處理器 = 理解 Shader Unit
     │
     ├──────────────────────────┐
     ▼                          ▼
[ Shader ISA 設計 ]    [ 固定管線光柵化 RTL ]
  MAD/DP4/TEX            Bounding Box 電路
  Python Assembler        Barycentric 流水線
     │                          │
     └──────────┬───────────────┘
                ▼
     [ AXI 匯流排介面 ]   ← ICLAB Project 2
       CPU ↔ GPU 通訊, DMA
                │
                ▼
     [ VDMA + HDMI 輸出 ]
                │
                ▼
     [ 軟硬對拍驗證 ]
                │
                ▼
     [ 驅動程式 ]
       裸機 C，CommandQueue 後端替換
                │
                ▼
     [ VLSI/ICLAB 課程 ]
       RTL → 合成 → APR → Tapeout
                │
                ▼
     [ 自製 GPU ASIC ]
```

---

## 開源 GPU 參考資料

| 專案 | 架構 | 特點 | 你的用途 |
|---|---|---|---|
| [raster-gpu/raster-i](https://github.com/raster-gpu/raster-i) | TBDR，NV10 等級 | Chisel HDL + Vitis HLS，A7-100T | 光柵化流水線參考 |
| [gplgpu](https://github.com/asicguy/gplgpu) | 固定管線 | 較完整但複雜 | 進階參考 |
| [TinyGPU](https://github.com/adam-maj/tiny-gpu) | 極簡 CUDA-like | 最易讀 | 理解 SIMT 概念 |
| [MIAOW GPU](https://github.com/VerticalResearchGroup/miaow) | AMD GCN 開源實作 | 學術研究用 | GPU 微架構研究 |

---

## 台灣學術路線

### 你想接觸的橫跨三個領域

```
圖形學 / 渲染演算法     → 資工系 (CS)
GPU 微架構 / RTL 設計   → 電機系 (EE)
VLSI / ASIC / 流片      → 電機系 (EE) + 專門實驗室
```
### 現實問題

- 電機所推甄：本校優先，外校考試科目是電子學、電路學、工程數學
- 資工所考試：六科（離散、資結、演算法、計組、OS、程設）
- **選指導教授比選學校重要**

### 建議

```
第一志願：成大資工（Jserv 或圖形組）
           資工六科，你的交叉點最多
第二志願：交大資工圖形組
第三志願：台大資工圖形 / 架構組
```

**你的 Pixel-Renderer 作品集**（GitHub 上有完整軟體光柵化 + 教學文件 + 架構設計）在資工所推甄裡是相當強的差異化優勢。

---

## 今日核心結論

1. **你的軟體渲染器是整條路的地基**，它同時是 GPU 的「行為規格書」和「硬體測試工具」
2. **目標是 NV20 等級（可程式 VS + FS）**，A7-100T + IMR 架構 + 4-wide SIMD 可以做到
3. **軟硬體是平行的兩套系統**，不是「後端換掉就完成」，需要用 Verilog 重新設計流水線
4. **驗證方法論**：先仿真 → ILA 抓波形 → 逐像素對拍，軟體渲染器就是你的 Golden Model
5. **台灣升學**：資工所六科考試 + 選對教授 > 考電機所；Pixel-Renderer 是有效的備審資料
