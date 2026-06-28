# Pixel-Renderer Docs

`docs/` 放 project docs, stable conventions, rough notes, and tutorial tracks. 這裡不是單一 roadmap, 而是把不同層級的文件分開保存.

## Reading Entry

一般閱讀從這裡開始:

```text
PROJECT_MAP.md
ARCHITECTURE.md
DEVELOPMENT.md
foundations/rendering_conventions.md
foundations/rasterization_edge_rules.md
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

  tutorial-soft-renderer/     graphics and rasterization teaching track
  tutorial-cpp/               C++ and engineering teaching track
  notes/                      rough ideas, learning traces, current-state analysis
```

## Notes

Use `docs/notes/` for rough ideas, learning traces, long-form discussion records, and current-state analysis.

Current useful notes:

```text
notes/2026-06-28-current_status_and_decision_map.md
notes/2026-05-22-pixel_renderer_next_trusted_pipeline_plan.md
notes/2026-05-22-rendering_systems_learning_record_and_plan.md
```

When a note becomes a durable project rule, extract only the stable part into `foundations/`, `architecture/`, `verification/`, `roadmap/`, or `adr/`.
