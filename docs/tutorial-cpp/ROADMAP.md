# C++ Tutorial Integration Roadmap

這份 roadmap 的目的不是再加一份待辦清單，而是把目前 `docs/tutorial-cpp/` 重新整理成一套連貫課程。

早期 19 章已經被新版 T/I 章節吸收。這份 roadmap 現在記錄的是「如何整理成統一版」，而不是讓舊 19 章和新版 roadmap 並排存在。

新版目標是：

```text
C++ for Pixel-Renderer
  |
  +-- Theory Track
  |     完整講 C++ 語意本體
  |     不被目前 code 限制
  |     但每個概念都能回到 renderer 問題
  |
  +-- Implementation Track
        把 theory 用在 Pixel-Renderer 工程演進
        專注工程化、抽象分離、toolchain、CMake、VSCode、Win32/SDL/macOS
```

## 1. 整合原則

### 1.1 Theory 不應只是 implementation 的附錄

C++ theory 線要能獨立成立。它應該回答：

- C++ 程式如何被編譯、連結、形成 executable？
- object 如何開始 lifetime、結束 lifetime、佔用 storage？
- type 如何定義合法狀態、操作、invariant、cost model？
- expression 為什麼同時有 type 與 value category？
- generic programming 到底要求 type 滿足哪些 laws？
- runtime polymorphism、type erasure、variant、command buffer 各自在解什麼問題？
- low-level memory/performance 問題如何影響 renderer hot loop？

Pixel-Renderer 是主要應用場景，但不能讓 C++ 教學只剩目前 code 的註解。

### 1.2 Implementation 不應重講 C++ 語法

Implementation 線要像工程手冊，回答：

- 目前專案如何編譯？
- macOS Apple Clang、Windows MinGW GCC、MSVC 差在哪？
- CMake target、include path、platform-specific source 怎麼設計？
- OpenGL、D3D、SDL、ImGui 這類第三方或平台 library 如何接進 build graph？
- VSCode 的 tasks/launch/include path 怎麼處理？
- `ScreenManager` 如何拆成 owned `Framebuffer` + `DisplayBackend`？
- Win32 `BitBlt`、SDL `UpdateTexture`、macOS present path 如何互換？

每個 implementation 章節可以引用 theory，但不應把 theory 從頭重講。

### 1.3 Implementation 也不應重講圖學與 rasterization

Pixel-Renderer 會另外有一組 rendering / computer graphics 筆記，專門處理：

- line rasterization
- triangle rasterization
- barycentric coordinates
- edge function
- clipping
- depth buffer
- texture sampling
- shader pipeline 的數學與圖學意義
- GPU pipeline history / architecture

因此 `tutorial-cpp` 的 Implementation Track 不應把篇幅花在重新推導圖學。它應該關注工程化問題：

- 專案如何拆 module？
- header/source dependency 怎麼收斂？
- build system 如何跨平台？
- compiler/toolchain 差異如何管理？
- VSCode / debugger / IntelliSense 如何配置？
- resource ownership 如何從 Win32 object 裡抽出來？
- backend interface 怎麼設計，才不污染 renderer core？
- command buffer、UI draw data、shader interface 如何定義資料邊界，而不是重講它們背後的圖學推導？

### 1.4 舊章節的處理原則

早期章節不作為最終入口；它們的有效內容被拆進新版 T/I 章節。遷移時的判斷原則是：

```text
keep     概念方向正確，只需小修
deepen   方向正確，但內容太淺，需要補完整語意
split    一章混入多個問題，需要拆成 theory / implementation
merge    多章其實屬於同一語意線，需要合併重排
new      原本缺席的新主題
```

## 2. 新版課程骨架

### Theory Track

```text
T-01  Compilation / Linkage / ABI
T-02  Type System / Initialization
T-03  Object Model / Storage / Lifetime / UB
T-18  Casts / Type Punning / Bit Cast
T-04  Value Category / Reference System
T-22  Const / cv-qualification / Value Category
T-23  Overload Resolution / Conversion / Operator Design
T-24  Type Deduction / auto / decltype / Forwarding
T-25  Static Storage / Global State / Initialization Order
T-26  Regular Types / Equality / Hashing / Ordering
T-05  Copy / Move / Object Transfer
T-06  RAII / Resource Ownership / Exception Safety
T-17  Smart Pointer / Ownership Adapters
T-07  Callable / Overload / Lambda
T-08  Templates / Concepts / Type Traits
T-09  STL / Containers / Iterators / Allocator Basics
T-10  Error Handling / Contracts / Invariants
T-11  Runtime Polymorphism / Type Erasure / Variant
T-12  Low-level Performance / Memory Layout
T-13  Concurrency / Memory Model / Job System Intro
T-14  Modern C++ Boundaries
T-15  Numeric Semantics / Pixel Formats / Bit Operations
T-16  API Design Conventions
```

### Implementation Track

```text
I-01  Current Renderer Architecture Walkthrough
I-02  Toolchain Map: MSVC / MinGW GCC / Apple Clang / graphics libraries
I-03  Build Workflow: g++ driver / VSCode / GDB process control / Make / CMake
I-04  Debugger Fundamentals: GDB / LLDB
I-05  VSCode Build / Debug / IntelliSense Setup
I-06  Make Workflow
I-07  CMake Target-based Build
I-08  Framebuffer Ownership Refactor
I-09  DisplayBackend Interface
I-10  Win32DisplayBackend
I-11  SDLDisplayBackend
I-12  Cross-platform Development Path
I-13  Renderer Frontend / Backend Engineering Boundary
I-14  Immediate-mode UI Integration Boundary
I-15  Shader Pipeline Interface Boundary
I-16  Verification / Golden Image / Regression Testing
I-17  Header Boundary / Dependency Hygiene
I-18  Diagnostics / Sanitizers / Static Analysis
I-19  Preprocessor / Compile-time Configuration
I-20  Dependency Management / Third-party Integration
```

## 3. 舊章節吸收表

| 早期章節 | 新位置 | 處理 | 理由 |
|---|---|---|---|
| Ch01 編譯模型 | T-01 + I-02/I-03 | split + deepen | 已擴成 declaration/definition、ODR、linkage、ABI；toolchain/CMake 放 implementation。 |
| Ch02 記憶體四區 | T-03 + T-12 | deepen | 已從 stack/heap/static/code 擴到 storage duration、object lifetime、alignment、object representation、UB、cache locality。 |
| Ch03 指標與參考 | T-03 + T-04 | split + deepen | pointer/reference 一部分是 memory alias，一部分是 reference/value category system。 |
| Ch04 struct 與 class | T-02 + T-10 | merge + deepen | 已接到 type system、access control、invariant、aggregate、conversion。 |
| Ch05 建構與解構 | T-03 + T-06 | merge + deepen | constructor/destructor 是 object lifetime，也是 RAII/resource ownership 的入口。 |
| Ch06 Copy 語意 | T-05 + T-06 | merge | 已把 copy 跟 move、resource ownership、exception safety 一起講。 |
| Ch07 Rule of 0/3/5 | T-06 | deepen | 已補 Rule of Zero 與 exception safety、copy-and-swap、move-only handle、custom deleter。 |
| Ch08 Move 是 ownership transfer | T-05 | merge + deepen | 已放進完整 object transfer：copy/move/elision/moved-from state。 |
| Ch09 std::move 與 value category | T-04 + T-22 + T-05 | split + deepen | `std::move` 屬於 value category cast；const/cv 會限制 move-from capability；move constructor 屬於 transfer。 |
| Ch10 Return by value 與 RVO | T-04 + T-05 | merge + deepen | 已接 C++17 prvalue、temporary materialization、guaranteed copy elision。 |
| Ch11 In-place construction | T-03 + T-09 | split | placement new/construct_at 是 storage/lifetime；emplace 是 container API。 |
| Ch12 vector reallocation 與 noexcept move | T-09 + T-06 | split + deepen | vector reallocation 屬於 container；`noexcept move` 屬於 exception safety/resource type design。 |
| Ch13 Type = states + operations + invariants | T-10 | keep + deepen | 方向正確，已補 contract、precondition/postcondition、error handling。 |
| Ch14 Regular / Semiregular types | T-08 + T-09 | merge + deepen | regular 概念已接 concepts、templates、STL algorithm requirements。 |
| Ch15 Operator / Template 與數學型別 | T-02 + T-04 + T-22 + T-23 + T-24 + T-08 | split + deepen | operator 涉及 type operation、value category、const capability、overload/conversion；template 與 deduction 應獨立加深。 |
| Ch16 智慧指標 | T-06 | keep + deepen | 已補 custom deleter、aliasing pitfalls、observer pointer/span/reference、shared ownership 的判準。 |
| Ch17 繼承、virtual 與 vtable | T-11 | keep + deepen | 已補 type erasure、variant、visitor、static polymorphism alternatives。 |
| Ch18 設計模式速查 | T-11 + I-10/I-11/I-12 | split | pattern 已分成 theory vocabulary 與 renderer command/UI/shader 實作。 |
| I-01 渲染器架構導覽 | I-01 | keep | 作為 implementation track 入口，後續章節從這裡拆 refactor plan。 |

## 4. 已補齊的 C++ 主題覆蓋範圍

### 4.1 Compilation / Linkage / ABI

新版 `T-01`、`I-02`、`I-03` 已補：

- declaration vs definition
- ODR
- internal/external linkage
- `static`, `extern`, anonymous namespace
- inline function / inline variable
- name mangling
- object file / symbol / relocation
- static library vs dynamic library
- ABI 與 name/layout/calling convention
- MSVC/GCC/Clang 差異

### 4.2 Initialization 與 Conversion

新版 `T-02` 已涵蓋：

- default/value/direct/copy/list initialization
- aggregate initialization
- narrowing
- `explicit`
- implicit conversion sequence
- `const`, `constexpr`, `consteval`
- `enum class`
- initialization order

### 4.3 Object Model / UB

新版 `T-03` 已涵蓋：

- storage vs object lifetime
- placement new / `std::construct_at`
- alignment
- strict aliasing
- object representation
- padding
- trap representation
- trivial / standard-layout
- undefined behavior 的工程後果

### 4.4 Value Category 完整版

新版 `T-04`、`T-05` 已從 `std::move` 擴到：

- expression has type and value category
- lvalue / xvalue / prvalue
- temporary materialization
- lifetime extension
- reference collapsing
- forwarding reference
- `std::forward`
- overload resolution with `T&`, `const T&`, `T&&`
- operator return category
- dangling reference in command/lambda/UI data

新增 Appendix `A-01` 後，`auto_ptr` 的疑問被放成 deep dive，而不是主線正式章節：

- `auto_ptr` 能 transfer ownership，但走的是 copy-looking syntax
- non-const copy constructor 反映它必須修改 source，和正常 copy expectation 衝突
- destructive copy 破壞 Copyable / Regular type 假設
- generic container / algorithm 會因為「copy 消耗 source」而失去局部推理
- C++11 rvalue reference 把 transfer 變成語言層語意通道
- `std::move` 只是把 expression 轉成 xvalue；真正 transfer 發生在 move constructor / move assignment
- move assignment 雖然仍用 `=`
  這個 token，但 `b = a` 和 `b = std::move(a)` 會因 RHS value category 進入不同 overload channel
- `unique_ptr` 以 deleted copy + explicit move 取代 `auto_ptr`
- Pixel-Renderer owner type 應避免任何「copy 偷 ownership」的 API

新增 `T-22` 後，value category 和 const/cv-qualification 的交界也已補上：

- top-level const vs low-level const
- `std::move(const T&)` 產生 `const T&&`，通常不能呼叫 move constructor
- member function `const` 改變 `this` 的型別
- `mutable` 是 logical const 的例外，不是 API contract 後門
- ref-qualified member function：`foo() &`、`foo() const&`、`foo() &&`
- `const_cast` 只改 access path，不改 object truth
- Pixel-Renderer API：read-only present、mutable render borrow、consume/move 分開表達

新增 `T-23` 後，operator / conversion / overload 的語意交界也已補上：

- overload resolution 的 candidate / viable / ranking 工作模型
- implicit conversion 與 user-defined conversion 如何穿過 domain boundary
- `explicit` constructor 與 `explicit operator bool`
- hidden friend 與 ADL 在 math type operator 的用途
- non-mutating operator by value、compound assignment returns `T&`
- equality / ordering / hash 的 cache key contract
- Pixel-Renderer strong type policy：TextureId、ShaderId、RGBA8、PipelineState 不隱式混用 raw integer

新增 `T-24` 後，type deduction 的語意交界也已補上：

- `auto` by value 會丟掉 reference 與 top-level const，建立新 object
- `auto&`、`const auto&`、`auto&&` 在 copy / borrow / forwarding 上的差異
- range-for 裡 `auto` / `const auto&` / `auto&` 對 command buffer 的影響
- `decltype(expr)` 如何把 value category 轉成 `T` / `T&` / `T&&`
- `decltype(auto)` 作為 forwarding accessor 的工具與 dangling risk
- template deduction 與 `std::forward` 如何保留 caller 的 lvalue/rvalue intent
- Pixel-Renderer deduction policy：domain API 明確寫 ownership/borrow，generic wrapper 才使用 forwarding reference

新增 `T-25` 後，static/global state 的語意交界也已補上：

- static storage duration 不等於 resource readiness
- cross-translation-unit dynamic initialization order 不應承載架構 dependency
- function-local static 解決部分 init order，但仍是 hidden global state
- inline variable / internal linkage 只處理 symbol visibility，不處理 ownership
- `thread_local` 是 per-thread state，不是 synchronization，也不是 deterministic replay 的免費答案
- composition root / RendererContext 應顯式擁有 framebuffer、backend、resource registry
- Pixel-Renderer policy：math constants 可 static，framebuffer/backend/resource cache 不應 process-wide global

新增 `T-26` 後，regular/equality/hash/order 的語意交界也已補上：

- Regular / Semiregular / Movable / Copyable 是 type law，不只是能不能編譯的語法分類
- equality 需要 reflexive / symmetric / transitive，否則 diff、cache、container 都會失去共同假設
- hash 必須和 equality 一致，否則 `std::unordered_map` / resource registry 會變成偶發錯誤
- ordering 需要 strict weak ordering，否則 sorting / `std::map` / command key 會出現 undefined behavior
- float、NaN、epsilon equality、`+0.0` / `-0.0` 不適合不經設計就進 cache key
- Pixel-Renderer policy：cache key、command diff、golden image comparison、resource handle equality 應各自定義語意

### 4.5 Callable / Lambda / Overload

新版 `T-07` 已涵蓋：

- function pointer
- member function pointer
- functor
- lambda closure object
- capture by value/reference lifetime
- `std::function`
- overload set
- default argument
- inline function

### 4.6 Templates / Concepts / Type Traits

新版 `T-08` 已從 operator/template 數學型別入門擴到：

- function template
- class template
- specialization / partial specialization
- type traits
- SFINAE
- `constexpr if`
- concepts
- CRTP
- policy-based design
- shader/math/backend compile-time policy

### 4.7 STL / Containers / Iterators

新版 `T-09` 已從 vector reallocation 擴到：

- `std::array`
- `std::vector`
- `std::span`
- `std::string_view`
- map/unordered_map tradeoff
- iterator invalidation
- erase-remove
- contiguous storage
- allocator basics
- command buffer / vertex buffer 的 container 選擇

### 4.8 Smart Pointer / Ownership Adapters

第三輪缺口盤點新增 `T-17`，把原本藏在 T-06 裡的 smart pointer 獨立成 ownership adapter 主題：

- `unique_ptr` as unique ownership
- `unique_ptr<Base>` and virtual destructor
- `make_unique`
- custom deleter and deleter size/cost
- incomplete type / PImpl
- `shared_ptr` control block and reference count
- `weak_ptr` and cycles
- `shared_ptr` aliasing constructor
- raw pointer / reference / span as borrow, not ownership
- resource id / generational handle for renderer command buffers

理由：smart pointer 不是「更安全的 pointer」，而是會改變 lifetime policy 的 type。Renderer 若濫用 `shared_ptr`，會讓 command buffer、resource cache、debug dump、hot path ownership 變得不透明。

### 4.9 Casts / Type Punning / Bit Cast

第三輪缺口盤點新增 `T-18`，補上 object representation 與 renderer buffer interpretation 中間缺少的 cast layer：

- C-style cast 為什麼不適合 C++ code review
- `static_cast` as explicit value conversion
- `const_cast` at legacy C API boundary
- `dynamic_cast` for debug/tool polymorphic checks
- `reinterpret_cast` does not create object lifetime / alignment / aliasing permission
- `std::memcpy` and `std::bit_cast` for representation copy
- `std::byte` view for raw binary data
- file/image/network data 不應直接 cast 成 C++ struct
- Pixel-Renderer cast policy：Pack/Unpack define semantic pixel format，platform cast 隔離在 adapter layer

理由：renderer 很容易把 framebuffer bytes、packed pixel、SDL locked pointer、Win32 DIB memory、file binary data 混成「反正都是 bytes」。沒有這章，T-03 的 strict aliasing 和 T-15 的 pixel format 會缺少中間的工程規則。

### 4.10 Error Handling / Contracts

新版 `T-10` 已把分散在 invariant 的問題整理成：

- `assert`
- `optional`
- expected-like result
- exception
- `noexcept`
- error code
- precondition/postcondition
- fail-fast vs recoverable error

### 4.11 Low-level Performance

新版 `T-12` 已涵蓋：

- cache line
- cache locality
- alignment/padding
- false sharing
- branch prediction basics
- SIMD intro
- AoS vs SoA
- memory bandwidth
- pointer aliasing and optimizer

### 4.12 Concurrency / Memory Model

新版 `T-13` 已建立 intro 層級：

- thread
- mutex
- atomic
- data race
- memory order intro
- producer-consumer
- tile/job system intro

### 4.13 Numeric Semantics / Pixel Formats

第三輪審查新增 `T-15`，補上 renderer C++ 會很快撞到但原本沒有獨立地基的 numeric layer：

- fixed-width integer
- signed/unsigned overflow rule
- integer promotion and narrowing
- bit shift and mask policy
- packed RGBA/BGRA pixel format
- semantic channel order vs memory byte order vs API interpretation
- floating-point precision, rounding, epsilon
- exact vs tolerance policy for golden image tests

### 4.14 API Design Conventions

第三輪審查新增 `T-16`，把前面分散的 ownership、lifetime、value category、contract、header boundary 收束成 API 設計規則：

- owner / view / borrow
- `const` as capability boundary
- `T&`、`T*`、`std::span<T>`、by value 的語意差異
- out parameter vs return by value
- recoverable failure vs precondition violation
- namespace / header boundary as API surface
- Pixel-Renderer 第一版 API policy：`Framebuffer` owns storage、`RenderDevice` mutably borrows、`DisplayBackend` read-only presents

### 4.15 Diagnostics / Sanitizers / Static Analysis

第三輪審查新增 `I-18`，補上工具鏈與 verification 中間缺少的一層：

- compiler warnings as source-level suspicion
- ASan for memory access contract
- UBSan for C++ semantic failure
- static analysis / clang-tidy for bug patterns
- Debug / Sanitized / Release profile separation
- renderer-specific diagnostics policy
- golden image mismatch 如何接回 sanitizer / debugger

### 4.16 Preprocessor / Compile-time Configuration

第三輪審查新增 `I-19`，補上 CMake target 與實際 C++ parsing 之間的 compile-time selection layer：

- preprocessor 在 C++ type checking 前處理 include / macro / conditional compilation
- include guard / `#pragma once`
- object-like macro / function-like macro / configuration macro 的風險
- platform macro vs feature macro
- `target_compile_definitions` 如何把 macro 綁在 target boundary
- compile-time selection vs runtime polymorphism
- Pixel-Renderer policy：platform `#ifdef` 留在 backend / factory / build boundary，不進 renderer core

### 4.17 Dependency Management / Third-party Integration

第三輪審查新增 `I-20`，補上第三方 library 接入專案時的完整工程模型：

- header/include path 只服務 compiler declaration lookup
- compile definition 服務 preprocessor / feature selection
- link library 服務 linker symbol resolution
- runtime artifact 服務 OS loader
- package discovery/version 服務 build system 找到前面那些東西
- SDL / OpenGL / D3D / Metal / ImGui 的接入型態差異
- CMake imported target / `target_link_libraries(PRIVATE/PUBLIC)` 的 boundary 意義
- Windows DLL、macOS dylib/framework/rpath、Linux `.so` runtime loading
- Pixel-Renderer policy：第三方 library 隔離在 display/backend/UI adapter target，不污染 renderer core

## 5. 新版 index 已如何整理

`index.html` 已改成統一版入口，不再保留早期 `Ch01` ~ `Ch18` 線性列表。現在主線是：

```text
Hero
Overview
How to use this tutorial

Track A: C++ Theory
  Module 1: Program and Binary
  Module 2: Object and Lifetime
  Module 3: Value Category and Transfer
  Module 4: Generic Programming
  Module 5: Runtime Architecture
  Module 6: Low-level Performance

Track B: Pixel-Renderer Implementation
  Module 1: Current Architecture
  Module 2: Toolchain and Build
  Module 3: Framebuffer and Backend Refactor
  Module 4: UI / Shader / Command Integration Boundary

Status
  只顯示新版 T/I 章節與後續處理方向
```

重點：使用者進入教學時只會看到統一版主線。早期章節檔案已移除，遷移關係只保留在 roadmap/audit 作為維護紀錄。

## 6. 已完成的生成順序紀錄

### Phase 1: 課程結構重整

1. 新增本 roadmap。
2. 改 `index.html`，讓新版 T/I 章節成為唯一主線。
3. 在 roadmap/audit 保留遷移紀錄，index 不再顯示舊章節作為閱讀入口。
4. 決定新版檔名規則。

### Phase 2: 已重寫最影響理解的 theory

1. T-01 Compilation / Linkage / ABI
2. T-03 Object Model / Lifetime / UB
3. T-04 Value Category / Reference System
4. T-05 Copy / Move / Object Transfer
5. T-08 Templates / Concepts / Type Traits

### Phase 3: 已接上 implementation track

1. I-02 Toolchain Map
2. I-03 Build Workflow: g++ driver / VSCode / GDB process control / Make / CMake
3. I-04 Debugger Fundamentals: GDB / LLDB
4. I-05 VSCode Setup
5. I-06 Make Workflow
6. I-07 CMake Target-based Build
7. I-08 Framebuffer Ownership Refactor
8. I-09 DisplayBackend Interface

### Phase 4: 已回補 renderer-oriented engineering topics

1. T-12 Low-level Performance / Memory Layout
2. I-13 Renderer Frontend / Backend Engineering Boundary
3. I-14 Immediate-mode UI Integration Boundary
4. I-15 Shader Pipeline Interface Boundary
5. T-13 Concurrency / Job System Intro

## 7. 判斷一章是否完成的標準

一章不是「寫了很多內容」就完成。每章至少要滿足：

- 有一個具體工程問題作為開場。
- 有 naive model，並指出它為什麼不夠。
- 有正式語意規則。
- 有至少一個 renderer-oriented 例子。
- 有常見錯誤。
- 有三個 takeaways。
- 如果是 implementation 章，必須有可驗證的檔案、指令或 refactor step。

## 8. 當前結論

舊 19 章已被新版 T/I 章節吸收，不再作為最終課程結構或平行入口。新版內容若已能講清楚同一組觀念，就不應保留新舊兩套主線。

下一步不應急著加 Ch20，而應先把 index 從「完成章節列表」改成「整合版課程入口」。接著挑最核心的 theory 章重寫，優先順序是：

```text
T-01 Compilation / Linkage / ABI
T-03 Object Model / Lifetime / UB
T-04 Value Category / Reference System
```

這三章會決定後面所有 C++ 深度章節的地基。

## 9. 思考深度判準

後續章節不應被固定成同一種模板。`I-04 Debugger Fundamentals` 的價值不是形式，而是思考深度：它沒有停在 GDB 指令表，而是拆到 process、trap、debug symbols、stack frame、memory。其他章節也要有同等層次的推理，但結構可以依主題調整。

也就是說，章節不能只回答「這個 API 怎麼用」或「這個語法是什麼」。每章都應該檢查自己是否有走到以下思考層級：

```text
1. Concrete Problem
   先用一個專案或語言現象開場。
   例：為什麼 SetPixel crash 時 print x 有時看不到？

2. Naive Model
   明確寫出直覺模型。
   例：debugger 好像是在讀 C++ source line。

3. Failure Point
   說明 naive model 在哪裡失效。
   例：CPU 執行 machine instruction，不執行 C++ source。

4. Underlying Mechanism
   拆到實際機制：process、memory、object lifetime、ABI、compiler transform、OS API。
   例：breakpoint 是 trap instruction；watchpoint 常靠 CPU debug registers。

5. Formal / Semantic Rule
   回到 C++ 語意或工程規則。
   例：expression 有 type 與 value category；object lifetime 不等於 raw storage 存在。

6. Renderer Consequence
   接回 Pixel-Renderer。
   例：framebuffer pitch 不能假設永遠是 width * 4；DisplayBackend 不應污染 RenderDevice。

7. Verification Method
   給出可驗證方法：GDB 指令、build command、small code sample、refactor checklist。
   例：用 conditional breakpoint 只停在 x == 400 && y == 300 的 SetPixel。
```

這不是要求每章都照同一個順序、同一個長度、同一種小標題。不同主題可以有不同鋪陳：

- build/toolchain 章可以從 command flow 和 dependency graph 切入。
- C++ 語意章可以從 expression / object / lifetime 的規則切入。
- renderer architecture 章可以從 ownership boundary 和 data flow 切入。
- UI/shader 章可以從資料生命週期、dispatch、command buffer 切入。

真正的要求是避免章節變成名詞百科。每章都必須讓讀者知道：

- 這個主題解決什麼真問題。
- 為什麼直覺理解不夠。
- 底層實際發生什麼。
- C++ 語意如何抽象化這件事。
- 對 renderer 工程設計有什麼後果。
- 如何在本專案裡驗證。

## 10. 前向整體編排

目前最重要的不是繼續往後追加章節，而是讓整套教學形成清楚的依賴路徑。建議把新版課程分成六個 Part，每個 Part 都有自己的結束能力。

### Part 0: Program as Binary

目的：先理解 C++ 程式不是 source file，而是被 compiler、linker、loader、OS、debugger 共同處理的 binary/process。

```text
T-01  Compilation / Linkage / ABI
I-02  Toolchain Map
I-03  Build Workflow
I-04  Debugger Fundamentals
I-05  VSCode Build / Debug / IntelliSense Setup
I-06  Make Workflow
I-07  CMake Target-based Build
I-19  Preprocessor / Compile-time Configuration
I-20  Dependency Management / Third-party Integration
I-18  Diagnostics / Sanitizers / Static Analysis
```

Part 結束時應能回答：

- `g++ main.cpp` 背後做了哪些階段？
- header、object file、symbol、library、ABI 的邊界在哪？
- VSCode task、launch、IntelliSense 各自包住哪個工具？
- GDB/LLDB 停住的是 source line、machine address，還是 process state？
- `#ifdef`、platform macro、feature macro 應該停在哪個 compile-time boundary？
- 第三方 library 進專案時，是 include path、library path、linker input，還是 runtime dependency？
- warnings、ASan、UBSan、static analysis 各自能觀測哪一層錯誤？

### Part 1: Object Exists in Storage

目的：把 C++ object model 講深。這是後面 RAII、move、container、framebuffer ownership 的地基。

```text
T-02  Type System / Initialization
T-03  Object Model / Storage / Lifetime / UB
T-18  Casts / Type Punning / Bit Cast
T-06  RAII / Resource Ownership / Exception Safety
T-17  Smart Pointer / Ownership Adapters
```

Part 結束時應能回答：

- storage 存在是否代表 object 已經存在？
- initialization、construction、lifetime begin/end 有什麼差別？
- cast 是否真的改變 object lifetime、alignment、aliasing permission？
- destructor 為什麼是 C++ 解決 C resource discipline 的核心？
- owned resource、borrowed view、observer pointer 應該如何分辨？
- framebuffer、DIBSection、SDL_Texture、file handle 這類 resource 應由誰釋放？
- `unique_ptr`、`shared_ptr`、`weak_ptr`、raw observer、resource handle 各自改變哪一種 lifetime policy？

### Part 2: Values Move Through the Program

目的：完整處理 value category、reference、copy、move、return by value，而不是只講 `std::move`。

```text
T-04  Value Category / Reference System
T-22  Const / cv-qualification / Value Category
T-23  Overload Resolution / Conversion / Operator Design
T-24  Type Deduction / auto / decltype / Forwarding
T-25  Static Storage / Global State / Initialization Order
T-26  Regular Types / Equality / Hashing / Ordering
T-05  Copy / Move / Object Transfer
T-07  Callable / Overload / Lambda
```

Part 結束時應能回答：

- expression 的 type 和 value category 為什麼是兩件事？
- `const` 如何限制 mutation capability？為什麼 `std::move(const T&)` 通常不會真正 move？
- overload resolution 和 conversion policy 如何影響 operator、strong id、resource handle？
- `T&`、`const T&`、`T&&`、forwarding reference 分別在保證什麼？
- operator overload 會不會回傳 dangling reference？
- lambda capture by reference 在 UI command 或 render callback 裡為什麼危險？
- shader object、texture handle、framebuffer view 應該可 copy、move-only，還是不可移動？

### Part 3: Types Become Generic Contracts

目的：讓泛型不只是 template 語法，而是 type 必須滿足的語意合約。

```text
T-08  Templates / Concepts / Type Traits
T-09  STL / Containers / Iterators / Allocator Basics
T-10  Error Handling / Contracts / Invariants
```

Part 結束時應能回答：

- template 參數不是「任意型別」，而是要滿足哪些 operation 和 law？
- `std::vector<T>` reallocation 對 `T` 的 move/copy/noexcept 有什麼要求？
- `std::span`、`std::string_view` 這類 view 如何避免 ownership 混淆？
- command buffer、vertex buffer、UI draw list 應用哪種 container？
- error 是 invariant violation、recoverable failure，還是 caller contract 被破壞？

### Part 4: Architecture Boundaries

目的：開始把 C++ 語意用在 Pixel-Renderer 的抽象分離。這裡要避免把 Win32/SDL/macOS、renderer core、UI、shader pipeline 混成一團。

```text
T-11  Runtime Polymorphism / Type Erasure / Variant
I-01  Current Renderer Architecture Walkthrough
I-08  Framebuffer Ownership Refactor
I-09  DisplayBackend Interface
I-10  Win32DisplayBackend
I-11  SDLDisplayBackend
I-12  Cross-platform Development Path
I-13  Renderer Frontend / Backend Engineering Boundary
T-16  API Design Conventions
```

Part 結束時應能回答：

- `RenderDevice::SetPixel` 寫的是 owned CPU framebuffer，還是 platform-owned memory？
- `DisplayBackend::Present` 的責任邊界是什麼？
- 什麼時候用 virtual interface？什麼時候用 template policy？什麼時候用 function object？
- Win32 `BitBlt`、SDL `UpdateTexture`、macOS present path 如何共用同一個 renderer core？
- 怎樣才算真的前後端分離，而不是只是把 class 名字改掉？
- 如何把 owner/view/borrow、const-correctness、failure mode 寫進 API signature？

### Part 5: Performance and Future Systems

目的：把 low-level performance、memory layout、concurrency 連到未來 shader、UI、job system，但不搶圖學筆記的篇幅。

```text
T-12  Low-level Performance / Memory Layout
T-13  Concurrency / Memory Model / Job System Intro
T-14  Modern C++ Boundaries
T-15  Numeric Semantics / Pixel Formats / Bit Operations
I-14  Immediate-mode UI Integration Boundary
I-15  Shader Pipeline Interface Boundary
I-16  Verification / Golden Image / Regression Testing
I-17  Header Boundary / Dependency Hygiene
I-18  Diagnostics / Sanitizers / Static Analysis
```

Part 結束時應能回答：

- AoS / SoA 如何影響 vertex、fragment、UI command data？
- cache locality 和 memory bandwidth 如何限制 software rasterizer？
- packed pixel、integer overflow、float rounding 如何影響 color/depth/test determinism？
- tile-based parallel rasterization 會碰到哪些 data race？
- immediate-mode UI draw data 如何進 renderer，但不反向依賴 renderer internal？
- shader pipeline interface 如何先用 C++ type system 表達，並讓每個 stage 都能測試與 debug？
- warnings、sanitizers、static analysis 如何和 golden image tests 一起定位 renderer bug？

## 11. 接下來的工作邊界

現在已經有 `T-01`、`T-02`、`T-03`、`T-04`、`T-05`、`T-06`、`T-07`、`T-08`、`T-09`、`T-10`、`T-11`、`T-12`、`T-13`、`T-14`、`T-15`、`T-16`、`T-17`、`T-18`、`T-22`、`T-23`、`T-24`、`T-25`、`T-26`、`I-01`、`I-02`、`I-03`、`I-04`、`I-05`、`I-06`、`I-07`、`I-08`、`I-09`、`I-10`、`I-11`、`I-12`、`I-13`、`I-14`、`I-15`、`I-16`、`I-17`、`I-18`、`I-19`、`I-20`，以及 Appendix `A-01 auto_ptr / move semantics deep dive`、`A-02 pointer semantics map`。新版教程骨架、早期補圖、index 統一、舊章節移除、numeric semantics、diagnostics、API design、smart pointer ownership adapter、casts/type punning、const/cv/value category、overload/conversion/operator、type deduction、static/global state、regular/equality/hash、preprocessor、dependency management、auto_ptr historical deep dive、pointer semantics deep dive 缺口都已完成；README 暫不動，除非使用者要求。

第二輪審查詳見 `docs/tutorial-cpp/AUDIT.md`。Phase A/B/C/D/D.1 已完成：結構缺口已補、骨架章已加深、早期章節已補 SVG / memory-style 圖解、index 已改成統一版入口、舊 HTML 章節已移除。

接下來分成兩條可能路線：

```text
1. 文件校稿 / 局部加深
   檢查各章是否還有 stale wording、章末 next link、SVG caption、verification 方法不足。

2. 第三輪缺口補強
   Diagnostics / Sanitizers / Static Analysis、API Design Conventions、Part dependency map、preprocessor、dependency management 已補。

3. 第四輪缺口補強
   重新審視後，下一批缺口不是 move/RAII 這類 C++ 核心語意，而是 renderer 專案工程化會需要的 frame loop、asset/resource、profiling、test infrastructure、standard utilities、arena/PMR、data-oriented design。value category 與 const/cv-qualification 交界已補成 T-22。

4. 第五輪語意交界補強
   沿著 T-22 的角度，overload/conversion/operator 已補成 T-23，type deduction 已補成 T-24，static/global state 已補成 T-25，regular/equality/hash/order 已補成 T-26；下一批應檢查 exception guarantee、backend init cleanup、nodiscard 這類「語法看起來簡單，但實際語意跨很多層」的主題。

5. src architecture refactor
   依 I-08 到 I-13 的設計，把 owned Framebuffer、DisplayBackend、Win32/SDL backend selection 逐步落地。
```

這樣安排的理由：

- `T-03` 要先於 framebuffer ownership，否則 owned buffer / borrowed pointer / platform resource 會講不清楚。
- `T-04`、`T-22`、`T-23`、`T-24`、`T-25`、`T-26` 要先於 shader/UI callback，否則 reference、const capability、move-from-const、overload/conversion、type deduction、static/global lifetime、regular/equality/hash/order、lambda capture、operator return category 的 dangling 或 cache-key 問題會一直散落。
- `I-05`、`I-06`、`I-07`、`I-19`、`I-20` 要先於 SDL/macOS backend；現在工具鏈、Make、CMake target boundary、compile-time configuration、third-party dependency boundary 已經有教學地基，可以開始進入 ownership refactor。
- `I-01` 到 `I-20` 已經把 current architecture、toolchain、debugger、diagnostics、build workflow、preprocessor、dependency management、framebuffer ownership、DisplayBackend、Win32/SDL/cross-platform、frontend/backend boundary、UI integration、shader pipeline boundary、verification、header hygiene 串成第一版工程地圖；`T-01` 到 `T-18` 也已經把 C++ 語意、containers、contracts、polymorphism、performance、concurrency、Modern C++ boundary、numeric semantics、API design conventions、smart pointer ownership adapter、casts/type punning 補齊。第二輪審查指出 FPGA 暫不進近期主線；testing / verification 已補成 implementation 章節；index 已補 Part dependency map。

第四輪審查後，建議新增或加深的下一批主題：

```text
P0  I-24 Test Infrastructure / CTest / CI
    進 src refactor 前需要 test target / golden data / CI matrix 的工程地基。

P1  I-21 Frame Loop / Event Loop / Input Abstraction
    DisplayBackend 不只 present，也牽涉 OS event、input state、delta time、vsync。

P1  I-22 Asset / Resource Pipeline
    UI font atlas、texture sampling、shader resources 都需要 file/resource lifetime 地圖。

P2  I-23 Profiling / Benchmarking / Performance Regression
    renderer 優化要從 measurement/profiler/benchmark policy 開始。

P2  T-19 Standard Library Utilities for Systems Projects
    補 filesystem、chrono、format/source_location 等專案工具。

P3  T-20 Memory Resources / Arena / Scratch Allocation
P3  T-21 Data-oriented Design for Renderer Data
    作為 UI/shader/parallel renderer 的進階語意與資料佈局地基。
```

若沿著 C++ 語意交界繼續補 theory，`T-26` 已完成；下一步不一定要新增章，較值得做的是局部加深：

```text
P1  加深 T-05 / T-06 / T-10
    補 assignment、exception guarantee、nodiscard、multi-step backend init cleanup。
```

若下一步要進程式碼，仍應先處理 Phase E 的 architecture refactor；若要繼續補教學，工程向優先補 `I-24`，語意向優先做 `T-05` / `T-06` / `T-10` 的局部加深。

## 12. Appendix / Deep Dive 候選池

Appendix 的判準不是「這個主題比較不重要」，而是它適合用 problem genealogy / semantic archaeology 的方式深挖：從歷史問題、naive solution、failure mode、語意 invariant、相鄰系統比較，一路推到現代設計。這類內容若放在主線，會拖慢正式章節；但若完全不寫，又會少掉森林視角。

已完成：

```text
A-01  auto_ptr 與 Move Semantics 的誕生
      從 destructive copy 看 C++11 rvalue reference / unique_ptr 為什麼存在。

A-02  Pointer 語意地圖：address、borrow、owner、handle
      raw pointer / reference / span / unique_ptr / shared_ptr / weak_ptr / resource id 的語意分層。

A-06  RAII 為什麼是 C++ 的核心，不只是 destructor 技巧
      從 C cleanup path、goto fail、exception unwinding、partial initialization 推出 type-level lifetime invariant。

A-10  Rule of Zero / Five 的歷史壓力
      從 raw resource owner、copy bug、double free、move-only type 推到為什麼 Rule of Zero 是現代預設。

A-16  dynamic_cast / RTTI 為什麼存在，又為什麼常被避免
      從 runtime type identity、polymorphic base、downcast pressure、capability query 推到 virtual / variant / type erasure / template 的替代路線。

A-17  std::function 的成本與語意
      從 lambda capture、type erasure、small buffer optimization、allocation、callback lifetime 解析。

A-22  為什麼 C++ 沒有真正的 interface keyword
      從 abstract class、pure virtual、concepts、type erasure、ABI boundary 比較不同 interface 意義。

A-25  Error code / exception / expected 的設計分歧
      從 backend init、asset loading、programmer bug、recoverable failure 分出錯誤語意。

A-32  Error Literacy：從 g++ 訊息到錯誤分層
      先分辨 compiler / linker / runtime / assert / sanitizer error，再學會拆 g++ 的 error、note、candidate、required from。
```

建議候選：

```text
A-03  Header / Linker 錯誤考古
      從 undefined reference、duplicate symbol、unresolved external 回推 declaration/definition/TU/linker/ABI。

A-04  SDL Present Path 考古：從 BitBlt 到 OS compositor
      Win32 DIBSection、SDL texture、renderer backbuffer、GPU/driver、OS compositor、display scanout 的跨平台資料流。

A-05  CMake 為什麼存在：從手打 g++ 到 target graph
      compiler command、object files、include path、link flags、package discovery、runtime artifact 如何逼出 target-based build。

A-07  Type Erasure / Virtual / Variant 的設計壓力史
      從 plugin boundary、hot path dispatch、closed/open set tradeoff，看 runtime polymorphism alternatives。

A-08  Testing Golden Image 的問題生成史
      從 pixel-perfect fragility、numeric tolerance、determinism、platform variance 推出 renderer verification policy。

A-09  noexcept 為什麼會影響 move / container / performance
      從 vector reallocation 為什麼有時 copy、有時 move，追到 exception guarantee 與 type trait。

A-11  const 不是 immutable：C++ const 的語意邊界
      從 const member function、mutable、logical constness、thread-safety 誤解拆起。

A-12  UB 為什麼不是 runtime error
      從 signed overflow、strict aliasing、dangling reference 看 compiler optimization contract。

A-13  Header-only library 為什麼可行又危險
      從 template instantiation、ODR、inline、compile time、binary size 追到工程取捨。

A-14  inline 的語意演化：不是只叫 compiler 展開函式
      從 multiple definition 問題、ODR、linkage 到 modern inline variable。

A-15  virtual destructor 為什麼重要
      從 base pointer delete derived object 的失敗案例推到 polymorphic ownership contract。

A-18  span / string_view 為什麼不是 owner
      從 dangling view、temporary lifetime、buffer resize、command buffer payload 設計切入。

A-19  allocator / pmr 為什麼存在
      從 STL container 為什麼要抽象 allocation，推到 arena、scratch allocator、frame allocator。

A-20  vector reallocation 的完整問題史
      從 pointer invalidation、iterator invalidation、move noexcept、capacity policy 追到 command buffer 設計。

A-21  enum class / strong typedef 為什麼重要
      從 int 混用 TextureId / ShaderId / Width / Height 的 bug 推到 domain type。

A-23  Compile-time polymorphism vs runtime polymorphism
      從 shader/backend policy、template bloat、vtable cost、binary boundary 做問題分類。

A-24  C API boundary 的 C++ 包裝策略
      從 SDL / Win32 raw handle、error code、opaque pointer、custom deleter 推到 wrapper policy。

A-26  Working directory 為什麼會害 asset loading 壞掉
      從 VSCode launch、terminal cwd、relative path、filesystem path policy 推到 asset root 設計。

A-27  DLL / shared library / static library 的差異
      從 link time、load time、symbol visibility、runtime dependency、Windows/macOS/Linux 差異整理。

A-28  ABI stability 為什麼難
      從 class layout、vtable、name mangling、standard library ABI、plugin boundary 說明。

A-29  Immediate-mode UI 為什麼適合 renderer tool
      從 retained UI state、command generation、frame lifetime、input event ownership 比較。

A-30  Golden image test 為什麼不能只做 pixel-perfect
      從 floating error、platform difference、tolerance、mask、debug diff image 推到 renderer verification。

A-31  Reflection / Metadata：為什麼 editor、serializer、debug UI 需要把 type 當資料
      從 C# reflection 對照 C++ RTTI，推到 explicit metadata registry、field table、macro / codegen / template reflection-like layer。
```
