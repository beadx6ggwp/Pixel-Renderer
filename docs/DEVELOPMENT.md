# Development Workflow

日期：2026-05-22

這份文件定義 Pixel-Renderer 的輕量開發規範。目標不是建立複雜流程，而是讓 `main` 的 history 可讀、branch 不互相污染、experiment 可以存在但不破壞主線。

---

## 1. Main Branch Semantics

`main` 代表整理過的專案主線：

```text
main = readable, explainable, preferably buildable history
```

`main` 不應該長期包含：

```text
unfinished experiment
unclear WIP commit
large unrelated mixed change
temporary debug output
generated test diff
```

如果某次 commit 讓 `main` 暫時不能 build，commit message 或後續 commit 必須清楚說明原因。

---

## 2. Branch Naming

Branch name 應描述工程意圖，不使用工具名稱。

建議 prefix：

```text
docs/       documentation, roadmap, stable notes
render/     rasterization and graphics pipeline work
arch/       architecture refactor and ownership boundaries
test/       automated tests and test harness
debug/      debug visualization, trace, runtime inspection tools
exp/        experiments that may not merge directly
perf/       performance experiments or optimizations
build/      build system and toolchain work
```

近期可能使用：

```text
docs/project-map
docs/foundations
arch/framebuffer-display
render/raster-baseline
test/render-core
debug/pipeline-trace
exp/gjk-2d
```

避免一開始開太多 branch。先讓工作自然分出邊界，再開新 branch。

---

## 3. Local vs Remote Branches

預設使用 local branch。

短任務：

```text
local branch -> commit -> merge main -> push main
```

長任務或高風險任務：

```text
local branch -> push remote branch -> commit -> merge main -> delete remote branch
```

需要 push remote branch 的情況：

```text
1. branch 會做超過一天
2. 改動風險高，例如 display/backend refactor
3. 需要備份
4. 需要跨電腦工作
5. 需要 GitHub PR / discussion
6. 需要 CI 跑在 branch 上
```

Remote branch 是暫時工作線，不是永久歷史保存區。merge 後可以刪掉。

---

## 4. Commit Message Convention

採用簡化版 Conventional Commits：

```text
type(scope): summary
```

常用 type：

```text
docs       documentation
feat       new feature
fix        bug fix
refactor   behavior-preserving code restructuring
test       tests
debug      debug tooling or visualization
perf       performance work
build      build system, makefile, CMake, toolchain
chore      maintenance
exp        experiment, not guaranteed to merge into main
```

常用 scope：

```text
project
docs
render
raster
line
triangle
depth
math
framebuffer
display
backend
sdl
win32
test
debug
ui
gjk
```

Examples:

```text
docs(project): add docs structure
docs(architecture): define display backend boundary

feat(render): add DDA line reference
feat(render): add Bresenham line rasterizer
feat(triangle): add bounding box rasterization
feat(depth): add depth buffer

test(raster): add barycentric coordinate cases
debug(render): add barycentric debug view

refactor(framebuffer): introduce owned framebuffer
refactor(display): add display backend interface

exp(gjk): prototype 2d simplex collision
```

---

## 5. Experiments

Experiments are allowed, but they must not silently become production code.

Use `exp/*` for:

```text
DDA vs Bresenham comparison
triangle edge-rule exploration
GJK collision prototype
temporary math experiments
```

Experiment branch rules:

```text
1. It may contain messy commits.
2. It may contain multiple competing implementations.
3. It should not be merged directly into main.
4. Useful results should be extracted into docs, tests, or a clean feature branch.
```

Example:

```text
exp/dda-vs-bresenham
  -> docs: record tradeoffs
  -> test: keep DDA as reference if useful
  -> render: merge only the selected production DrawLine path
```

---

## 6. Definition of Done

Every branch should have a small done condition before it grows too large.

Examples:

```text
docs/project-map done when:
  PROJECT_MAP.md exists
  docs directory skeleton is documented
  planning notes are linked

render/raster-baseline done when:
  DDA reference exists
  Bresenham candidate exists
  triangle bbox / barycentric baseline exists
  minimal demo or tests exist

arch/framebuffer-display done when:
  Framebuffer owns pixels
  RenderDevice writes Framebuffer
  DisplayBackend presents Framebuffer
  Win32 path still works or limitation is documented

test/render-core done when:
  minimal test runner exists
  barycentric / bbox / depth tests exist
  generated outputs are ignored or documented
```

If the branch starts pulling in unrelated work, split it or stop after the current done condition.

---

## 7. Docs and Code Sync

Use two kinds of documentation commits:

```text
design docs:
  document a decision or plan before implementation

implementation docs:
  update docs to match code that already changed
```

Examples:

```text
docs(architecture): define display backend boundary
refactor(display): introduce display backend interface
docs(display): update backend flow after framebuffer split
```

Code changes should ideally include at least one of:

```text
test
demo
debug trace
docs update
```

Docs-only branches can merge independently if they keep the project structure clear.

---

## 8. Merge Policy

Small docs or maintenance changes may be merged directly after clean commits.

Larger features should use a branch:

```text
git switch -c render/raster-baseline
```

Before merging:

```text
1. Review git status.
2. Remove temporary files.
3. Make commits readable.
4. Run relevant build/test if available.
5. Check whether docs need updates.
```

For important milestones, prefer a merge commit:

```text
git switch main
git merge --no-ff render/raster-baseline
```

This keeps the feature as a readable group in `git log --graph`.

For small linear changes, rebase/fast-forward is fine.

---

## 9. Generated Files

Generated artifacts should not enter git unless they are intentional test fixtures.

Likely ignored outputs:

```text
build/
*.exe
*.o
test-output/
golden-diff/
trace-output/
```

Golden images are allowed only when they are stable reference fixtures. Diff images and failed outputs should stay ignored.

---

## 10. Current Near-Term Flow

Recommended near-term branch sequence:

```text
1. docs/project-map
   commit current docs structure and planning notes

2. docs/foundations
   write rendering_conventions.md and display_backend_architecture.md

3. render/raster-baseline
   implement DDA / Bresenham / triangle / barycentric baseline

4. arch/framebuffer-display
   separate owned Framebuffer and DisplayBackend boundary

5. test/render-core
   add unit tests, debug trace, and first golden-image foundation
```

This sequence may change, but changes should preserve the same principle:

```text
correctness -> observability -> clear architecture -> performance
```

