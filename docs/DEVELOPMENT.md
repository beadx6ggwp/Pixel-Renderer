# Development Workflow

Last updated: 2026-09-05

這份文件定義 Pixel-Renderer 的輕量開發規範. 目標不是建立複雜流程, 而是讓 `main` 的 history 可讀, branch 不互相污染, experiment 可以存在但不破壞主線.

---

## 1. Main Branch Semantics

`main` 代表整理過的專案主線:

```text
main = source + build/test + stable project docs
main = learning notes + tutorial tracks + small self-contained labs
main = readable, explainable, preferably buildable history
```

`main` 可以包含:

```text
source code
build files
tests
README.md
AGENTS.md
stable docs
renderer conventions
architecture boundaries
verification strategy
roadmap
ADR
learning notes
tutorial tracks
paper readings
small self-contained labs
```

`main` 不應該長期包含:

```text
unfinished experiment
unclear WIP commit
large unrelated mixed change
temporary debug output
generated test diff
unbounded experiment output
private or source-unbounded conversation dumps
```

如果某次 commit 讓 `main` 暫時不能 build, commit message 或後續 commit 必須清楚說明原因.

`learning/` 可以保存在 `main`. 它和 `docs/` 的差別是 authority, 不是 branch: `learning/` 解釋與探索, `docs/` 記錄 current project 可以依賴的 stable truth.

---

## 2. Branch Naming

Branch name 應描述工程意圖, 不使用工具名稱.

建議 prefix:

```text
docs/       stable docs, roadmap, policy, extracted decisions
render/     rasterization and graphics pipeline work
arch/       architecture refactor and ownership boundaries
test/       automated tests and test harness
debug/      debug visualization, trace, runtime inspection tools
exp/        experiments that may not merge directly
perf/       performance experiments or optimizations
build/      build system and toolchain work
```

近期可能使用:

```text
docs/project-policy
docs/foundations
arch/framebuffer-display
render/raster-baseline
test/render-core
debug/pipeline-trace
exp/gjk-2d
```

避免一開始開太多 branch. 先讓工作自然分出邊界, 再開新 branch.

已經 fast-forward merge 回 `main` 的短期 feature branch 可以刪掉. 例如 `docs/main-cleanup` 用完後就不需要保留.

---

## 3. Local vs Remote Branches

預設使用 local branch.

短任務:

```text
local branch -> commit -> merge main -> delete local branch -> push main
```

長任務或高風險任務:

```text
local branch -> push remote branch -> commit -> merge main -> delete temporary branch
```

需要 push remote branch 的情況:

```text
1. branch 會做超過一天
2. 改動風險高, 例如 display/backend refactor
3. 需要備份
4. 需要跨電腦工作
5. 需要 GitHub PR / discussion
6. 需要 CI 跑在 branch 上
```

Remote branch 是暫時工作線, 不是永久的內容分類或歷史保存區. merge 後可以刪掉.

---

## 4. Learning Workspace And Branches

`learning/` 用來保存:

```text
long-form reasoning
learning notes and teaching drafts
paper readings
naive or broken examples
small mechanism-focused labs
historical decision context
rough architecture comparisons
```

這些內容直接與 project 共存在 `main`, 不需要為了「這是學習內容」建立永久 branch. 但它們不是 current source truth; 涉及目前 code、build 或正式 architecture 時仍要驗證 source 與 stable docs.

```text
directory
  classifies what the content is

branch
  isolates an alternative repository state
```

只有在 experiment 或 implementation 需要修改同一批正式 source、可能破壞 build、需要獨立 review, 或會持續多日並與主線衝突時, 才需要 branch. 完全獨立的 note、tutorial、reading 或 small lab 可以直接放進 `learning/`.

當 learning material 產生 durable project rule 時, 把 stable conclusion 重寫到對應的 `docs/foundations/`, `docs/architecture/`, `docs/verification/`, `docs/roadmap/`, 或 `docs/adr/`. 原始推導可以繼續留在 `learning/`.

---

## 5. Commit Message Convention

採用簡化版 Conventional Commits:

```text
type(scope): summary
```

常用 type:

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

常用 scope:

```text
project
docs
notes
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
docs(project): clarify development workflow
docs(notes): add project journal index
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

Branch commits can be exploratory, but commits merged into `main` should be readable logical steps.

---

## 6. Experiments

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

## 7. Definition of Done

Every branch should have a small done condition before it grows too large.

Examples:

```text
docs/project-policy done when:
  README / AGENTS / DEVELOPMENT agree on the same workflow
  stale references are removed
  learning/ and docs/ authority boundaries are documented

render/raster-baseline done when:
  ScreenVertex exists
  edge-function coverage exists
  pixel-center sampling is used
  half-open bbox is used
  depth buffer and depth test exist
  color / depth interpolation exists
  relevant raster tests or deterministic demos exist

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

## 8. Docs and Code Sync

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

Learning-heavy material should go to `learning/`. Stable docs should be concise enough to guide source work, not preserve the full learning path.

---

## 9. Merge Policy

Small docs or maintenance changes may be merged directly after clean commits.

Larger features should use a branch:

```bash
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

Default policy for this personal project:

```text
Keep main linear when the branch can be cleanly replayed.
Use rebase + fast-forward merge as the normal path.
Delete short-lived local branches after they are merged.
```

Default merge path:

```bash
git switch main
git pull --ff-only

git switch render/raster-baseline
git rebase main

git switch main
git merge --ff-only render/raster-baseline
git branch -d render/raster-baseline
```

Use a merge commit only when the branch grouping itself is intentionally useful:

```text
large milestone branch
multi-person review
remote PR with discussion history
branch with many commits that should remain visibly grouped
```

Explicit merge-commit path:

```bash
git switch main
git merge --no-ff render/raster-baseline
```

This keeps the feature as a readable group in `git log --graph`, but it should be a deliberate choice, not the default.

---

## 10. Generated Files

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

## 11. Current Near-Term Flow

Current repository shape:

```text
main
  source + build/test + stable project docs
  learning history + tutorial tracks + rough reasoning under learning/
```

Recommended near-term source branch:

```text
render/raster-baseline
```

Recommended implementation style:

```text
1. small testable raster helpers
2. edge-function coverage
3. pixel-center sampling
4. top-left shared-edge rule
5. color interpolation
6. depth interpolation
7. depth buffer and depth test
8. simple debug views or deterministic demos
```

If raster work exposes ownership pressure, split a focused architecture branch:

```text
arch/render-target
```

This sequence may change, but changes should preserve the same principle:

```text
correctness -> observability -> clear architecture -> performance
```
