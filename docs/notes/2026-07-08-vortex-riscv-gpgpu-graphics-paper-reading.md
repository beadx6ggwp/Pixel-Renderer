# Vortex paper reading

Paper:

- Blaise Tine, Fares Elsabbagh, Krishna Yalamarthy, Hyesoon Kim.
- "Vortex: Extending the RISC-V ISA for GPGPU and 3D-Graphics Research."
- MICRO 2021.
- Source PDF: [vortex_micro21_final.pdf](https://vortex.cc.gatech.edu/publications/vortex_micro21_final.pdf)

Related note:

- `docs/notes/2026-07-08-nyuzi-raster-paper-reading.md`

## 一句話結論

這篇 paper 的核心不是單一 rasterizer optimization, 而是:

> 如何用最少的 RISC-V ISA extension, 加上一套可跑在 FPGA 上的 hardware/software stack, 做出能支援 OpenCL 和 OpenGL 的 open-source soft GPU research platform.

它和 NyuziRaster 的差別很清楚:

- NyuziRaster 問的是: "software rasterization 太貴時, tiny fixed-function rasterizer 是否值得?"
- Vortex 問的是: "如果要做完整 open GPU research platform, ISA, compiler, runtime, cache, texture unit, PCIe, FPGA scaling 要怎麼一起接起來?"

對 Pixel-Renderer 最重要的啟發是:

> 如果未來要把 software renderer 當 FPGA/GPU pipeline golden model, 不能只想 raster function. 要從 ISA boundary, SIMT execution, memory/cache pressure, texture unit, runtime/driver, trace/simulation stack 一起看.

## 這篇在解決什麼問題

### 問題背景

GPU 是最重要的 accelerator 之一, 但 public domain 的 open-source GPU infrastructure 很少. Paper 認為原因不只是硬體難, 還包括:

- GPU ISA 很複雜.
- GPU software stack 很複雜.
- 商用 GPU 的 ISA / driver / compiler / runtime 多半封閉或高度專有.
- 很多 GPU research 只能靠 simulator, 而且常常停在 PTX / HSAIL 這種 intermediate language level.

這會造成一個問題:

```text
如果只在 IL-level simulator 做 GPU research
  -> 看不到真正 RTL / cache / memory / runtime / power / FPGA resource 的 tradeoff
  -> 對 microarchitecture 的判斷容易被 abstraction 擋住
```

Paper 的解法是 Vortex:

```text
RISC-V ISA + minimal GPU extensions
  -> SIMT soft GPU microarchitecture
  -> FPGA RTL implementation
  -> PCIe host interface
  -> OpenCL software stack
  -> OpenGL / texture / rasterization support
  -> simulation stack for architecture research
```

### 為什麼選 RISC-V

RISC-V 對這篇 paper 的價值不是 "比較潮", 而是:

- ISA open.
- ecosystem 有 LLVM / compiler / tools.
- extension space 讓研究者能加 GPU primitive.
- 若 extension 很少, software ecosystem 的改動就能比較小.

這篇最強的主張之一是:

> 只加六個 RISC-V instructions, 就可以支援 GPGPU SIMT execution 和 3D graphics acceleration 的基本需求.

## Vortex 的定位

Vortex 是一個 PCIe-based soft GPU:

- RISC-V based.
- FPGA implementation.
- 支援 OpenCL.
- 支援 OpenGL graphics path.
- 支援 texture sampling hardware.
- 可 scale 到 Stratix 10 FPGA 上 32 cores.
- 在 200 MHz 時 peak performance 約 25.6 GFlops.

這裡要注意 "soft GPU" 的意思:

```text
不是商用 GPU silicon
也不是純 software simulator
而是可合成到 FPGA 的 GPU-like processor
```

所以 Vortex 很適合作為 architecture research platform:

- 可以改 ISA.
- 可以改 core microarchitecture.
- 可以改 cache subsystem.
- 可以改 texture unit.
- 可以跑 compiler/runtime stack.
- 可以用 simulator / RTL / FPGA 做不同層級驗證.

## Paper 的主要貢獻

Paper 自己列出的貢獻可以整理成五個方向:

1. GPU ISA taxonomy
   - 比較 PTX, AMD RDNA/GCN, Intel GEM, PowerVR.
   - 找出 SIMT GPU 所需的 common primitives.

2. Minimal RISC-V ISA extension
   - 加六個 instructions: `wspawn`, `tmc`, `split`, `join`, `bar`, `tex`.
   - 目標是最小化對 RISC-V ecosystem 的破壞.

3. SIMT microarchitecture + graphics support
   - wavefront scheduler.
   - thread mask.
   - IPDOM stack for divergence/reconvergence.
   - barrier support.
   - texture unit.
   - software rasterization pipeline with hardware texture sampling.

4. FPGA-friendly high-bandwidth cache
   - multi-banked, non-blocking, pipelined cache.
   - virtual multi-porting.
   - MSHR per bank.
   - optimized for FPGA memory constraints.

5. Full stack evaluation
   - FPGA synthesis on Arria 10 and Stratix 10.
   - OpenCL Rodinia benchmarks.
   - texture sampling benchmarks.
   - scaling up to 32 cores.

## Graphics background in this paper

Paper 先把 programmable 3D graphics pipeline 分成:

```text
Geometry stage
  -> vertex shader transforms vertices to screen-space triangles

Rasterization stage
  -> triangles traversed pixel-by-pixel
  -> fragment shader invoked
  -> output color to destination buffer

Texturing
  -> fragment shader samples texture data
  -> point / bilinear / trilinear / mipmaps
```

它也提到兩種 rendering architecture:

```text
Immediate-mode rendering
  triangle primitives are rasterized in produced order

Tile-based rendering
  geometry outputs are subdivided and rasterized per tile
  reduces memory footprint
```

Paper 對 rasterization 的判斷和 NyuziRaster 有呼應:

- 現代 GPU 通常用 fixed-function hardware 做 rasterization.
- 也可以把 graphics stack 用 GPU compute pipeline 軟體化, 但會有 slowdown.
- Larrabee 曾經把大部分 pipeline 軟體化, 只加速 texture sampling.
- Vortex 受 Larrabee 影響, 但因 FPGA area 限制, 選擇只硬體加速 texture sampling.

這裡有一個容易誤讀的地方:

Paper 在第 2 節說 Vortex differs from Larrabee in that only rasterization is offloaded to FPGA. 但後面的 implementation 實際描述是:

```text
host CPU:
  geometry processing

Vortex FPGA accelerator:
  rasterization pipeline as a kernel
  fragment processing
  texture sampling via tex instruction / texture unit
```

所以比較準確的理解是:

```text
geometry stays on host
rasterization/fragment-side work runs on Vortex
texture sampling gets dedicated hardware support
```

## GPU ISA taxonomy

Paper 的 Table 1 比較多個 GPU ISA:

- AMD RDNA.
- AMD GCN.
- NVIDIA PTX.
- Intel GEM.
- PowerVR.
- Vortex.

比較維度包含:

```text
Memory model
Threading model
Register file
Thread control
Synchronization
Flow control
ALU operations
Memory operations
GPU operations
```

這個 taxonomy 的目的不是做百科比較, 而是要回答:

> 一個 SIMT GPU 最少需要哪些 ISA-level primitives?

Paper 的抽象結果大概是:

1. Threading / wavefront control
   - GPU 要能啟動很多 logical threads.
   - 需要 active thread mask 或等價機制.

2. Control-flow divergence / reconvergence
   - SIMT thread 會走不同 branch.
   - 需要 split/join 或 predication/IPDOM-style support.

3. Synchronization
   - wavefront / workgroup / memory ordering 需要 barrier/fence.

4. Memory hierarchy
   - global/shared/local memory.
   - load/store.
   - cache behavior.

5. Graphics operations
   - texture sampling 幾乎所有 GPU ISA 都有某種形式 support.
   - 有些 ISA 也有 interpolate, alpha/depth, pixel iteration 等 graphics-specific instructions.

Vortex 的設計選擇是:

```text
不要完整複製商用 GPU ISA
只保留能支撐 SIMT + graphics texture path 的最低必要 primitive
```

## Vortex 的六個 RISC-V extensions

Table 2 是整篇 paper 的核心表之一:

```text
wspawn %numW, %PC
  Wavefronts activation

tmc %numT
  Thread mask control

split %pred
  Control flow divergence

join
  Control flow reconvergence

bar %barID, %numW
  Wavefronts barrier

tex %dest, %u, %v, %lod
  Texture sampling/filtering
```

### `wspawn`

`wspawn` 用來 activate wavefronts.

直覺上可以想成:

```text
launch many wavefronts at PC
```

它對應 GPU kernel execution 的基本需求: 同一段 program 要被很多 parallel lanes / threads 執行.

### `tmc`

`tmc` 控制 thread mask.

SIMT 的核心不是每個 thread 都有獨立 instruction stream, 而是:

```text
one instruction stream
many lanes
active mask decides which lanes participate
```

`tmc` 就是把這件事 expose 成指令.

### `split` / `join`

`split` 和 `join` 處理 control-flow divergence / reconvergence.

Naive model:

```text
SIMT wavefront 裡 32 個 threads 一起跑
```

Failure point:

```text
if (condition depends on thread id / data)
  some lanes go true
  some lanes go false
```

這時不能單純用 scalar branch. Hardware 必須保存:

- 當前 thread mask.
- branch target.
- reconvergence point.

Vortex 用 `split` 把資訊 push 到 hardware IPDOM stack, 用 `join` pop 出來恢復 thread mask / PC.

### `bar`

`bar` 是 wavefront barrier.

Vortex hardware 裡有 barrier table:

- counter: 還有多少 wavefronts 要到達.
- mask: 哪些 wavefronts 正被 barrier stall.

Barrier ID 的 MSB 還能表示 local/global scope.

### `tex`

`tex` 是 texture sampling/filtering.

它使用 RISC-V R4-type format, 類似 FMA 有三個 source operands:

```text
u
v
lod
```

其他 texture state 由 CSR 設定:

- dimension.
- format.
- filtering mode.
- addressing mode.
- memory address.

這是 Vortex graphics support 的關鍵. 它沒有把整個 graphics pipeline 固定功能化, 而是只把 texture sampling 這個 memory-bound, highly repeated, format/filter-heavy operation 做成 hardware unit.

## Vortex microarchitecture

Figure 4 描述 Vortex microarchitecture. 它基於 standard five-stage in-order RISC-V pipeline, 但加上 SIMT 元件:

```text
Fetch
  wavefront scheduler
  wavefront table
  thread masks
  IPDOM stack
  instruction cache

Decode
  decoder

Issue
  GPRs
  scoreboard
  issue buffer

Execute
  texture units
  GPGPU ALU
  ALU
  FPU
  CSR
  LSU
  data cache
  shared memory

Commit
  writeback
```

### Wavefront scheduler

Fetch stage 的 scheduler 決定每 cycle 要 fetch 哪個 wavefront.

它追蹤幾種 mask:

- active wavefront mask.
- stalled wavefront mask.
- barrier mask.
- visible wavefront mask.

這對 Pixel-Renderer 目前不是實作目標, 但對 GPU architecture learning 很重要:

> GPU throughput 不是只靠 ALU 多, 而是靠 scheduler 不斷從可執行 wavefront 裡挑工作, 用 latency hiding 保持 pipeline 忙碌.

### Thread mask and IPDOM stack

Vortex 不用 predication register 的方式完整模仿某些 GPU ISA, 而是在 hardware 中放:

- thread mask register.
- IPDOM stack.

`split` 時:

```text
current thread mask pushed as fall-through
false-predicate lanes pushed with next PC
true-predicate lanes become active
```

`join` 時:

```text
pop stack
restore thread mask
if needed, resume stored PC
```

這就是 SIMT divergence 的最小硬體化版本.

### Memory system

每個 core 有:

- instruction cache.
- data cache.
- optional shared memory.

多個 cores 可以組成 cluster:

- optional L2 cache.
- clusters 可共享 optional L3 cache.
- flush operation 用來提供 weak coherent memory space.

這裡的 design 很 research-oriented:

```text
cache hierarchy configurable
coherence model not overbuilt
focus on FPGA resource / bandwidth / simulation flexibility
```

## Texture unit

Figure 5 是 texture unit microarchitecture. 它大致分三段:

```text
Texture address generation
  -> compute texel addresses from u, v, lod, CSR states

Texture memory system
  -> de-duplicate repeated accesses
  -> schedule texel memory requests to data cache
  -> gather returned texels

Texel sampler
  -> format conversion
  -> bilinear interpolation
  -> output filtered RGBA per thread
```

支援:

- point sampling.
- bilinear sampling.
- 1D / 2D textures.
- texture formats and wrap modes from OpenGL.

Trilinear filtering 沒有做成單一 hardware primitive, 而是用 pseudo-instruction:

```text
a = tex(stage, u, v, lod)
b = tex(stage, u, v, lod + 1)
return LERP(a, b, FRAC(lod))
```

這是很好的 hardware/software split 範例:

```text
bilinear sampling:
  frequent, core primitive, hardware unit

trilinear sampling:
  built from multiple tex instructions
  keep hardware simpler
```

它的 first-principles reasoning 是:

```text
不要把每個 high-level feature 都硬體化
先找最低層, 最常用, 最固定, 最能被重用的 primitive
```

這點和 Pixel-Renderer 未來的設計很像:

```text
不要太早做完整 material / shader / renderer skeleton
先把 coverage, interpolation, depth, texture sample 這些 primitive 定義穩
```

## High-bandwidth cache

Paper 花很多篇幅在 cache, 因為 GPU workload 的 bottleneck 常常不是 ALU, 而是:

- many parallel memory requests.
- texture sampling memory pressure.
- bank conflicts.
- memory latency hiding.

Vortex 的 cache 是:

- multi-banked.
- non-blocking.
- pipelined.
- each bank has MSHR.
- supports virtual multi-porting.

Figure 6 的 cache pipeline:

```text
Bank selector
  -> assign requests to banks
  -> resolve bank conflicts
  -> coalesce requests mapping to same bank/cache line when virtual ports enabled

Per-bank pipeline
  1. schedule
  2. tag access
  3. data access
  4. response

Bank merger
  -> coalesce outgoing responses by request tag
```

### 為什麼 FPGA cache 難

FPGA memory block ports 有限制. 如果 naive 地想支援 many read/write ports:

```text
need many true memory ports
  -> FPGA BRAM 不支援或成本極高
```

常見方案:

- multi-banking: 分成多個 banks, 但會有 bank conflict.
- multi-pumping: 用更高 clock 做 time-sharing, 但受 clock limit.
- Live-value Table: replicate memory, 但 area/storage cost 高.

Vortex 的 hybrid solution 是:

```text
multi-banking + virtual ports + cache line locality
```

這讓它在 FPGA resource 限制下取得較好的 bandwidth.

### Multi-port cache evaluation

Table 5:

```text
1-port:
  LUT 10747
  Registers 13238
  BRAM 72
  Frequency 253 MHz

2-port:
  LUT 11722
  Registers 13650
  BRAM 72
  Frequency 250 MHz

4-port:
  LUT 13516
  Registers 14928
  BRAM 72
  Frequency 244 MHz
```

Paper 指出:

- 1 -> 2 ports: logic area 約 +9%.
- 1 -> 4 ports: logic area 約 +25%.
- sgemm / vecadd 的 bank utilization 可因更多 virtual ports 提高到接近 100%.
- 2-port configuration 是較好的 balance.

對 renderer 的啟發:

> 當 fragment / texture workload 變大時, memory access pattern 會比 ALU instruction count 更早成為設計核心.

## Elastic pipelines

Vortex 從一開始就用 elastic pipeline design pattern.

目標不是 "performance trick", 而是 research infrastructure:

- 模組更容易接.
- request/response protocol 一致.
- 每個 request 帶 tag.
- tag 可追蹤 instruction PC / wavefront id.
- 容易 trace/debug.
- 容易擴展和替換 units.

Figure 7 顯示 wavefront scheduler 到 instruction cache 到 decode 的 elastic request:

```text
valid
ready
data = PC / instruction
tag = PC, wavefront id
```

對 Pixel-Renderer 的長期啟發:

```text
如果未來有 software pipeline trace 或 FPGA block trace
每個 unit 的 input/output 都應該帶穩定 tag
例如 triangle id, tile id, patch id, primitive id, draw id
```

否則 pipeline 一拆開, debug 會變得很難.

## Software stack

Vortex 不是只有 RTL. 它的 software stack 很完整:

### Driver and host interface

Vortex 透過 PCIe 和 host processor 溝通.

它使用 OPAE:

- configure FPGA.
- read/write instructions and data to FPGA RAM.
- expose FPGA resources as host-accessible features.
- use CCI-P protocol for shared memory space between host and AFU.

這點對 "open GPU infrastructure" 很關鍵:

```text
GPU research platform 不是只有 core
還要有 host interface, command processor, memory transfer, driver API
```

### OpenCL compiler/runtime

OpenCL 是 Vortex 的主要 parallel API.

它改了 POCL:

- support RISC-V target.
- support Vortex instructions.
- integrate Vortex runtime.
- generate Vortex kernel binaries.

OpenCL flow 大致是:

```text
OpenCL source
  -> POCL / LLVM
  -> Vortex target
  -> GPU binary
  -> loaded to FPGA memory
  -> Vortex runs kernel
```

### Graphics support

Vortex graphics API 實作 OpenGL-ES style path:

```text
Host CPU:
  geometry processing

Vortex:
  rasterization pipeline as kernel
  fragment processing
  texture sampling via tex instruction
```

Shader compilation pipeline:

```text
Shader source
  -> LunarGLASS compiler
  -> SPIR-V
  -> SPIR-V-to-LLVM-IR
  -> POCL compiler
  -> GPU binary
```

Paper 的 sample code 展示 fragment-side kernel 會:

- configure texture unit via CSRs.
- setup shader state.
- `spawn_tasks(shader, state)`.

這讓 graphics path 不是完全硬體固定功能, 而是:

```text
raster/fragment tasks run as parallel kernels
texture sampling is accelerated by dedicated unit
```

## Evaluation setup

Host:

- Intel Xeon E5-1650 at 3.5 GHz.

FPGA:

- Intel Arria 10 GX.
- Intel Stratix 10.
- speed grade 2.

Benchmarks:

Compute-bound:

- `sgemm`.
- `vecadd`.
- `sfilter`.

Memory-bound:

- `saxpy`.
- `nearn`.
- `gaussian`.
- `bfs`.

Texture benchmarks:

- point sampling.
- bilinear filtering.
- trilinear filtering.
- 1080p source texture.
- destination render target same size.

## Core configuration tradeoff

Vortex can scale data-level parallelism in two ways:

```text
increase number of threads
  -> similar to increasing SIMD width
  -> more ALUs
  -> wider GPR reads/writes
  -> wider pipeline registers
  -> more arbitration/cache/shared-memory pressure
  -> more IPDOM entries

increase number of wavefronts
  -> more scheduling contexts
  -> no need to increase ALUs directly
  -> larger wavefront table, scoreboards, GPR tables, IPDOM stacks
```

Table 3 compares configurations:

```text
4W-4T:
  LUT 21502
  Regs 32661
  BRAM 131
  f 233 MHz

2W-8T:
  LUT 36361
  Regs 54438
  BRAM 238
  f 224 MHz

8W-2T:
  LUT 16981
  Regs 24343
  BRAM 77
  f 225 MHz

4W-8T:
  LUT 37857
  Regs 57614
  BRAM 247
  f 224 MHz

8W-4T:
  LUT 24485
  Regs 34854
  BRAM 139
  f 228 MHz
```

Key result:

- 2W-8T maximizes threads, area cost rises about 69%, sgemm speedup about 20%.
- 8W-2T maximizes wavefronts, hardware is about 27% smaller, but sgemm IPC drops about 36%.
- 4W-4T chosen as baseline because it balances performance and resource use, allowing scale to 16/32 cores.

這裡的第一性原理:

```text
more lanes
  -> more per-cycle compute
  -> more area and memory arbitration pressure

more wavefronts
  -> more latency hiding
  -> lower ALU replication
  -> but if lanes too few, throughput suffers
```

## FPGA area and scaling

Table 4:

```text
1 core:
  ALM 13%
  Regs 78K
  BRAM 10%
  DSP 2
  fmax 234 MHz
  FPGA A10

2 cores:
  ALM 19%
  Regs 111K
  BRAM 15%
  DSP 5
  fmax 225 MHz
  FPGA A10

4 cores:
  ALM 30%
  Regs 176K
  BRAM 25%
  DSP 9
  fmax 223 MHz
  FPGA A10

8 cores:
  ALM 53%
  Regs 305K
  BRAM 45%
  DSP 19
  fmax 210 MHz
  FPGA A10

16 cores:
  ALM 85%
  Regs 525K
  BRAM 83%
  DSP 38
  fmax 203 MHz
  FPGA A10

32 cores:
  ALM 70%
  Regs 1057K
  BRAM 23%
  DSP 20
  fmax 200 MHz
  FPGA S10
```

Paper 說:

- Arria 10 可放到 16 cores.
- Stratix 10 可放到 32 cores.
- 32 cores at 200 MHz.

Area breakdown 在 8-core Arria 10 時:

- Dcache 約 35%.
- TEX 約 20%.
- FPU 約 13%.
- ALU 約 7%.
- Icache 約 6%.
- GPR buffer / shared memory / GPR 等較小.

這個結果很有意義:

> 對 GPU-like design, cache 和 texture unit 的 area 可能比 ALU 更主導.

這和一般 "GPU = 很多 ALU" 的直覺不同. 真實 pipeline 的成本常常在 memory movement 和 special function units.

## Performance scaling

Figure 18:

- compute-bound benchmarks 隨 core count 幾乎線性成長.
- memory-bound benchmarks 也有成長, 但受 memory system 限制.
- `nearn` 例外, 因為 kernel 裡有 expensive long-latency floating-point square-root, 所以更像 compute-bound.

這裡對 renderer 的啟發:

```text
raster / fragment workload 的 scaling
不是只有 cores x lanes
還要看 texture/cache/bandwidth/depth/framebuffer writes
```

也就是說, 未來若 Pixel-Renderer 做 performance study, 不能只量 triangles/s 或 pixels/s. 至少要拆:

- coverage test.
- interpolation.
- depth test.
- framebuffer write.
- texture fetch.
- cache locality.

## Texture sampling result

Figure 20 比較 hardware texture acceleration vs software.

結論:

- Point sampling 差異很小.
- Bilinear filtering 在 single core 約有接近 2x speedup.
- Core count 增加後 speedup 下降, 因為 memory bandwidth 更容易飽和.
- Trilinear filtering 也有硬體加速 benefit, 但不如 bilinear 明顯, 因為 trilinear 需要更多 memory requests.

Paper 對 point sampling 的解釋很實際:

```text
source texture is RGBA
format conversion unnecessary
software point sampling becomes near-copy
hardware unit advantage small
```

這個結果很好, 因為它避免了 "hardware 一定快" 的簡化說法.

真正的規則是:

```text
operation 越複雜, 越固定, 越 memory/filter-heavy
  -> dedicated texture unit 越有價值

operation 只是 simple copy / nearest fetch
  -> software overhead 可能已經夠低
  -> hardware advantage 不一定大
```

## Memory scaling result

Figure 21 探討 memory latency/bandwidth/L2 cache 對 performance 的影響.

Paper 設定:

- 16-core.
- 16-wavefront.
- 16-thread.
- 用 SIMX 模擬不同 memory config.

圖中比較:

```text
sh:
  24-cycle memory latency

lg:
  200-cycle memory latency

2c:
  2-channel memory

8c:
  8-channel memory

l2:
  L2 cache enabled
```

主要觀察:

- memory-bound workloads 對 bandwidth/cache 更敏感.
- compute-bound workloads 不一定因 memory scaling 大幅受益.
- L2 cache / memory channels 對不同 benchmark 的效果不同.

這對未來 FPGA/GPU route 很重要:

> 一個 renderer 是否快, 不只取決於 rasterizer 算得多快, 還取決於 memory hierarchy 能不能餵得動 texture/depth/color traffic.

## Vortex 和 NyuziRaster 的比較

Table 6 直接把 Vortex 和其他 open-source GPGPUs 放在一起比較.

NyuziRaster:

```text
ISA: Custom
Exec model: SIMT
Cache: L1, L2
Memory system: FPGA
Graphics support: Fixed-function rasterizer
Threads x Cores: 4x1
RTL: Yes
Host interface: N/A
Software stack: Custom
Cycle-level simulation: No
```

Vortex:

```text
ISA: RISC-V
Exec model: SIMT
Cache: shared memory, L1, L2, L3
Memory system: FPGA
Graphics support: Shaders, Texture Units
Threads x Cores: 16x32
RTL: Yes
Host interface: PCIe
Software stack: OpenCL, OpenGL
Cycle-level simulation: Yes
```

Paper 對 NyuziRaster 的定位:

- 有 graphics rendering support.
- simple multi-threaded in-order processor.
- custom ISA.
- no texture unit.
- texture sampling 完全用 software.
- fixed-function rasterizer.
- no programmable shader support.

Vortex 的定位:

- programmable shaders via OpenGL.
- shaders execute as parallel tiles on compute platform.
- hardware accelerated texture sampling.
- scale to 512 total threads on FPGA.

這裡可以接到上一篇 note:

```text
NyuziRaster:
  teaches why tiny rasterizer hardware can be worth it

Vortex:
  teaches how an open GPU research platform needs ISA/runtime/compiler/cache/texture/PCIe/simulation together
```

## Conclusion 的意思

Paper 結論:

- Vortex 利用 RISC-V open ecosystem 和 LLVM/POCL compiler.
- 用 minimal ISA extensions 支援 GPGPU 和 3D graphics.
- 搭配 high-bandwidth caches 和 elastic pipeline, 能在 FPGA 上維持高 frequency.
- Configurable RTL + tightly coupled runtime stack 讓 architecture experimentation 更快.
- Future work 包含 support CUDA and Vulkan APIs, 以及 extend compiler/runtime/software stack.
- ASIC design flow support 是未來 chip fabrication 的 roadmap.

我會把它濃縮成:

> Vortex 的價值不在某個單一 benchmark 贏很多, 而是它讓 GPU research 從 "paper simulator" 更靠近 "可跑 software stack 的 RTL/FPGA platform".

## 對 Pixel-Renderer 的啟發

### 近期不該直接吸收的東西

Pixel-Renderer 現在不該直接模仿 Vortex:

- 不該現在做 RISC-V ISA extension.
- 不該現在做 SIMT scheduler.
- 不該現在做 OpenCL/POCL backend.
- 不該現在做 PCIe-like command processor.
- 不該現在做 texture unit hardware model.

原因很簡單:

```text
Pixel-Renderer current priority:
  trusted raster pipeline
  screen-space triangle
  edge-function/top-left
  interpolation/depth
  tests/debug trace

Vortex priority:
  full-stack open GPU architecture platform
```

兩者層級不同.

### 現在可以吸收的設計觀念

#### 1. Minimal primitive boundary

Vortex 不把整個 GPU ISA 搬進 RISC-V. 它找出六個 primitive.

Pixel-Renderer 也應該用同樣方式想:

```text
不要先做整個 engine abstraction
先找 renderer pipeline 最小不可再拆 primitive
```

例如:

```text
edge setup
coverage test
top-left bias
barycentric / interpolation
depth compare
texture sample
framebuffer write
```

#### 2. Texture sampling 是獨立硬體/模組邊界

Vortex 把 `tex` 當 ISA primitive, 背後接 texture unit.

Pixel-Renderer 目前還不急著做 texture, 但未來可以預先記住:

```text
texture sampling should not be hidden inside arbitrary fragment code forever
```

至少要能獨立觀察:

- coordinates.
- wrap mode.
- filter mode.
- mip level.
- texel format conversion.
- cache/memory behavior.

#### 3. Pipeline trace 要帶 tag

Elastic pipeline 的 tag idea 對 software renderer debug 很有用.

未來 Pixel-Renderer 的 debug trace 可以用:

```text
draw_id
primitive_id
triangle_id
tile_id
patch_id
pixel_coord
sample_id
stage
```

這樣未來 C++ golden model 對 FPGA block 才能逐 stage compare.

#### 4. Memory system 會比想像中早變成主角

Vortex area breakdown 裡 Dcache 和 TEX 很大. Texture benchmark 也顯示 bandwidth 會吃掉 scaling.

對 Pixel-Renderer 來說, 一開始可以忽略 performance. 但當進入:

- texture.
- depth buffer.
- multiple render targets.
- debug visualization.
- tiling.

就要開始拆 memory traffic:

```text
color writes
depth reads/writes
texture reads
attribute reads
trace writes
```

### 長期 FPGA route 的啟發

如果長期目標是 FPGA 上重現 NV20-like programmable GPU pipeline, Vortex 提供另一條路線:

```text
route A: NyuziRaster-like
  fixed-function rasterizer + simple custom soft GPU

route B: Vortex-like
  RISC-V based SIMT soft GPU + minimal ISA extension + software stack

route C: Pixel-Renderer golden model first
  C++ reference pipeline
  deterministic trace
  then extract selected blocks to FPGA
```

Pixel-Renderer 現階段應該走 route C, 但要閱讀 route A/B 的 paper, 因為它們告訴你:

- 哪些 boundary 未來會硬體化.
- 哪些 stage 最需要 trace.
- ISA/runtime/cache/driver 問題會在哪裡冒出來.

## 和上一篇 NyuziRaster note 的連接

這兩篇可以形成一條學習線:

```text
NyuziRaster:
  Why fixed-function rasterization exists.
  Why software rasterization can remain a critical-path cost.
  Why coverage mask / patch-level output is a useful boundary.

Vortex:
  How an open GPU platform needs ISA, SIMT, compiler, driver, cache, texture, simulation, FPGA synthesis.
  Why texture sampling and memory hierarchy become central.
  Why minimal ISA extension can be a sustainable research strategy.
```

如果要把它們整理成 stable docs, 不應該直接搬全文. 比較合理的 stable extraction 是:

```text
docs/architecture/future_gpu_research_map.md
  - Pixel-Renderer golden model
  - NyuziRaster fixed-function lesson
  - Vortex full-stack soft GPU lesson

docs/verification/pipeline_trace.md
  - tags
  - stage-by-stage comparison
  - triangle/tile/patch/pixel trace

docs/foundations/texture_sampling_contract.md
  - later, when texture work actually starts
```

現在先放 `docs/notes/` 是對的.

## 建議閱讀順序

如果之後重讀, 可以照這個順序:

1. Abstract + Introduction
   - 抓 "open GPU infrastructure is missing because ISA/software stack is complex".

2. Table 1
   - 看 mainstream GPU ISA 到底有哪些 common primitives.

3. Table 2
   - 記住六個 Vortex instructions.

4. Figure 4
   - 看 Vortex core 如何把 RISC-V pipeline 變成 SIMT core.

5. Figure 5
   - 看 texture unit split: address generation, memory scheduling, sampler.

6. Figure 6 / Figure 7
   - 看 cache and elastic pipeline.

7. Figure 12 / Figure 13
   - 看 graphics shader compilation and texture setup.

8. Table 3 / Table 4 / Figure 18
   - 看 core configuration and FPGA scaling.

9. Figure 20 / Figure 21
   - 看 texture acceleration and memory scaling.

10. Table 6
   - 把 Vortex 和 NyuziRaster 放到同一張地圖.

## 後續可做的小任務

短期可以從這篇抽一個非常小的 docs follow-up:

```text
docs/notes/2026-07-08-open-gpu-paper-map.md
```

內容只做一張 map:

```text
NyuziRaster
  -> rasterizer fixed-function tradeoff

Vortex
  -> RISC-V SIMT soft GPU full-stack platform

Pixel-Renderer
  -> C++ golden model and trace-first renderer
```

等 source 進到 texture/depth/debug trace 後, 再把 stable conclusion 抽到 `docs/verification/` 或 `docs/foundations/`.
