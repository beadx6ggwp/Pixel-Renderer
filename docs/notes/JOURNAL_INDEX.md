# Pixel-Renderer Project Journal

這條 branch 保存 Pixel-Renderer 延伸出的 learning notes, thinking records, teaching drafts, and exploratory design notes.

Branch:

```text
notes/journal
```

## Purpose

`notes/journal` 是 project journal, 不是 current source truth.

它可以保存:

```text
long-form reasoning
learning notes
teaching drafts
brainstorming
historical decision context
rough architecture comparisons
```

`main` 只保留:

```text
source code
build and test files
README / AGENTS
stable project docs
architecture decisions
renderer conventions
verification strategy
```

## Rules

Do not merge this branch wholesale into `main`.

When a journal note becomes durable project knowledge, extract only the stable conclusion into a focused docs branch, then merge that docs branch into `main`.

Recommended extraction flow:

```bash
git switch main
git switch -c docs/<topic>

# copy or rewrite only the stable conclusion from notes/journal

git add <stable-docs>
git commit -m "docs(<scope>): <summary>"

git switch main
git merge --ff-only docs/<topic>
```

## Reading From Main

Future sessions should start from `main` and read:

```text
AGENTS.md
docs/HANDOFF.md
docs/README.md
docs/PROJECT_MAP.md
docs/ARCHITECTURE.md
docs/DEVELOPMENT.md
```

Only read this branch when historical reasoning is needed:

```bash
git show notes/journal:docs/notes/JOURNAL_INDEX.md
```

Do not treat `notes/journal` as current source state.

## Current Useful Notes

```text
docs/notes/2026-06-28-current_status_and_decision_map.md
docs/notes/2026-05-22-pixel_renderer_next_trusted_pipeline_plan.md
docs/notes/2026-05-22-rendering_systems_learning_record_and_plan.md
docs/notes/2026-05-22-pixel_renderer_architecture_and_learning_roadmap.md
docs/notes/2026-05-22-pixel_renderer_debug_testing_architecture.md
docs/notes/2026-05-22-pixel_renderer_hidden_spine.md
```

Tutorial tracks preserved here:

```text
docs/tutorial-soft-renderer/
docs/tutorial-cpp/
```
