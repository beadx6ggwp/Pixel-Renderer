# C++ Tutorial Audit

這份 audit 是對 `docs/tutorial-cpp/` 的第二輪課程設計審查。它不是新的教學章節，而是接下來重寫、加深、補章節、整理 index / README 的依據。

目前狀態可以先這樣判斷：

```text
T-01 ~ T-10
  已接近正式章節
  但早期章節是在 SVG / memory-style 指示之前完成
  需要看情況補圖、補架構記憶點、補跨章地圖

T-11 ~ T-14
  目前是骨架版
  需要加深到正式教學密度

I-02 ~ I-13
  多數已接近正式章節
  I-12 已改為 cross-platform 命名
  I-13 是目前架構主線的核心章

I-14 ~ I-15
  目前是骨架版
  需要補完整資料流、API sketch、失敗案例、圖示
```

## 1. 這輪審查的判準

每章不應只是有標題和 API 清單，而要滿足下面幾個判準：

| 判準 | 說明 |
|---|---|
| concrete case | 開頭要有具體疑問或專案場景，不要直接進名詞定義。 |
| naive model | 要明確說初學者或目前專案直覺會怎麼想，以及為什麼不夠。 |
| mechanism | 要拆到底層實際發生什麼：memory、compiler、process、buffer、dispatch、build graph。 |
| semantic rule | 要收束成 C++ 語意、contract、ownership、lifetime、invariant 或 cost model。 |
| renderer consequence | 要回到 Pixel-Renderer 的 framebuffer、backend、UI、shader、toolchain。 |
| verification | 要說怎麼檢查這件事是否真的成立。 |
| visual support | 關鍵段落可用 SVG 或 ASCII；不要求每章都有 SVG，但 buffer/lifetime/pipeline/build graph 這類結構應優先圖解。 |

## 2. 目前不列為近期主線的內容

### 2.1 FPGA 暫時不進 tutorial-cpp 主線

長遠目標仍然可以把 software renderer 當 golden model，但這份 C++ 教學近期不應把篇幅拉去 FPGA。

保留方向：

- shader pipeline interface 可以提到「stage-by-stage 可驗證」。
- testing / golden image 可以先服務 software renderer。
- 不在近期新增 FPGA-specific 章節。

### 2.2 圖學數學不放進這份 tutorial

`tutorial-cpp` 的 Implementation Track 應關注工程化，不重推：

- edge function
- barycentric coordinates
- clipping math
- texture filtering
- perspective-correct interpolation

這些應放在 rendering / graphics 專門筆記。

## 3. 應新增的章節

### I-01 Current Renderer Architecture Walkthrough

已補成正式新版章節。它取代舊版 `impl/ch01_renderer_walkthrough.html` 在新版 track 裡的入口位置，避免 Implementation Track 從 toolchain 開始卻缺少專案現況地圖。

檔案：

```text
docs/tutorial-cpp/impl/i01_current_renderer_architecture_walkthrough.html
```

應回答：

- 目前 `src` 裡有哪些主要 module？
- `Application`、`ScreenManager`、`RenderDevice`、`Rasterizer` 的關係是什麼？
- framebuffer pointer 現在從哪裡來？
- Win32 message loop、DIBSection、BitBlt 目前在哪裡？
- 哪些地方是 architecture debt，不是單純命名問題？
- 後續 I-08 ~ I-13 為什麼要這樣拆？

建議視覺：

- SVG：current architecture graph。
- ASCII：current frame loop。
- table：現有 class responsibility vs 目標 responsibility。

### I-16 Verification / Golden Image / Regression Testing

使用者已明確指出 FPGA 先不用，但測試驗證需要補。這章已補成新的 implementation 主線。

檔案：

```text
docs/tutorial-cpp/impl/i16_verification_golden_image_regression_testing.html
```

應回答：

- software renderer 怎麼測？
- 為什麼只看螢幕不算測試？
- golden image test 是什麼？
- deterministic rendering 需要哪些條件？
- 哪些測試應該 exact match，哪些需要 tolerance？
- 如何測 line / triangle / depth / blend / UI draw data？
- 如何建立 debug dump：framebuffer、depth buffer、command buffer、intermediate stage？
- 未來 CI 怎麼跑 renderer tests？

建議章節：

```text
1. naive model：畫面看起來對就算對
2. 為什麼 renderer 需要 golden output
3. deterministic input：固定解析度、固定 clear color、固定 draw commands
4. golden image：儲存與比較 framebuffer
5. exact match vs tolerance
6. stage-level tests：SetPixel / DrawLine / triangle / depth / blend
7. command-buffer tests：輸入 commands，驗證 output
8. debug dump：失敗時不要只說 image mismatch
9. CI / local workflow
10. Pixel-Renderer 第一版測試清單
```

建議視覺：

- SVG：test pipeline `Draw commands -> Renderer -> Framebuffer -> Comparator -> Report`。
- ASCII：golden image mismatch report。

### I-17 Header Boundary / Dependency Hygiene

目前已有 CMake、backend boundary，但 C++ 專案會長期被 include dependency 污染。這章已獨立補成正式章節。

檔案：

```text
docs/tutorial-cpp/impl/i17_header_boundary_dependency_hygiene.html
```

應回答：

- public header vs private implementation 差在哪？
- 為什麼 `windows.h` / `SDL.h` 不應出現在 core header？
- forward declaration 何時可用，何時不可用？
- include path 是 build graph 的一部分，不是 editor 設定。
- pimpl 是否需要？第一版應不應用？
- target dependency 如何反映 source dependency？

建議視覺：

- SVG：allowed dependency direction。
- table：header 可以 expose / 不應 expose 的東西。

### Optional: T-15 Testing Semantics / Determinism

先不急著新增。若 I-16 寫太大，可以拆出 theory：

```text
T-15 Testing Semantics / Determinism / Floating Point Tolerance
```

但目前建議先把 testing 放在 `I-16`，因為需求更偏工程實作。

## 4. 應優先加深的章節

### P0：必須加深

#### T-11 Runtime Polymorphism / Type Erasure / Variant

狀態：已加深第一輪。原本只有 164 行、偏摘要；目前已補 dispatch decision map、vtable mental model、template/variant/type-erasure tradeoff、command buffer value semantics 與驗證方式。後續若再加深，可補更完整的手寫 type erasure 範例與 object slicing 節。

已處理：

- vtable 的實際 memory model：object pointer、vptr、vtable、indirect call。
- `unique_ptr<Base>` ownership 語意。
- `std::function` 可能 allocation / inline 困難。
- `std::variant` memory layout、visitor、closed set tradeoff。
- Pixel-Renderer dispatch map 應加更多反例：
  - 為什麼 DisplayBackend virtual OK。
  - 為什麼 Fragment shader 不該每 pixel virtual。
  - 為什麼 UI command buffer 不應存任意 lambda。

建議 SVG：

- vptr/vtable dispatch 圖。
- variant as tagged union 圖。

#### T-12 Low-level Performance / Memory Layout

狀態：已加深第一輪。原本只有 146 行、偏摘要；目前已補 memory hierarchy、cache line pixel model、pitch-aware address、AoS/SoA access pattern、branch hoisting、aliasing、measurement workflow。後續若再加深，可補 struct padding/alignment 的具體 byte layout 與更多 benchmark 範例。

已處理：

- cache hierarchy mental model：L1/L2/L3/main memory。
- cache line 圖：framebuffer scanline vs random access。
- pitch/alignment 更具體化。
- struct padding / alignment 範例。
- AoS vs SoA 用 vertex transform 具體比較。
- branch prediction 以 triangle edge test / depth test 舉例。
- simple profiling workflow：debug vs release、固定 workload、計時範圍。
- aliasing / restrict-like reasoning 可先介紹概念，不必深入 compiler flags。

建議 SVG：

- cache line + scanline write。
- AoS vs SoA memory layout。

#### I-14 Immediate-mode UI Integration Boundary

狀態：已加深第一輪。原本只有 146 行、偏摘要；目前已補 immediate-mode UI state model、widget ID、layout cursor、draw command value semantics、clip/alpha/font atlas、ImGui software backend、UI verification workflow。後續若再加深，可補 widget state timeline SVG 與更多 pseudo-code。

已處理：

- immediate-mode UI 的真正 state model：
  - `hot`
  - `active`
  - `focused`
  - app-owned widget value
  - per-frame draw list
- widget ID / ID stack。
- layout cursor。
- clipping / scissor。
- text rendering / font atlas。
- alpha blending。
- UI command buffer lifetime。
- ImGui 的 `ImDrawData` 結構更完整：
  - command lists
  - vertex/index buffers
  - texture id
  - clip rect
- 和自製 UI 的差異。

建議 SVG：

- UI frontend -> draw list -> UI software renderer -> framebuffer。
- widget state timeline。

#### I-15 Shader Pipeline Interface Boundary

狀態：已加深第一輪。原本只有 147 行、偏摘要；目前已補 shader pipeline stage boundary、DrawCall/PipelineState/resource binding、shader dispatch choice、output merger、command buffer、stage-level verification dump。後續若再加深，可補 varying interpolation data layout 與 material registry 範例。

已處理：

- draw call 結構：mesh、transform、shader、pipeline state、resources。
- vertex shader input/output。
- clipping / primitive setup 與 shader boundary。
- varying interpolation 的資料結構，不重推數學但要說 interface。
- fragment shader input/output。
- depth/blend/output merger。
- shader dispatch choices：template、variant、type erasure、material id。
- CPU golden model 先改成「renderer verification model」，不要把 FPGA 當近期主題。

建議 SVG：

- programmable pipeline stage diagram。
- DrawCall -> pipeline state -> framebuffer data flow。

### P1：應加深，但不一定先做

#### T-13 Concurrency / Memory Model / Job System

狀態：已加深第一輪。已補 data race / happens-before / tile ownership / false sharing / worker pool / deterministic verification。後續若再加深，可補 atomic acquire-release 的小例子與更完整 job queue pseudo-code。

已處理：

- C++ data race 的更精準定義。
- happens-before。
- mutex lock/unlock 的 visibility。
- atomic acquire/release 只作為概念，不深挖 memory order。
- tile-based rasterization 的 race case。
- depth buffer parallelization 的困難。
- job queue / worker pool 最小模型。

#### T-14 Modern C++ Boundaries

狀態：已加深第一輪。已補 feature adoption map、use-now / understand-first / delay-in-core 分層、toolchain boundary、project standard、feature-by-layer policy。後續若再加深，可補具體 compiler support matrix 與 expected-like Result polyfill 範例。

已處理：

- C++17 / C++20 / C++23 feature matrix。
- 目前專案建議標準版本。
- compiler support：MSVC / MinGW GCC / Apple Clang / Ubuntu GCC。
- 為什麼 modules/coroutines 暫緩。
- `std::expected` 是否需要 polyfill。
- `std::pmr` 對 UI command buffer 的未來價值。

### P2：前段章節可選擇補圖或局部重寫

因為 SVG / memory-style 指示是在後面才補上，前面章節可做選擇性重寫，而不是全部推倒。

#### T-01 Compilation / Linkage / ABI

可補：

- SVG：source -> translation unit -> object file -> linker -> executable。
- SVG：symbol / relocation / library resolution。
- 更明確的 memory anchor：program as binary artifact，不只是 compiler steps。

#### T-02 Type System / Initialization

可補：

- SVG：invalid raw values -> constructor -> valid state。
- 更強調 type as boundary / invariant。
- 可補更多 aggregate vs invariant-protecting class 的對照。

#### T-03 Object Model / Storage / Lifetime / UB

已經重要，但可補：

- SVG：storage vs object lifetime。
- SVG：alignment / padding。
- 更清楚連到 framebuffer memory 與 backend staging memory。

#### T-04 Value Category / Reference System

可補：

- SVG：expression has type + value category two-axis model。
- value category 對 operator return、lambda capture、UI command dangling 的圖。

#### T-05 Copy / Move / Object Transfer

可補：

- SVG：copy vs move ownership graph。
- moved-from state 圖。
- copy elision timeline。

#### T-06 RAII / Resource Ownership / Exception Safety

可補：

- SVG：constructor/destructor stack unwinding。
- resource graph：Application owns backend/framebuffer/device/rasterizer。

#### T-07 Callable / Overload / Lambda

可補：

- SVG：lambda closure object。
- capture by value/reference lifetime 圖。

#### T-08 Templates / Concepts / Type Traits

可補：

- SVG：template requirement -> concept -> instantiation。
- CRTP / policy 的 dispatch map。

#### T-09 STL / Containers / Iterators

已經有 SVG，優先度低。可以之後補 map/unordered_map tradeoff。

#### T-10 Error Handling / Contracts

已有 contract SVG。可之後補 expected/result state machine。

## 5. Index / README 整理問題

### 5.1 index 已改成 Part-based reading path

`index.html` 已從長清單整理成 Part-based reading path：

```text
Part 0 Program as Binary
  T-01, I-02, I-03, I-04, I-05, I-06, I-07

Part 1 Object Exists in Storage
  T-02, T-03, T-06

Part 2 Values Move Through the Program
  T-04, T-05, T-07

Part 3 Types Become Generic Contracts
  T-08, T-09, T-10

Part 4 Architecture Boundaries
  T-11, I-08, I-09, I-10, I-11, I-12, I-13

Part 5 Performance and Future Systems
  T-12, T-13, T-14, I-14, I-15, I-16, I-17
```

### 5.2 Legacy chapters 已移除

舊 `Ch01` ~ `Ch18` 已被新版 track 吸收，不再保留 HTML 檔或 index 入口。正式閱讀順序以新版 T/I track 為主。

### 5.3 README 目前先不用動

使用者前面提過 README 先不動。這份 audit 只標記未來需要，不直接修改 README。

## 6. 建議執行順序

### Phase A：補缺口

狀態：已完成第一版。

1. `I-01 Current Renderer Architecture Walkthrough`。
2. `I-16 Verification / Golden Image / Regression Testing`。
3. `I-17 Header Boundary / Dependency Hygiene`。

理由：這三章是結構缺口，不只是加深。尤其 I-16 是使用者明確指出需要的測試驗證主線。

### Phase B：加深骨架章

1. `T-11` 已加深第一輪。
2. `T-12` 已加深第一輪。
3. `I-14` 已加深第一輪。
4. `I-15` 已加深第一輪。
5. `T-13` 已加深第一輪。
6. `T-14` 已加深第一輪。

理由：這些章節目前行數與推理深度明顯低於前半。

### Phase C：前段補圖與局部重寫

1. `T-03` storage/lifetime SVG 已補。
2. `T-04` value category two-axis SVG 已補。
3. `T-05` copy/move ownership graph 已補。
4. `T-01` compile/link pipeline SVG 已補。
5. `T-06` RAII unwinding / ownership map 已補。

理由：這些是前面文章最適合補視覺結構的段落，不需要全篇重寫。

### Phase D：index 重排

把 `index.html` 從長清單改成 Part-based curriculum。已補六個 Part 的主閱讀路徑，並從 index source 移除舊 first-stage material 長清單。

### Phase D.1：移除舊章節檔案

已移除舊版 `theory/ch01_*.html` 到 `theory/ch18_*.html`，以及舊版 `impl/ch01_renderer_walkthrough.html`。統一版入口現在只保留 `T-01` ~ `T-14` 與 `I-01` ~ `I-17`。

理由：新版章節已經吸收舊章節的核心觀念；若繼續保留舊 HTML，讀者會無法判斷哪一套才是 canonical version。

### Phase E：進入 src refactor

等教學與架構圖穩定後，再把 `Framebuffer`、`DisplayBackend`、Win32/SDL backend selection 落到程式碼。

## 7. 不立即做的事

- 不新增 FPGA 專章。
- 不重推圖學數學。
- 不急著修改 README。
- 不先進入 `src` 大重構，除非使用者明確切換到實作。
