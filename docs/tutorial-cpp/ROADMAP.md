# C++ Tutorial Integration Roadmap

這份 roadmap 的目的不是再加一份待辦清單，而是把目前 `docs/tutorial-cpp/` 重新整理成一套連貫課程。

目前已完成的 19 章有價值，但它們比較像第一階段筆記：以 Pixel-Renderer 的 ownership、move、RAII、virtual/backend abstraction 為主線。下一階段不能只是把新章節接在後面，否則會變成「舊 19 章 + 新 roadmap」兩套東西並排存在。

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

### 1.4 舊章節不是全部保留，也不是全部重寫

每個舊章節要被標記為：

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
T-04  Value Category / Reference System
T-05  Copy / Move / Object Transfer
T-06  RAII / Resource Ownership / Exception Safety
T-07  Callable / Overload / Lambda
T-08  Templates / Concepts / Type Traits
T-09  STL / Containers / Iterators / Allocator Basics
T-10  Error Handling / Contracts / Invariants
T-11  Runtime Polymorphism / Type Erasure / Variant
T-12  Low-level Performance / Memory Layout
T-13  Concurrency / Memory Model / Job System Intro
T-14  Modern C++ Boundaries
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
```

## 3. 舊章節遷移表

| 舊章節 | 新位置 | 處理 | 理由 |
|---|---|---|---|
| Ch01 編譯模型 | T-01 + I-02/I-03 | split + deepen | Ch01 應擴成 declaration/definition、ODR、linkage、ABI；toolchain/CMake 則放 implementation。 |
| Ch02 記憶體四區 | T-03 + T-12 | deepen | 目前有 stack/heap/static/code，但還要補 storage duration、object lifetime、alignment、object representation、UB、cache locality。 |
| Ch03 指標與參考 | T-03 + T-04 | split + deepen | pointer/reference 一部分是 memory alias，一部分是 reference/value category system。 |
| Ch04 struct 與 class | T-02 + T-10 | merge + deepen | struct/class 應接 type system、access control、invariant、aggregate、conversion。 |
| Ch05 建構與解構 | T-03 + T-06 | merge + deepen | constructor/destructor 是 object lifetime，也是 RAII/resource ownership 的入口。 |
| Ch06 Copy 語意 | T-05 + T-06 | merge | copy 應跟 move、resource ownership、exception safety 一起講。 |
| Ch07 Rule of 0/3/5 | T-06 | deepen | 需要補 Rule of Zero 與 exception safety、copy-and-swap、move-only handle、custom deleter。 |
| Ch08 Move 是 ownership transfer | T-05 | merge + deepen | 應放進完整 object transfer：copy/move/elision/moved-from state。 |
| Ch09 std::move 與 value category | T-04 + T-05 | split + deepen | `std::move` 屬於 value category cast；move constructor 屬於 transfer。 |
| Ch10 Return by value 與 RVO | T-04 + T-05 | merge + deepen | 需要接 C++17 prvalue、temporary materialization、guaranteed copy elision。 |
| Ch11 In-place construction | T-03 + T-09 | split | placement new/construct_at 是 storage/lifetime；emplace 是 container API。 |
| Ch12 vector reallocation 與 noexcept move | T-09 + T-06 | split + deepen | vector reallocation 屬於 container；`noexcept move` 屬於 exception safety/resource type design。 |
| Ch13 Type = states + operations + invariants | T-10 | keep + deepen | 方向正確，應補 contract、precondition/postcondition、error handling。 |
| Ch14 Regular / Semiregular types | T-08 + T-09 | merge + deepen | regular 概念應接 concepts、templates、STL algorithm requirements。 |
| Ch15 Operator / Template 與數學型別 | T-02 + T-04 + T-08 | split + deepen | operator 涉及 type operation、value category、overload resolution；template 應獨立加深。 |
| Ch16 智慧指標 | T-06 | keep + deepen | 應補 custom deleter、aliasing pitfalls、observer pointer/span/reference、shared ownership 的判準。 |
| Ch17 繼承、virtual 與 vtable | T-11 | keep + deepen | 應補 type erasure、variant、visitor、static polymorphism alternatives。 |
| Ch18 設計模式速查 | T-11 + I-10/I-11/I-12 | split | pattern 應分成 theory vocabulary 與 renderer command/UI/shader 實作。 |
| I-01 渲染器架構導覽 | I-01 | keep | 作為 implementation track 入口，後續章節從這裡拆 refactor plan。 |

## 4. 缺席但應新增的 C++ 主題

### 4.1 Compilation / Linkage / ABI

目前 Ch01 還不夠完整。應補：

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

目前缺：

- default/value/direct/copy/list initialization
- aggregate initialization
- narrowing
- `explicit`
- implicit conversion sequence
- `const`, `constexpr`, `consteval`
- `enum class`
- initialization order

### 4.3 Object Model / UB

目前缺：

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

目前只接到 `std::move`，應完整補：

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

### 4.5 Callable / Lambda / Overload

目前幾乎缺席：

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

目前只有 operator/template 數學型別入門。應補：

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

目前只講 vector reallocation。應補：

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

### 4.8 Error Handling / Contracts

目前分散在 invariant。應補：

- `assert`
- `optional`
- expected-like result
- exception
- `noexcept`
- error code
- precondition/postcondition
- fail-fast vs recoverable error

### 4.9 Low-level Performance

目前缺：

- cache line
- cache locality
- alignment/padding
- false sharing
- branch prediction basics
- SIMD intro
- AoS vs SoA
- memory bandwidth
- pointer aliasing and optimizer

### 4.10 Concurrency / Memory Model

不必太早深挖，但 roadmap 要保留位置：

- thread
- mutex
- atomic
- data race
- memory order intro
- producer-consumer
- tile/job system intro

## 5. 新版 index 應如何改

目前 `index.html` 已新增 roadmap，但仍保留「第一階段 19 章」的線性列表。下一次 index 重構應改成：

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

Legacy / First-stage chapters
  Show migration status instead of treating them as final structure
```

重點：不要讓使用者以為 `Ch01-Ch18` 是最終順序。它們是第一階段素材，會被新版 track 吸收。

## 6. 建議實作順序

### Phase 1: 課程結構重整

1. 新增本 roadmap。
2. 改 `index.html`，把現有 19 章標成 first-stage material。
3. 新增 migration/status table 到 index 或獨立頁面。
4. 決定新版檔名規則。

### Phase 2: 先重寫最影響理解的 theory

1. T-01 Compilation / Linkage / ABI
2. T-03 Object Model / Lifetime / UB
3. T-04 Value Category / Reference System
4. T-05 Copy / Move / Object Transfer
5. T-08 Templates / Concepts / Type Traits

### Phase 3: 接 implementation track

1. I-02 Toolchain Map
2. I-03 Build Workflow: g++ driver / VSCode / GDB process control / Make / CMake
3. I-04 Debugger Fundamentals: GDB / LLDB
4. I-05 VSCode Setup
5. I-06 Make Workflow
6. I-07 CMake Target-based Build
7. I-08 Framebuffer Ownership Refactor
8. I-09 DisplayBackend Interface

### Phase 4: 回補 renderer-oriented engineering topics

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

目前 19 章應視為第一階段素材，不是最終課程結構。

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
```

Part 結束時應能回答：

- `g++ main.cpp` 背後做了哪些階段？
- header、object file、symbol、library、ABI 的邊界在哪？
- VSCode task、launch、IntelliSense 各自包住哪個工具？
- GDB/LLDB 停住的是 source line、machine address，還是 process state？
- 第三方 library 進專案時，是 include path、library path、linker input，還是 runtime dependency？

### Part 1: Object Exists in Storage

目的：把 C++ object model 講深。這是後面 RAII、move、container、framebuffer ownership 的地基。

```text
T-02  Type System / Initialization
T-03  Object Model / Storage / Lifetime / UB
T-06  RAII / Resource Ownership / Exception Safety
```

Part 結束時應能回答：

- storage 存在是否代表 object 已經存在？
- initialization、construction、lifetime begin/end 有什麼差別？
- destructor 為什麼是 C++ 解決 C resource discipline 的核心？
- owned resource、borrowed view、observer pointer 應該如何分辨？
- framebuffer、DIBSection、SDL_Texture、file handle 這類 resource 應由誰釋放？

### Part 2: Values Move Through the Program

目的：完整處理 value category、reference、copy、move、return by value，而不是只講 `std::move`。

```text
T-04  Value Category / Reference System
T-05  Copy / Move / Object Transfer
T-07  Callable / Overload / Lambda
```

Part 結束時應能回答：

- expression 的 type 和 value category 為什麼是兩件事？
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
I-08  Framebuffer Ownership Refactor
I-09  DisplayBackend Interface
I-10  Win32DisplayBackend
I-11  SDLDisplayBackend
I-12  Cross-platform Development Path
I-13  Renderer Frontend / Backend Engineering Boundary
```

Part 結束時應能回答：

- `RenderDevice::SetPixel` 寫的是 owned CPU framebuffer，還是 platform-owned memory？
- `DisplayBackend::Present` 的責任邊界是什麼？
- 什麼時候用 virtual interface？什麼時候用 template policy？什麼時候用 function object？
- Win32 `BitBlt`、SDL `UpdateTexture`、macOS present path 如何共用同一個 renderer core？
- 怎樣才算真的前後端分離，而不是只是把 class 名字改掉？

### Part 5: Performance and Future Systems

目的：把 low-level performance、memory layout、concurrency 連到未來 shader、UI、job system，但不搶圖學筆記的篇幅。

```text
T-12  Low-level Performance / Memory Layout
T-13  Concurrency / Memory Model / Job System Intro
T-14  Modern C++ Boundaries
I-14  Immediate-mode UI Integration Boundary
I-15  Shader Pipeline Interface Boundary
```

Part 結束時應能回答：

- AoS / SoA 如何影響 vertex、fragment、UI command data？
- cache locality 和 memory bandwidth 如何限制 software rasterizer？
- tile-based parallel rasterization 會碰到哪些 data race？
- immediate-mode UI draw data 如何進 renderer，但不反向依賴 renderer internal？
- shader pipeline interface 如何先用 C++ type system 表達，再留給未來 GPU/FPGA model？

## 11. 接下來生成順序

因為現在已經有 `T-01`、`T-02`、`T-03`、`T-04`、`T-05`、`T-06`、`T-07`、`T-08`、`T-09`、`I-02`、`I-03`、`I-04`、`I-05`、`I-06`、`I-07`、`I-08`、`I-09`、`I-10`、`I-11`、`I-12`，下一輪不要只沿著 implementation 一路寫到所有 backend。應該繼續 theory / implementation 並行，避免工具鏈章節太重、語意地基太晚補。

建議接下來順序：

```text
1. I-13 Renderer Frontend / Backend Engineering Boundary
   回到工程架構，把 renderer frontend、software backend、display backend、future shader/UI boundary 分層。

2. T-10 Error Handling / Contracts / Invariants
   接續 container/data structure 後，補 assert、precondition、postcondition、optional/expected-like pattern 與 renderer invariant。

3. T-11 Runtime Polymorphism / Type Erasure / Variant
   接回 frontend/backend boundary 後，系統整理 virtual、type erasure、variant、visitor、command buffer、template dispatch 的取捨。
```

這樣安排的理由：

- `T-03` 要先於 framebuffer ownership，否則 owned buffer / borrowed pointer / platform resource 會講不清楚。
- `T-04` 要先於 shader/UI callback，否則 reference、lambda capture、operator return category 的 dangling 問題會一直散落。
- `I-05`、`I-06`、`I-07` 要先於 SDL/macOS backend；現在工具鏈、Make、CMake target boundary 已經有教學地基，可以開始進入 ownership refactor。
- `I-08` 已把 framebuffer ownership 的切割線補上，`T-06` 也已補齊 resource-owning type 的 RAII 設計，`I-09` 已定義 DisplayBackend interface，`I-10` 已把 Win32 path 落成 backend 教學，`I-11` 已整理 SDL2/3、SDL_Renderer、UpdateTexture / LockTexture、OS compositor 與 SDL backend 實作邊界，`I-12` 已把 macOS/Ubuntu/Windows 的跨平台 toolchain 與 backend selection 補上，`T-07` 已補 callback/lambda lifetime，`T-08` 已補泛型 contract，`T-09` 已把 vector/span/iterator invalidation、contiguous storage、allocator basics 連回 framebuffer、vertex buffer、UI command buffer；下一步應回到 renderer frontend/backend 工程邊界。
