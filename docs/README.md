# Pixel-Renderer Docs

`docs/` 放 stable project docs, renderer conventions, architecture notes, verification policy, roadmap, and ADR. 這裡保存 current project truth. 完整學習歷程、tutorial、readings 與 exploratory notes 放在 repository root 的 `learning/`.

## Reading Entry

一般閱讀從這裡開始:

```text
PROJECT_MAP.md
ARCHITECTURE.md
DEVELOPMENT.md
foundations/rendering_conventions.md
foundations/rasterization_edge_rules.md
```

如果要接下一個 `render/raster-baseline` branch，再看：

```text
roadmap/next_steps.md
verification/testing_strategy.md
foundations/interpolation_contract.md
```

如果是跨裝置, 跨 session, 久未開專案, 或 context 已經 stale, 再讀:

```text
HANDOFF.md
```

如果要編輯 `docs/` 子樹, 先看:

```text
AGENTS.md
```

## Document Roles

```text
PROJECT_MAP.md
  project-level overview, knowledge map, and long-term direction

ARCHITECTURE.md
  short/mid/long technical target architecture

DEVELOPMENT.md
  git, branch, commit, experiment workflow

HANDOFF.md
  cross-session orientation only, not the general docs workflow

AGENTS.md
  docs subtree collaboration rules
```

## Directory Map

```text
docs/
  README.md                   this file
  AGENTS.md                   docs subtree collaboration rules
  HANDOFF.md                  cross-session handoff and stale-context entry
  PROJECT_MAP.md              project-level overview
  ARCHITECTURE.md             target architecture and technical direction
  DEVELOPMENT.md              development workflow

  foundations/                coordinate, depth, raster, math, pipeline conventions
  architecture/               renderer core, display backend, command queue, UI
  verification/               tests, debug views, traces, golden images
  mapping/                    commercial engine mapping and career mapping
  roadmap/                    milestone planning and execution order
  adr/                        architecture decision records
```

## Learning Workspace

Read [`../learning/README.md`](../learning/README.md) for learning notes, tutorial tracks, readings, and small labs. Learning material lives on `main`, but it is not automatically current source truth. When a note becomes a durable project rule, extract only the stable part into `foundations/`, `architecture/`, `verification/`, `roadmap/`, or `adr/`.
