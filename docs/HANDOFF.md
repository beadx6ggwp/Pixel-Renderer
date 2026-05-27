# Pixel-Renderer Handoff

Last updated: 2026-05-27

This file is the cross-machine / cross-session handoff for continuing Pixel-Renderer development on Windows or in a fresh Codex session.

Read this first, then read:

```text
AGENTS.md
docs/PROJECT_MAP.md
docs/ARCHITECTURE.md
docs/DEVELOPMENT.md
docs/foundations/rendering_conventions.md
docs/foundations/rasterization_edge_rules.md
```

---

## 1. Current Git State

As of this handoff:

```text
main == origin/main
latest pushed commit: 6447230 docs(raster): define edge coverage rules
remote on this Mac: git@github.com:beadx6ggwp/Pixel-Renderer.git
```

The Mac session configured GitHub SSH over port 443:

```text
Host github.com
  HostName ssh.github.com
  User git
  Port 443
```

On Windows, verify your own GitHub authentication separately. GitHub Desktop may already provide HTTPS credentials there. If the Windows clone uses HTTPS and works, it can stay HTTPS. If you want SSH there too, configure a Windows SSH key separately and add the public key to GitHub.

Startup check on Windows:

```bash
git status --short --branch
git pull --ff-only
git log --oneline --decorate -5
```

Expected before new work:

```text
main is clean
main is up to date with origin/main
```

---

## 2. Project Framing

Treat Pixel-Renderer as a `Rendering Systems Lab`, not only a small software rasterizer.

The long-term strategy is:

```text
Pixel-Renderer
  implement rendering concepts from first principles

Engine Mirror Demo
  reproduce or inspect the same concept in Unity / Unreal / Godot / Filament-like systems
```

For major topics, preserve this mapping:

```text
low-level mechanism
math / geometry
C++ implementation boundary
debug / testing method
commercial engine abstraction
workflow / tooling
performance tradeoff
related job role
```

Do not prematurely force the project into only one end-state such as FPGA, SwiftShader, Filament clone, Unity/Unreal client, technical art, or self-built engine. Build the shared rendering core first, then branch through small experiments.

---

## 3. Current Source Snapshot

Verify before implementation, but the current code is still an early Win32 software renderer prototype:

```text
Application
  owns ScreenManager
  owns RenderDevice
  owns Rasterizer

ScreenManager
  owns Win32 window resources
  owns DIBSection framebuffer memory
  handles input events
  presents by BitBlt

RenderDevice
  borrows framebuffer pointer
  provides Clear()
  provides SetPixel()

Rasterizer
  borrows RenderDevice
  provides Bresenham DrawLine()
  provides bounding-box / barycentric DrawTriangle()
```

Important missing pieces:

```text
owned Framebuffer abstraction
depth buffer
edge-function triangle rasterizer
color / depth interpolation
tests
debug views
Mat4 / MVP / viewport transform
IShader
Material / Renderer / Scene / View / Camera
CommandQueue
DisplayBackend abstraction
```

---

## 4. Docs Added In The Planning Session

Important new docs:

```text
docs/PROJECT_MAP.md
  project-level map for Rendering Systems, Engine Mirror Demo, roadmap, branch directions

docs/ARCHITECTURE.md
  short / mid / long technical target architecture

docs/DEVELOPMENT.md
  git workflow, branch naming, commit style, experiment policy

docs/foundations/rendering_conventions.md
  coordinate, NDC, Vulkan-inspired conventions, depth, screen space, pixel sampling

docs/foundations/rasterization_edge_rules.md
  pixel center, edge function, top-left rule, bbox, degenerate triangle, tests

docs/notes/2026-05-22-*.md
  long-form planning notes and workflow notes
```

Docs directories now have stable roles:

```text
docs/foundations/    stable conventions and concept contracts
docs/architecture/   renderer core, framebuffer/display/backend, command queue, UI
docs/verification/   tests, traces, debug views, golden images
docs/mapping/        engine mirror and career mapping
docs/roadmap/        milestone planning
docs/adr/            durable architecture decision records
```

---

## 5. Technical Decisions To Preserve

### Vulkan-Inspired, Learning-First

Use Vulkan as a precise reference for post-projection conventions:

```text
clip / NDC depth range
viewport transform
framebuffer coordinates
explicit render state vocabulary
```

Do not copy Vulkan API architecture too early:

```text
no premature Vk-like object graph
no CommandQueue before raster core is trustworthy
no pipeline state explosion before there are real states to control
```

Current convention:

```text
NDC x: [-1, 1]
NDC y: [-1, 1]
NDC z: [0, 1]
screen origin: top-left
screen y: down
depth: [0, 1], smaller is closer
clear depth: 1.0
```

Important clarification:

```text
Traditional OpenGL-style teaching often uses NDC z [-1, 1].
Vulkan-style keeps x/y [-1, 1] but uses z [0, 1].
Do not remember this as "Vulkan has no [-1, 1] NDC square".
```

### Raster Baseline Rules

The next triangle implementation should use:

```text
pixel center sampling: (x + 0.5, y + 0.5)
half-open bbox ranges: [x0, x1), [y0, y1)
edge-function coverage
top-left rule for shared edges
degenerate triangle early return
float edge functions first for learning clarity
separate future integer / fixed-point experiment
```

### Git Workflow

Default workflow:

```text
branch -> clean commits -> rebase main -> merge --ff-only -> push main
```

Use engineering branch names:

```text
docs/
render/
arch/
test/
debug/
exp/
perf/
build/
```

Use `exp/*` as disposable experiments. Do not merge messy experiment branches directly into `main`; extract useful code, docs, or tests into a clean branch.

---

## 6. Next Recommended Work

Next branch:

```bash
git switch main
git pull --ff-only
git switch -c render/raster-baseline
```

Goal:

```text
Build the first trusted island:
screen-space triangle pipeline
```

Suggested implementation order:

```text
1. inspect current src/render/rasterizer.* and src/core/render_device.*
2. add or document DDA reference line path if useful
3. clean up Bresenham line behavior
4. introduce ScreenVertex for screen-space raster tests
5. implement DrawTriangleScreenSpace
6. switch triangle coverage to pixel-center edge functions
7. implement half-open bbox range
8. implement degenerate triangle early return
9. add barycentric weights from edge values
10. add depth buffer / depth test
11. add barycentric / depth / wireframe debug views or deterministic demos
12. add small tests as soon as the test harness exists
```

Commit shape:

```text
feat(render): add DDA line reference
feat(triangle): add screen-space triangle path
feat(triangle): use edge-function coverage
feat(depth): add depth buffer
debug(render): add barycentric debug view
test(raster): add triangle edge-rule cases
```

If no test harness exists yet, prefer deterministic examples or trace dumps rather than waiting until the end.

---

## 7. Do Not Do Yet

Avoid these until the raster baseline is trustworthy:

```text
full DisplayBackend refactor
SDL replacement
CommandQueue
Renderer / Scene / View / Camera skeleton
Material system
IShader full design
OBJ / texture / Phong / shadow mapping
ShaderVM
FPGA branch
full immediate-mode UI
```

Display/backend remains important, but should be its own `arch/*` branch after the raster core has a stable baseline. The intended boundary is:

```text
renderer core calculates pixels
Framebuffer owns pixel/depth memory
RenderDevice writes Framebuffer
DisplayBackend presents Framebuffer
```

---

## 8. Windows Session Checklist

When reopening on Windows:

```bash
git status --short --branch
git pull --ff-only
git log --oneline --decorate -5
```

Read:

```text
AGENTS.md
docs/HANDOFF.md
docs/foundations/rendering_conventions.md
docs/foundations/rasterization_edge_rules.md
src/render/rasterizer.cpp
src/core/render_device.cpp
```

Verify build commands from:

```text
README.md
makefile
docs/tutorial-cpp/impl/i06_make_workflow.html
```

Then start:

```bash
git switch -c render/raster-baseline
```

Before coding, inspect current implementation:

```bash
rg -n "DrawLine|DrawTriangle|SetPixel|Framebuffer|Vertex" src
```

Keep `main` clean and preferably buildable. Branches may contain WIP commits, but do not merge WIP commits into `main` without cleanup.

---

## 9. If Context Is Missing

If a future agent lacks the prior chat/session memory, use the repo docs as source of truth:

```text
PROJECT_MAP.md for project identity and roadmap
ARCHITECTURE.md for short/mid/long technical target
DEVELOPMENT.md for git workflow
rendering_conventions.md for coordinate/depth/screen rules
rasterization_edge_rules.md for triangle correctness
HANDOFF.md for current next step
```

If source code contradicts older notes, verify the code and update the docs. Older session notes are planning records, not immutable truth.
