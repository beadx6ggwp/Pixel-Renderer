# Pixel-Renderer Experiments

`experiments/` 用來回答「實際執行後會發生什麼」. 它保存 algorithm comparison、behavior probe、architecture spike、build/toolchain trial, 以及可能失敗的假設驗證.

這個目錄直接存在於 `main`. 建立 experiment 不代表一定要建立 branch. Branch 只在工作需要隔離另一份 repository state 時使用.

## Learning, Experiment, And Production

```text
learning/
  understand why and how a mechanism works

experiments/
  compare alternatives or observe actual behavior

src/
  contain the implementation currently selected by the project

tests/
  preserve behavior that is already known and must not regress

docs/
  state durable conventions and decisions the project can rely on
```

Not every question must pass through every area. A discussion may end without a file, a learning note may remain a learning note, and an experiment may validly conclude that no candidate should be adopted.

## Experiment Types

### Self-Contained Algorithm Sandbox

Use this when the question is about the mechanism itself, for example DDA vs Bresenham or two edge-function formulations.

```text
experiments/dda-vs-bresenham/
  README.md
  main.cpp
  fixtures/
```

Keep the smallest useful implementation inside the experiment. It does not need to depend on `src/`.

### Integration Experiment

Use this when the question is about the current renderer, for example whether the existing `Rasterizer::DrawTriangle()` produces cracks on a shared edge.

An integration experiment may include public or currently usable headers from `src/` and link only the required implementation files:

```text
experiments/triangle-shared-edge/main.cpp
  -> src/types.h
  -> src/core/render_device.h/cpp
  -> src/render/rasterizer.h/cpp
```

Do not include `.cpp` files from C++ source. Include headers, then compile and link the required translation units explicitly.

### Replacement Prototype

First prefer keeping a competing implementation self-contained under `experiments/`. Use an `exp/*` branch only when the prototype must modify formal source incompatibly, may temporarily break the main build, needs an independent review path, or will conflict with concurrent work.

## Dependency Direction

```text
learning/     ---> src/       allowed for teaching and source reading
experiments/  ---> src/       allowed for behavior probes and integration experiments
tests/        ---> src/       required for formal verification

src/ -X-> learning/
src/ -X-> experiments/
src/ -X-> tests/
```

Outer learning and verification tools may observe the renderer core. The renderer core must not depend on those tools.

Avoid building a second shared framework inside `experiments/`. If multiple experiments need the same capability because it is genuinely part of the renderer, that is a signal to consider extracting a clean production interface or test utility.

## Build Isolation

The root `makefile` builds the formal PixelRenderer application. It must not recursively discover `learning/` or `experiments/`.

Use the lightest build rule suitable for each case:

```text
pure note or recorded observation
  no compilation

single-file experiment
  one compile command in README.md

multi-file or repeatable experiment
  a local Makefile or CMakeLists.txt

formal application
  the root makefile
```

The current source has not yet been separated into a `renderer_core` library. An experiment that uses the headless raster path can explicitly compile its driver with `render_device.cpp` and `rasterizer.cpp`, without linking `Application`, `ScreenManager`, or Win32 presentation code.

Generated executables, temporary traces, benchmark output, failed image diffs, and other observations belong under `build/experiments/` or another ignored output directory. Commit only intentional source, documentation, and stable fixtures.

## Experiment Record

An experiment only needs enough information for a future reader to understand it:

```text
Question
  what uncertainty is being tested

Setup
  inputs, candidates, controlled variables, and build/run command

Observation
  what actually happened

Conclusion
  adopt, reject, defer, or remain uncertain

Next boundary
  whether anything should move to src/, tests/, docs/, or nowhere
```

This is guidance, not a mandatory template. A small experiment may use a short README.

## Result Flow

```text
question
  -> learning/ when the mechanism is not understood
  -> experiments/ when behavior or alternatives need evidence
  -> src/ when a production implementation is selected
  -> tests/ when the expected result becomes a stable contract
  -> docs/ or docs/adr/ when the conclusion becomes a durable project rule
```

If an experiment is rejected, keep a concise negative result when it prevents the same dead end from being repeated. Do not merge temporary debugging code into production merely to preserve the experiment.

## When A Branch Is Useful

Use an `exp/*` branch when:

```text
the experiment modifies formal source in incompatible ways
the formal build may be temporarily broken
two designs need separate repository states
the work is multi-day or conflicts with another source change
the prototype needs independent review, CI, backup, or cross-machine work
```

Calling existing `src/` code, compiling a self-contained experiment, or recording observations does not by itself require a branch.
