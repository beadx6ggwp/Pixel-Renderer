# C++ Tutorial Audit

這份 audit 是對 `docs/tutorial-cpp/` 的第二輪課程設計審查。它不是新的教學章節，而是接下來重寫、加深、補章節、整理 index / README 的依據。

目前狀態可以先這樣判斷：

```text
T-01 ~ T-10
  已接近正式章節
  早期章節已完成第一輪 SVG / memory-style 補圖

T-11 ~ T-14
  已加深第一輪

T-15
  第三輪新增，補 numeric semantics / pixel formats / bit operations

T-16
  第三輪新增，補 API design conventions，把 ownership / lifetime / contract 落到函式簽名

T-17
  第三輪缺口盤點新增，補 smart pointer / ownership adapters，避免 smart pointer 只藏在 T-06 小節

T-18
  第三輪缺口盤點新增，補 casts / type punning / bit_cast，銜接 object representation 與 pixel/buffer interpretation

T-22
  使用者指出 value category 與 const 還能延伸後新增，補 cv-qualification、move-from-const、const member function、mutable、ref-qualified member function

T-23
  Phase H 語意交界補強新增，補 overload resolution、conversion policy、operator design、hidden friend/ADL、equality/hash contract

T-24
  Phase H 語意交界補強新增，補 auto、decltype、decltype(auto)、template deduction、forwarding reference 與 range-for copy/borrow policy

T-25
  Phase H 語意交界補強新增，補 static storage duration、global state、initialization order、function-local static、thread_local、composition root

T-26
  Phase H 語意交界補強新增，補 Regular/Semiregular、equality law、hash consistency、strict weak ordering、float pitfalls、cache key / command diff / golden test equality policy

A-01
  Appendix deep dive 新增，回應 auto_ptr 是否也能做到 move 的疑問，拆解 destructive copy、C++98 library hack、rvalue reference 與 unique_ptr 的語意演化

A-02
  Appendix deep dive 新增，補 pointer semantic taxonomy：address、borrow、range view、unique owner、shared owner、weak observer、resource handle

A-06
  Appendix deep dive 新增，補 RAII problem genealogy：C cleanup path、goto cleanup、exception unwinding、partial initialization、destructor noexcept、scope guard、Pixel-Renderer handle wrapper

I-02 ~ I-13
  多數已接近正式章節
  I-12 已改為 cross-platform 命名
  I-13 是目前架構主線的核心章

I-14 ~ I-15
  已加深第一輪，補上資料流、API sketch、失敗案例與圖示

I-18
  第三輪新增，補 diagnostics / sanitizers / static analysis，銜接 GDB、UB、numeric policy 與 verification

I-19
  第三輪新增，補 preprocessor / compile-time configuration，銜接 CMake target、platform macro、feature macro 與 backend selection

I-20
  第三輪新增，補 dependency management / third-party integration，釐清 SDL/OpenGL/D3D/Metal/ImGui 的 include/link/runtime/package boundary
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

## 3. 已補的章節

### I-01 Current Renderer Architecture Walkthrough

已補成正式新版章節。它取代舊版 `impl/ch01_renderer_walkthrough.html` 在新版 track 裡的入口位置，避免 Implementation Track 從 toolchain 開始卻缺少專案現況地圖。

檔案：

```text
docs/tutorial-cpp/impl/i01_current_renderer_architecture_walkthrough.html
```

已用來回答：

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

使用者已明確指出 FPGA 先不用，但 testing / verification 需要成為近期主線。這章已補成新的 implementation 主線。

檔案：

```text
docs/tutorial-cpp/impl/i16_verification_golden_image_regression_testing.html
```

已用來回答：

- software renderer 怎麼測？
- 為什麼只看螢幕不算測試？
- golden image test 是什麼？
- deterministic rendering 需要哪些條件？
- 哪些測試應該 exact match，哪些需要 tolerance？
- 如何測 line / triangle / depth / blend / UI draw data？
- 如何建立 debug dump：framebuffer、depth buffer、command buffer、intermediate stage？
- 未來 CI 怎麼跑 renderer tests？

章節覆蓋：

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

視覺支撐：

- SVG：test pipeline `Draw commands -> Renderer -> Framebuffer -> Comparator -> Report`。
- ASCII：golden image mismatch report。

### I-17 Header Boundary / Dependency Hygiene

目前已有 CMake、backend boundary，但 C++ 專案會長期被 include dependency 污染。這章已獨立補成正式章節。

檔案：

```text
docs/tutorial-cpp/impl/i17_header_boundary_dependency_hygiene.html
```

已用來回答：

- public header vs private implementation 差在哪？
- 為什麼 `windows.h` / `SDL.h` 不應出現在 core header？
- forward declaration 何時可用，何時不可用？
- include path 是 build graph 的一部分，不是 editor 設定。
- pimpl 是否需要？第一版應不應用？
- target dependency 如何反映 source dependency？

視覺支撐：

- SVG：allowed dependency direction。
- table：header 可以 expose / 不應 expose 的東西。

### Third-round addendum: T-15 Numeric Semantics / Pixel Formats

第三輪審查指出 C++ 主線仍缺一個 renderer 會很快撞到的底層語意層：numeric semantics。它不是圖學數學，而是 C++ 如何承諾 integer、float、packed pixel、bit operation、rounding、overflow、endianness。

```text
docs/tutorial-cpp/theory/t15_numeric_semantics_pixel_formats.html
```

補上的問題：

- screen coordinate 和 buffer size 為什麼不能隨便 signed/unsigned 混算？
- signed overflow 和 unsigned wrap 差在哪？
- color channel arithmetic 為什麼要 widen -> compute -> clamp -> narrow？
- packed RGBA/BGRA 的 semantic bit layout、memory byte order、API interpretation 為什麼是三層？
- float depth / interpolation / viewport transform 為什麼需要 rounding 和 tolerance policy？
- golden image test 何時應 exact match，何時應 tolerance？

理由：沒有這章，後面 shader、UI alpha blend、depth buffer、texture sampling、golden image diff 都會缺少 C++ 語意地基。

### Third-round addendum: I-18 Diagnostics / Sanitizers / Static Analysis

第三輪審查指出 GDB、verification、UB、numeric semantics 之間還缺一個觀測層：工具如何在錯誤進入手動 debug 前先暴露。

```text
docs/tutorial-cpp/impl/i18_diagnostics_sanitizers_static_analysis.html
```

補上的問題：

- warnings、static analysis、sanitizers、debugger、golden tests 各自觀測哪一層？
- 為什麼「沒 crash」不代表沒有 C++ bug？
- ASan 如何抓 framebuffer 越界、use-after-free、use-after-scope？
- UBSan 如何抓 signed overflow、invalid shift、misaligned access、null dereference？
- Debug / Sanitized / Release build profile 為什麼不應混用？
- golden image mismatch 如何接回 sanitizer / debugger？

理由：沒有這章，I-16 的 testing 會停在 output comparison，T-03/T-15 的 UB/numeric policy 也缺少工程化檢查入口。

### Third-round addendum: I-19 Preprocessor / Compile-time Configuration

第三輪審查指出 CMake target graph 之後，還缺少一章說明 C++ 真正 parsing 之前的 text-level selection。跨平台支援若只靠到處塞 `#ifdef _WIN32`，會讓 renderer core 的語意被 platform macro 切碎。

```text
docs/tutorial-cpp/impl/i19_preprocessor_compile_time_configuration.html
```

補上的問題：

- preprocessor 發生在 C++ type checking 前，負責 include / macro / conditional compilation。
- include guard / `#pragma once` 的真正目的。
- object-like macro、function-like macro、configuration macro 的風險。
- platform macro 與 project feature macro 應分開。
- `target_compile_definitions` 應把 feature macro 綁到需要它的 target。
- compile-time backend selection 與 runtime `DisplayBackend` polymorphism 是兩層問題。
- Pixel-Renderer policy：platform `#ifdef` 應留在 backend / factory / CMake boundary，不進 `RenderDevice` / `Rasterizer`。

理由：沒有這章，I-07 的 CMake target boundary 和 I-08 之後的 backend abstraction 中間會缺少 compile-time configuration 規則。

### Third-round addendum: I-20 Dependency Management / Third-party Integration

第三輪審查指出使用者已多次追問 SDL、macOS、Windows、Ubuntu、OpenGL/D3D、ImGui 等第三方或平台 library 如何接入，但原本教學只零散提到 CMake linking，沒有完整拆出 dependency 的層次。

```text
docs/tutorial-cpp/impl/i20_dependency_management_third_party_integration.html
```

補上的問題：

- header/include path 只讓 compiler 找到 declaration，不代表 library 已接好。
- link library 負責 symbol resolution；runtime artifact 負責 OS loader 能執行。
- package discovery/version 只是幫 build system 找到 header/link/runtime 相關資訊，不是取代底層模型。
- SDL 可作為 DisplayBackend platform layer；OpenGL/D3D/Metal 是 graphics API，不應偷換 software renderer core。
- ImGui 應先當 UI frontend / draw data producer，不應讓 `RenderDevice` 直接依賴 ImGui。
- CMake `PRIVATE` / `PUBLIC` dependency 反映 dependency 是否漏出 public API。
- Windows DLL、macOS dylib/framework/rpath、Ubuntu/Linux `.so` 的 runtime loading 差異。
- Pixel-Renderer 第一版 policy：第三方 library 隔離在 display/backend/UI adapter target，不污染 renderer core。

理由：沒有這章，I-11 SDLDisplayBackend 和 I-12 cross-platform path 會少一個前置地基，使用者也難以判斷「能 include」、「能 link」、「能執行」到底是哪一層完成。

### Third-round addendum: T-16 API Design Conventions

第三輪審查指出 theory 雖然已有 ownership、value category、contract、header boundary，但還缺一章把它們收束成 API signature 的規則。

```text
docs/tutorial-cpp/theory/t16_api_design_conventions.html
```

補上的問題：

- API signature 如何表達 owner / view / borrow？
- <code>T&amp;</code>、<code>T*</code>、<code>std::span</code>、by value 何時使用？
- <code>const</code> 為什麼是 capability boundary？
- out parameter 和 return by value 如何對應 ownership / allocation policy？
- error return 如何分辨 precondition violation、recoverable failure、optional value？
- namespace / header boundary 為什麼也是 public API？

理由：沒有這章，I-08 到 I-13 的架構抽象容易停在圖上；T-16 把「前後端分離」落到可檢查的 C++ interface。

### Third-round addendum: T-17 Smart Pointer / Ownership Adapters

使用者指出 smart pointer 這類核心 C++ 主題沒有被正式提到。審查後確認：T-06 有提到 `unique_ptr` / `shared_ptr`，但只是 RAII 章節中的一節，缺少 `weak_ptr`、control block、cycle、aliasing、incomplete type、virtual destructor、custom deleter cost、resource handle 等足以影響 renderer 架構的細節。

```text
docs/tutorial-cpp/theory/t17_smart_pointer_ownership_adapters.html
```

補上的問題：

- smart pointer 為什麼不是「更安全的 pointer」，而是 ownership adapter？
- `unique_ptr` 適合哪些 unique ownership / runtime polymorphism case？
- `unique_ptr<Base>` 為什麼要求 base virtual destructor？
- custom deleter 如何包 SDL / Win32 / C API resource？deleter 本身有什麼 size / type cost？
- incomplete type / PImpl 如何協助 header hygiene？
- `shared_ptr` control block、strong count、weak count 的底層語意是什麼？
- `weak_ptr` 如何打斷 cycle？為什麼它仍不一定適合 command buffer？
- `shared_ptr` aliasing constructor 為什麼強但危險？
- raw pointer / reference / span 什麼時候反而是正確 borrow 表達？
- future texture / mesh / material 為什麼可能更適合 `TextureId` / generational handle？

理由：沒有這章，T-06 的 ownership map 會缺少 smart pointer 的完整取捨，I-13/I-15 未來談 resource cache、command buffer、shader resource binding 時也容易過度使用 `shared_ptr`。

### Third-round addendum: T-18 Casts / Type Punning / Bit Cast

第三輪缺口盤點指出 casts 不能只散落在 `static_cast` 範例或 strict aliasing 小段落裡。Renderer 會頻繁碰到 framebuffer bytes、packed pixel、SDL locked pointer、Win32 DIB memory、file/image binary data；如果不獨立釐清 cast 語意，很容易把「同一段 bytes」誤認成「可以任意用另一個 type 讀」。

```text
docs/tutorial-cpp/theory/t18_casts_type_punning_bit_cast.html
```

補上的問題：

- C-style cast 為什麼把多種語意藏在同一個語法裡？
- `static_cast` 適合哪些 explicit value conversion？
- `const_cast` 為什麼應隔離在 legacy C API boundary？
- `dynamic_cast` 適合 debug/tool path，為什麼不應成為 hot path default？
- `reinterpret_cast` 為什麼不會創造 object lifetime、修正 alignment、解除 strict aliasing？
- `std::memcpy` / `std::bit_cast` 何時是 representation copy 的正確工具？
- `std::byte` view 為什麼比把 file bytes cast 成 C++ struct 更誠實？
- Pixel-Renderer cast policy：Pack/Unpack 定義 semantic pixel format，platform/C API cast 隔離在 adapter layer。

理由：沒有這章，T-03 的 object representation、T-15 的 numeric/pixel format、I-11 的 SDL texture update、I-17 的 platform boundary 之間會缺少低階語意橋。

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
  T-01, I-02, I-03, I-04, I-05, I-06, I-07, I-19, I-20, I-18

Part 1 Object Exists in Storage
  T-02, T-03, T-18, T-06, T-17

Part 2 Values Move Through the Program
  T-04, T-22, T-23, T-24, T-25, T-26, T-05, T-07

Part 3 Types Become Generic Contracts
  T-08, T-09, T-10

Part 4 Architecture Boundaries
  T-11, I-08, I-09, I-10, I-11, I-12, I-13, T-16

Part 5 Performance and Future Systems
  T-12, T-13, T-14, T-15, I-14, I-15, I-16, I-17
```

並已補上 Part dependency map，讓入口不只列出章節，而是說明每個 Part 如何建立下一層需要的 invariant：

```text
Part 0  program / binary / diagnostics
  -> Part 1  object / storage / RAII
  -> Part 2  value category / transfer / callable lifetime
  -> Part 3  generic contracts / containers / invariants
  -> Part 4  architecture boundary / API signature
  -> Part 5  performance / UI / shader / verification
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

### Phase D.2：補 Part dependency map

已在 `index.html` 補上 Part dependency map 與每個 Part 的 output ability。目的不是再列一次章節，而是明確標出：

- 每個 Part 建立什麼 invariant。
- 下一層依賴上一層哪些語意。
- 為什麼 C++ theory、toolchain、backend abstraction、UI/shader、verification 不是分散主題。

### Phase D.1：移除舊章節檔案

已移除舊版 `theory/ch01_*.html` 到 `theory/ch18_*.html`，以及舊版 `impl/ch01_renderer_walkthrough.html`。統一版入口現在只保留新版 `T-*` 與 `I-*` 章節。

理由：新版章節已經吸收舊章節的核心觀念；若繼續保留舊 HTML，讀者會無法判斷哪一套才是 canonical version。

### Phase E：進入 src refactor

等教學與架構圖穩定後，再把 `Framebuffer`、`DisplayBackend`、Win32/SDL backend selection 落到程式碼。

### Phase F：第三輪缺口補強

使用者指出仍有漏講主題、部分章節不連貫或太淺。第三輪不應無限制加章，而是補會直接影響 Pixel-Renderer 工程演進的缺口：

1. `T-15 Numeric Semantics / Pixel Formats` 已新增。
2. `I-18 Diagnostics / Sanitizers / Static Analysis` 已新增：ASan/UBSan、warnings、static analyzer、clang-tidy、debug vs release diagnostics。
3. `T-16 API Design Conventions` 已新增：const-correctness、value/view/owner naming、span/string_view、out parameter、return type、namespace/header boundary。
4. Part dependency map 已補到 index。
5. `T-17 Smart Pointer / Ownership Adapters` 已新增：unique/shared/weak pointer、control block、custom deleter、observer/span、resource handle。
6. `T-18 Casts / Type Punning / Bit Cast` 已新增：static/reinterpret/const/dynamic cast、bit_cast、memcpy、std::byte、object representation。
7. `I-19 Preprocessor / Compile-time Configuration` 已新增：include、macro、platform macro、feature macro、target compile definitions、compile-time selection。
8. `I-20 Dependency Management / Third-party Integration` 已新增：SDL/OpenGL/D3D/Metal/ImGui 的 include/link/runtime/package boundary。
9. `T-22 Const / cv-qualification / Value Category` 已新增：move-from-const、const T&&、member function const、mutable、ref-qualified member function。
10. `T-23 Overload Resolution / Conversion / Operator Design` 已新增：overload ranking、explicit conversion、operator policy、hidden friend/ADL、equality/hash contract。
11. `T-24 Type Deduction / auto / decltype / Forwarding` 已新增：auto copy/borrow、decltype expression category、decltype(auto) dangling risk、forwarding reference、range-for policy。
12. `T-25 Static Storage / Global State / Initialization Order` 已新增：static storage duration、static initialization order、function-local static、inline variable/linkage、thread_local、composition root、global resource cache policy。
13. `A-01 auto_ptr 與 Move Semantics 的誕生` 已新增並加深為 Appendix deep dive：ownership transfer problem、destructive copy、non-const copy constructor、auto_ptr_ref proxy、container/algorithm generic assumption、std::move pipeline、move assignment 的 RHS value category / overload resolution、unique_ptr 的 deleted copy + move、Pixel-Renderer owner type checklist。
14. `T-26 Regular Types / Equality / Hashing / Ordering` 已新增：Regular/Semiregular、equality law、hash consistency、strict weak ordering、float/NaN/epsilon pitfalls、cache key、command diff、golden test equality policy。
15. `A-02 Pointer 語意地圖` 已新增：raw pointer、reference、span/view、unique_ptr、shared_ptr、weak_ptr、resource id / handle 的 semantic taxonomy。
16. `A-06 RAII 為什麼是 C++ 的核心` 已新增：C cleanup path、goto cleanup、exception unwinding、partial initialization、destructor 不應 throw、scope guard、Pixel-Renderer RAII wrapper policy。
17. 下一輪缺口盤點整理在 Phase G / Phase H；另外仍可做局部校稿與前段章節補 SVG / 深度。

### Phase G：第四輪缺口盤點

重新掃過目前 `T-01` ~ `T-18`、`T-22` ~ `T-26` 與 `I-01` ~ `I-20` 後，結論是：C++ 語意主線已經完整到可以支撐 framebuffer/backend refactor；但如果目標是把 Pixel-Renderer 擴充到 UI、shader、asset/resource、跨平台工作流與可量測效能，還有幾個主題目前只是散落在各章裡，尚未成為正式教學路徑。

這一輪不建議立刻把所有候選都寫成 HTML。應先分成「必須補正式章」、「應加深既有章」、「暫時不進主線」三類。

#### G1：應新增正式章節

##### I-21 Frame Loop / Event Loop / Input Abstraction

目前 `I-09`、`I-10`、`I-11` 有提到 Win32 message pump、SDL event、input state，但還沒有一章完整回答：

- application main loop 到底在每幀做哪些階段？
- OS event loop、window message、input polling、render update、present 之間的順序是什麼？
- `WM_PAINT` / `BitBlt`、`SDL_PollEvent` / `SDL_RenderPresent` 這類 platform event model 如何抽象？
- input 是 display backend 的責任，還是獨立 `InputState` / `InputBackend`？
- frame time、delta time、fixed timestep、render timestep、vsync wait 應該怎麼分層？
- UI 要吃 input event，renderer 要吃 draw command，兩者如何不互相污染？

理由：使用者前面一直追 SDL lifecycle、Win32 `BitBlt` 之後、OS compositor。現在 SDL/Win32 present path 已有，但整個 application loop / event loop 尚未成為架構圖的一等公民。

建議放置：

```text
I-21  Frame Loop / Event Loop / Input Abstraction
```

適合放在 `I-12 Cross-platform Development Path` 或 `I-13 Renderer Frontend / Backend Boundary` 後面。

##### I-22 Asset / Resource Pipeline

目前 `I-15` 有 resource binding，`I-14` 有 font atlas，`I-20` 有 third-party dependencies，但還沒有一章從專案工程角度整理 asset/resource pipeline：

- image / texture / font / mesh / shader-like data 從 disk 到 CPU memory 到 renderer resource 的生命週期。
- `std::filesystem` 如何處理 asset path，而不是硬寫 working directory。
- PNG/JPG 載入通常透過 `stb_image`、FreeType、或其他 library；這些 library 應放在哪個 target？
- texture resource 是 owned pixels、view、handle、還是 backend object？
- resource cache、resource id、generation counter、hot reload、debug dump 的基本模型。
- file format bytes 不能直接 cast 成 C++ struct，應接回 `T-18`。

理由：未來 UI/font atlas、texture sampling、shader resource binding 都會需要 asset/resource 概念。若不補，後面很容易把 texture pointer、file path、platform texture、renderer resource 混成同一件事。

建議放置：

```text
I-22  Asset / Resource Pipeline
```

##### I-23 Profiling / Benchmarking / Performance Regression

`T-12` 有 measurement，但偏概念；`I-16` 有 golden image，但偏 correctness。還缺一章工程化回答：

- microbenchmark、frame benchmark、profile capture 差在哪？
- Debug / Release / Sanitized build 的效能數字為什麼不能混用？
- `std::chrono` 如何正確量 frame time / scope time？
- profiler、sampling、instrumentation、flame graph 的基本差異。
- renderer hot loop 要量 SetPixel、DrawLine、DrawTriangle、Clear、Present 哪一層？
- performance regression test 應該如何和 golden image test 分開？

理由：software renderer 後面一定會碰效能，但如果沒有 profiling 方法，優化會變成憑感覺改 code。

建議放置：

```text
I-23  Profiling / Benchmarking / Performance Regression
```

##### I-24 Test Infrastructure / CTest / CI

`I-16` 說明了 renderer testing 的概念，但還沒工程化到 test runner / CI：

- unit test、golden image test、integration test 的目錄與 target 怎麼放？
- CTest 在 CMake project 裡扮演什麼角色？
- 是否使用 Catch2 / doctest / GoogleTest，或先用自製 minimal test harness？
- test data / golden images 如何管理？
- CI matrix 如何跑 Windows / macOS / Ubuntu？
- test executable 如何避免 depend on display backend？

理由：如果下一步真的進 `src` refactor，沒有 test target 很難確認 framebuffer/backend 拆分沒有壞掉。

建議放置：

```text
I-24  Test Infrastructure / CTest / CI
```

#### G2：應新增或加深的 Theory 主題

##### T-19 Standard Library Utilities for Systems Projects

目前 `std::span`、`string_view`、`optional`、`variant`、`expected-like` 已有，但 systems/rendering 專案還會頻繁需要：

- `std::filesystem`：asset path、working directory、relative path、portable path。
- `std::chrono`：frame time、benchmark、time point vs duration。
- `std::format` / logging string：debug output、diagnostic message。
- `std::source_location`：assert/logging 的 call site。
- `std::array` / `std::span` / `std::byte` 跨章整合。

不建議做成 STL 百科，而是以 Pixel-Renderer 的工具需求切入。

##### T-20 Memory Resources / Arena / Scratch Allocation

`T-09` 目前只有 allocator basics，`T-12` 有 memory layout，但還沒有完整講：

- allocator 不是只有「自訂 new」，而是 allocation policy。
- frame allocator / scratch allocator / arena allocator 的適用場景。
- `std::pmr` 的 mental model：memory_resource、polymorphic_allocator、container。
- UI layout、temporary transformed vertices、command buffer build phase 可能需要 frame-local allocation。
- 為什麼第一版不一定要實作 allocator，但要知道何時該引入。

這章可先標為進階，不必阻塞 framebuffer refactor。

##### T-21 Data-oriented Design for Renderer Data

`T-12` 有 AoS/SoA，但 data-oriented design 還不是一等主題：

- object-oriented decomposition vs data-oriented decomposition 的差異。
- command buffer、vertex buffer、depth buffer、tile list、UI draw list 如何被 CPU cache 使用。
- DOD 不是反 OOP，而是根據 access pattern 設計資料。
- ECS 可提到但不應成為主線；目前 renderer 更需要 command/data layout。

這章可接在 `T-12` 後，或作為 `I-15` / future shader pipeline 的理論支撐。

##### T-22 Const, cv-qualification, and Value Category

使用者指出 `value category` 和 `const` 還能繼續延伸。審查後確認：`T-04` 已講 expression category、reference binding、temporary lifetime、operator return；`T-16` 已講 const-correctness as capability design；但兩者交會處仍缺一個正式章節，因此已新增：

```text
docs/tutorial-cpp/theory/t22_const_cv_value_category.html
```

這章應回答：

- `const T`、`T const&`、`T&&`、`const T&&` 分別限制了什麼 operation？
- 為什麼 `std::move(const T&)` 產生的是 `const T&&`，通常不能呼叫 move constructor？
- 為什麼 `const` 不是 object lifetime，也不是 thread-safety 保證，而是 type system 上的 mutation capability 限制？
- top-level const / low-level const / pointer const / pointee const 如何區分？
- member function 後面的 `const` 改變的是 `this` 的型別；`mutable` 是什麼語意例外？
- ref-qualified member function：`foo() &`、`foo() const&`、`foo() &&` 如何和 value category 合作？
- `as const view`、`mutable borrow`、`consume/move` API 如何在 renderer resource、framebuffer view、command buffer 中表達？
- `const_cast` 只能移除 type qualifier，不會讓原本真的 const 的 object 變成可合法修改。

理由：這個主題是 T-04、T-05、T-16、T-18 的交界。沒有它，讀者容易以為 `const` 只是「不要改」、`std::move` 一定會 move、或 `const&` 延長 temporary lifetime 就能當成 ownership 策略。對 Pixel-Renderer 來說，這會直接影響 `DisplayBackend::Present(const Framebuffer&)`、`RenderDevice(Framebuffer&)`、resource cache lookup、UI command data ownership。

章節位置：

```text
T-22  Const / cv-qualification / Value Category
```

這章應排在 `T-04` / `T-05` 之後閱讀，並在 `T-16 API Design Conventions` 前後都建立 cross-link。

#### G3：既有章節需要加深，不一定新增章

##### T-11 Runtime Polymorphism

目前已能回答 virtual / template / variant / type erasure 的取捨，但手寫 type erasure、object slicing、ABI / plugin boundary 仍偏薄。若未來做 material/shader/resource registry，可以再加深。

##### T-04 / T-16 Value Category and Const Cross-link

`T-22` 已新增，`T-04` 不必重寫，但已把章末 next link 改成指向 T-22：value category 只描述 expression 能不能被當成 identity / movable source 使用；`const` 另外限制這個 expression 能不能被 mutation / move-from。`T-16` 的 const-correctness 也已回指 `T-22`，避免 API design 只停在 `const Framebuffer&` 的表層。

##### T-12 Low-level Performance

目前是 intro 層級。若要服務 renderer hot loop，後續可補：

- row-major framebuffer traversal 的 cache-line walk。
- branchless clipping / bounds check 的 tradeoff。
- SIMD intrinsic 只是最末端，不是資料整理的起點。
- CPU write-combining / memory bandwidth 直覺。

##### T-13 Concurrency

目前適合作為 intro。若未來要平行 rasterizer，應再補：

- work stealing vs static tiling。
- atomic memory order 更具體的 acquire/release 例子。
- per-thread buffer merge 的 determinism。
- job graph vs thread pool。

##### I-16 Verification

目前概念正確，但工程落地偏少。可以選擇把 test runner / CI 拆成 `I-24`，或直接加深 I-16。建議拆成 I-24，因為測試基礎設施是 build system 層，不只是 renderer correctness。

##### I-20 Dependency Management

目前已建立五層模型。若之後真的導入 SDL/vcpkg/Homebrew/Ubuntu packages，需補實際 CMake presets、install runtime copy、macOS app bundle 或 rpath。現在先不展開，避免變成平台安裝手冊。

#### G4：暫時不進 tutorial-cpp 主線

這些主題重要，但不應現在塞進 C++ 教學：

- 完整 graphics math：edge function、barycentric、clipping、texture filtering，應留在 `tutorial-soft-renderer`。
- FPGA / HDL / hardware verification：長期方向，但不放進近期 C++ track。
- 完整 OpenGL / D3D / Vulkan 教學：目前只需要知道它們是 dependency/API boundary，不應讓 software renderer 教學偏航。
- ECS / game engine architecture 全套：可作為 comparison，不應成為 Pixel-Renderer 第一階段主線。

#### G5：建議下一步排序

若目標是「先讓教學能支撐接下來的程式碼 refactor」，優先順序應該是：

```text
P0  I-24 Test Infrastructure / CTest / CI
    因為進 src refactor 前需要測試地基。

P1  I-21 Frame Loop / Event Loop / Input Abstraction
    因為 DisplayBackend 不只 present，也牽涉 event/input/frame lifetime。

P1  I-22 Asset / Resource Pipeline
    因為 UI font atlas、texture sampling、shader resources 都會依賴它。

P2  I-23 Profiling / Benchmarking / Performance Regression
    因為性能優化應在 correctness/refactor 穩定後再系統化。

P2  T-19 Standard Library Utilities
    補 filesystem/chrono/source_location 等工具，但不急於阻塞架構 refactor。

P3  T-20 Memory Resources / Arena / Scratch Allocation
P3  T-21 Data-oriented Design for Renderer Data
    作為 shader/UI/parallel renderer 前的進階理論。
```

這個排序的判準是：先補會影響「下一次動 `src` 能不能安全」的主題，再補未來擴充會需要的概念地基。

### Phase H：語意交界缺口盤點

沿著 `T-22` 的觀點繼續檢查，下一批值得補的不是「多認識幾個標準庫 API」，而是 C++ 語意彼此交會時容易失真的地方。這些主題通常不會單獨在初學語法書裡被講深，但會直接影響 renderer 的 math type、resource handle、command buffer、generic shader、backend API。

#### H1：已新增 / 建議新增正式章節

##### T-23 Overload Resolution / Conversion / Operator Design

已新增正式章節：

```text
docs/tutorial-cpp/theory/t23_overload_conversion_operator_design.html
```

`T-02` 有 `explicit` / implicit conversion，`T-04` 有 operator return category，`T-08` 有 templates，`T-16` 有 API signature。T-23 把這些主題收束成「call expression / operator / conversion」的語意交界，避免 C++ 表面語法看起來合理，但實際選到錯誤 overload 或允許危險 conversion。

已補上的問題：

- overload set 如何被選中？ranking 大概看哪些東西？
- implicit conversion、user-defined conversion、standard conversion 誰優先？
- `explicit constructor`、`explicit operator bool` 如何避免 resource handle / color / id 被誤用？
- hidden friend operator 與 ADL 是什麼？為什麼 math type 常用 hidden friend？
- `operator==`、`operator<=>`、hash function 的語意 contract 是什麼？
- `Vec3 + Vec3` 應回傳 value；`operator+=` 應回傳 `T&`；哪些 operator 不應 overload？
- `Color`、`TextureId`、`FramebufferSize` 這類 strong type 如何避免和 raw integer 混用？

Pixel-Renderer relevance：

- math/vector type 的 operator design。
- `TextureId` / `ShaderId` / `ResourceHandle` 不應隱式變成 int。
- `RGBA8` / `BGRA8` / `DepthValue` 不應靠同一個 `uint32_t` 或 `float` 混用。
- UI command / shader command 的 equality / hashing 若錯，cache 和 regression test 會失真。

##### T-24 Type Deduction / auto / decltype / Template Deduction

已新增正式章節：

```text
docs/tutorial-cpp/theory/t24_type_deduction_auto_decltype_forwarding.html
```

`T-04` 講 forwarding reference，`T-08` 講 template，但 `auto`、`decltype(auto)`、template argument deduction 的實際陷阱需要被放到 call site / generic helper / range-for 的具體場景裡。T-24 已把這些整理成 deduction policy。

已補上的問題：

- `auto` 會丟掉 top-level const/reference 的哪些部分？
- `auto&`、`const auto&`、`auto&&` 分別適合什麼？
- `decltype(expr)` 和 `auto` 為什麼不是同一套規則？
- `decltype(auto)` 可能保留 reference，何時會產生 dangling？
- range-for 裡 `auto` / `auto&` / `const auto&` 對 copy、borrow、mutation 的影響。
- template deduction 和 forwarding reference 如何和 `const` / value category 合作？

Pixel-Renderer relevance：

- command buffer iteration 若用錯 `auto`，可能複製 command 或失去 mutation。
- `auto view = framebuffer.Pixels()` 可能把 view/value 的語意藏起來。
- generic shader helper 若用 `decltype(auto)` 回傳 reference，可能製造 dangling。
- UI layout / resource registry iteration 需要清楚 copy vs reference。

##### T-25 Static Storage / Global State / Initialization Order

已新增正式章節：

```text
docs/tutorial-cpp/theory/t25_static_storage_global_state_initialization.html
```

`T-03` 有 storage duration，`T-01` 有 translation unit，`T-06` 有 RAII；T-25 已把這些接到 static/global state 的工程後果，避免 renderer owner state 被 hidden global lifetime 污染。

已補上的問題：

- static storage duration 和 object lifetime 如何開始/結束？
- global object initialization order across translation units 為什麼危險？
- function-local static 解決什麼、又帶來什麼 lifetime / testability 問題？
- inline variable、anonymous namespace、internal linkage 和 singleton-style object 的差異。
- `thread_local` 的 lifetime 和 job system / worker thread 的關係。
- logger、resource registry、global config、font atlas 是否應是 global？

Pixel-Renderer relevance：

- renderer/global device singleton 會讓 tests、headless rendering、multi-window backend 很難做。
- global resource cache 會污染 golden tests 和 deterministic replay。
- static font atlas / default texture 若要存在，應該有明確 owner 或 composition root。

##### T-26 Regular Types / Equality / Hashing / Ordering

舊章曾提到 Regular / Semiregular，現在散落在 `T-08` / `T-09`，但沒有完整成章。

狀態：已完成。

```text
docs/tutorial-cpp/theory/t26_regular_types_equality_hashing_ordering.html
```

已補上的問題：

- Regular / Semiregular / Movable / Copyable 不是語法分類，而是 type law。
- equality 要滿足 reflexive / symmetric / transitive；浮點、NaN、epsilon 會破壞哪些假設？
- hash 必須和 equality 一致，否則 unordered container 會失效。
- ordering 對 `std::map` / sorting / command key / cache key 有什麼 contract？
- resource handle、draw command、pipeline state、vertex attribute layout 哪些需要 equality/hash？

Pixel-Renderer relevance：

- pipeline state cache、texture cache、shader variant cache 都需要 key equality/hash。
- golden test / command buffer diff 需要定義 equality。
- `float` 參與 key 或 comparison 時要非常小心。

正式章節：

```text
T-26  Regular Types / Equality / Hashing / Ordering
```

#### H2：建議加深既有章節，不一定新增章

##### T-02 Initialization

可補：

- `std::initializer_list` 優先權陷阱：`vector<int>{10}` vs `vector<int>(10)`。
- most vexing parse 作為語法/初始化交界。
- aggregate vs invariant-protecting class 的進階取捨。
- designated initialization 在 C++20 的邊界。

理由：renderer 會大量建立 small math types、rect、color、pipeline state；初始化語意若不清楚，容易讓 type invariant 漏掉。

##### T-05 Copy / Move / Transfer

可補：

- move assignment vs move construction 的差異。
- self-assignment / self-move 應如何看待。
- copy-and-swap 現在何時仍有價值。
- strong/basic/no-throw exception guarantee 如何落到 resource type。

理由：現在 T-05 已能建立 transfer 模型，但若要寫 robust resource owner，assignment 和 guarantee 還能更深。

##### T-06 RAII / Exception Safety

可補：

- constructor failure 時，已建好的 member 如何被 destruct。
- destructor 不應 throw 的原因。
- scope guard / unique_resource 這類 generalized RAII pattern。
- partial initialization 的 cleanup path。

理由：Win32/SDL backend init 很容易是多步驟 resource acquisition，這比單一 `unique_ptr` 更接近實作現場。

##### T-10 Error Handling / Contracts

可補：

- `[[nodiscard]]` 對 Result / error code 的意義。
- recoverable error 和 programmer bug 如何在 function signature 上區分。
- `noexcept` 和 terminate 的實際後果。

理由：backend init、asset loading、shader/resource creation 都會回傳失敗；若錯誤被 caller 忽略，測試和 debug 會很痛苦。

##### T-13 Concurrency

可補：

- `volatile` 不是 synchronization。
- `memory_order` 的最小 mental model。
- acquire/release 在 worker queue 或 atomic flag 的具體例子。

理由：如果未來做 job system，這是會從「能跑」變成「偶爾壞」的分界。

#### H3：暫時不建議新增成正式章

- `friend` / ADL 可放進 T-23，不必獨立。
- `operator<=>` 可放進 T-26，不必獨立。
- `volatile` 可先作為 T-13 加深，不必獨立。
- `decltype(auto)` 可放進 T-24，不必獨立。
- PImpl 可留在 T-17 / I-17 加深，不必獨立。

#### H4：建議優先順序

如果接下來繼續補 theory，T-23、T-24、T-25、T-26 已完成；下一輪優先順序建議：

```text
P1  加深 T-05 / T-06 / T-10
    補 assignment、exception guarantee、nodiscard、backend init cleanup。
```

這一輪的判準是：凡是會讓 C++ type 的「表面語法」和「實際語意」分裂，且會影響 renderer API / cache / command / resource lifetime 的，都值得補。

## 7. 不立即做的事

- 不新增 FPGA 專章。
- 不重推圖學數學。
- 不急著修改 README。
- 不先進入 `src` 大重構，除非使用者明確切換到實作。

## 8. Appendix / Deep Dive 分流

使用者明確指出新的思考偏好：透過 problem genealogy / semantic archaeology 回到概念當初要解決的問題，避免倒果為因；再用多視角 bootstrapping 檢查同一概念在語法、機制、語意 contract、invariant、工程設計中的不同面向。

因此 Appendix 的定位應該是：

- 不取代主線章節。
- 不只是 FAQ。
- 承接「為什麼這個概念會長成這樣」的深挖。
- 用 naive solution -> failure -> abstraction upgrade 的方式補森林視角。

已完成：

```text
A-01  auto_ptr 與 Move Semantics 的誕生
A-02  Pointer 語意地圖：address、borrow、owner、handle
A-06  RAII 為什麼是 C++ 的核心，不只是 destructor 技巧
```

建議候選：

```text
A-03  Header / Linker 錯誤考古
A-04  SDL Present Path 考古：從 BitBlt 到 OS compositor
A-05  CMake 為什麼存在：從手打 g++ 到 target graph
A-07  Type Erasure / Virtual / Variant 的設計壓力史
A-08  Testing Golden Image 的問題生成史
```

排序建議：

1. `A-04` 優先，因為使用者前面密集追問 SDL / Win32 / compositor / framebuffer path。
2. `A-05` 適合在進一步寫 I-24 / CI 前補，避免 CMake 只像工具語法。
3. `A-03` 適合回補 T-01 / I-17，把 header/linker 錯誤從現象拉回 translation unit / symbol / ABI。
