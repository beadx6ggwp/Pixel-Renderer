# Development Workflow

Last updated: 2026-06-29

這份文件定義 Pixel-Renderer 的輕量開發規範. 目標不是建立複雜流程, 而是讓 `main` 的 history 可讀, branch 不互相污染, experiment 可以存在但不破壞主線.

---

## 1. Main Branch Semantics

`main` 代表整理過的專案主線:

```text
main = source + build/test + stable project docs
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
```

`main` 不應該長期包含:

```text
unfinished experiment
unclear WIP commit
large unrelated mixed change
temporary debug output
generated test diff
long-form learning notes
teaching drafts
raw brainstorming records
```

如果某次 commit 讓 `main` 暫時不能 build, commit message 或後續 commit 必須清楚說明原因.

完整 learning history 不放在 `main`, 而是保存在 `notes/journal`.

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
notes/      project journal branches, not merged wholesale into main
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
notes/journal
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

Remote branch 是暫時工作線, 不是永久歷史保存區. merge 後可以刪掉.

`notes/journal` 是例外. 它是長期 project journal branch, 可以長期存在, 但不整條 merge 回 `main`.

---

## 4. Project Journal Branch

`notes/journal` 用來保存:

```text
long-form reasoning
learning notes
teaching drafts
brainstorming
historical decision context
rough architecture comparisons
```

它不是 current source truth.

從 `main` 讀歷史脈絡時, 用:

```bash
git show notes/journal:docs/notes/JOURNAL_INDEX.md
```

當 journal 裡的內容成熟成 project rule, 只抽取 stable conclusion 回 `main`:

```bash
git switch main
git switch -c docs/<topic>

# rewrite or copy only the stable conclusion

git add <stable-docs>
git commit -m "docs(<scope>): <summary>"

git switch main
git merge --ff-only docs/<topic>
git branch -d docs/<topic>
```

不要把 rough note, tutorial draft, 或完整推導直接搬回 `main`, 除非它已經變成 project convention, architecture boundary, verification strategy, roadmap decision, or ADR.

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
  notes/journal lookup is documented if needed

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

Learning-heavy docs should go to `notes/journal` first. Stable docs on `main` should be concise enough to guide source work, not preserve the full learning path.

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

Current branch state:

```text
main
  source + build/test + stable project docs

notes/journal
  learning history + tutorial drafts + rough reasoning
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
