# AGENTS.md

This file gives repo-local instructions for Codex or other coding agents working on Pixel-Renderer.

Last updated: 2026-06-29

## User Context

The user is a Taiwan CS graduate preparing for graduate school, with a long-term interest in:

- Computer graphics and rendering systems
- GPU architecture and low-level C++ systems programming
- Software rasterization from first principles
- Eventually using the software renderer as a golden model for FPGA / GPU pipeline experiments
- Immediate-mode UI and renderer/tooling architecture
- Commercial engine fluency in Unity / Unreal / Godot / Filament-like systems

Use Traditional Chinese for explanations. Keep technical terms such as class names, APIs, algorithms, compiler flags, and architecture terms in English.

The user prefers first-principles reasoning:

```text
concrete case
  -> naive model
  -> failure point
  -> deeper mechanism
  -> math / geometry
  -> code / engineering consequence
```

Avoid generic advice. When making a recommendation, explain the reasoning and the tradeoff.

## Project Identity

Pixel-Renderer should be treated as a `Rendering Systems Lab`, not only a toy software rasterizer.

The long-term learning strategy is:

```text
Pixel-Renderer
  -> first-principles implementation track

Engine Mirror Demo
  -> reproduce or inspect the same concept in Unity / Unreal / Godot / Filament-like systems
```

Every major concept should eventually be connectable across:

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

Do not force the project too early into only one direction such as FPGA, SwiftShader, Filament clone, game engine, UI framework, Unity client, Unreal client, or technical art. Preserve the branching map while building the shared core.

## First Files To Read

For ordinary source or docs work, read these first:

```text
docs/PROJECT_MAP.md
docs/ARCHITECTURE.md
docs/DEVELOPMENT.md
docs/README.md
docs/foundations/rendering_conventions.md
docs/foundations/rasterization_edge_rules.md
```

Read `docs/HANDOFF.md` only when this is a fresh cross-machine / cross-session handoff, the project has not been opened for a while, the local context looks stale or conflicting, or the user explicitly asks for current-state orientation.

When editing files under `docs/`, also follow:

```text
docs/AGENTS.md
```

Then inspect the current source and git state:

```bash
git status --short
git branch --show-current
rg --files src docs | head
```

Important docs areas:

```text
docs/AGENTS.md                 documentation workflow rules for docs/
docs/foundations/              stable conventions and math mapping
docs/architecture/             renderer/backend/UI architecture
docs/verification/             tests, traces, debug views, golden images
docs/mapping/                  engine mirror and career mapping
docs/roadmap/                  milestone planning
docs/adr/                      architecture decision records
notes/journal branch           learning history, thinking records, tutorial drafts
```

## Current Source State

As of 2026-05-22, the source is still an early Win32 software renderer prototype. Verify current code before relying on this snapshot.

Current architecture:

```text
Application
  owns ScreenManager
  owns RenderDevice
  owns Rasterizer
  controls frame loop

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
  provides DrawLine()
  provides DrawTriangle()
```

Current code likely has:

```text
Win32 window + DIB framebuffer
Application frame loop
CPU-side Clear / SetPixel
Bresenham DrawLine
Bounding-box triangle fill
Barycentric inside test
```

Current missing pieces likely include:

```text
owned Framebuffer abstraction
depth buffer
edge-function rasterizer
color / depth interpolation
unit tests
debug views
Mat4 / MVP / viewport transform
IShader
Material / Renderer / Scene / View / Camera
CommandQueue
```

## Near-Term Technical Priority

Do not jump directly to a full mini-Filament skeleton, Material system, CommandQueue, SwiftShader-like architecture, FPGA work, OBJ/texture/Phong/shadow, or full UI framework.

First build a trusted raster pipeline:

```text
1. DDA reference and Bresenham line rasterization
2. screen-space triangle baseline
3. bounding box, edge function, barycentric coordinates
4. color interpolation and depth interpolation
5. depth buffer and depth test
6. small tests / demo cases as soon as possible
7. debug views and pipeline trace
```

Then move toward:

```text
NDC / viewport
Vec4 / Mat4 / LookAt / Orthographic
Perspective
IShader / VertexInput / VertexOutput / FragmentInput / ShaderContext
Material / MaterialInstance
Renderer / Scene / View / Camera
CommandQueue / Debug UI
Engine Mirror Demo
```

Testing should not wait until the end. Add small tests or deterministic demo cases during line, triangle, barycentric, and depth work.

## Display Backend Priority

SDL / Win32 / display abstraction is an important architecture topic and should not be buried.

The key separation:

```text
renderer core
  calculates pixels

Framebuffer
  owns pixel memory

RenderDevice
  writes into Framebuffer

DisplayBackend
  presents Framebuffer to the OS/window system
```

`DisplayBackend` should present a framebuffer; it should not own the renderer target lifetime.

Future architecture docs should cover:

```text
Win32DisplayBackend
SDLDisplayBackend
macOSDisplayBackend
HeadlessTestBackend
SDL_UpdateTexture vs SDL_LockTexture
input/event boundary
platform header isolation
```

## Documentation Workflow

When editing docs, follow `docs/AGENTS.md`. It defines the local documentation workflow for the `docs/` subtree.

Prefer stable docs for decisions and long-lived concepts:

```text
docs/foundations/rendering_conventions.md
docs/architecture/display_backend_architecture.md
docs/foundations/pipeline_flow.md
docs/foundations/math_to_renderer.md
docs/verification/testing_strategy.md
docs/verification/debug_visualization.md
```

Use branch `notes/journal` for dated long-form thinking, conversation records, learning notes, and tutorial drafts. Do not merge that branch wholesale into `main`; extract only stable project conclusions back through focused `docs/*`, `render/*`, `arch/*`, or `test/*` branches.

Use `docs/adr/` only for decisions that will affect future branches, such as:

```text
owned Framebuffer
DisplayBackend boundary
depth convention
screen-space triangle before MVP
Material vs MaterialInstance
CommandQueue after Renderer skeleton
```

## Git Workflow

Follow `docs/DEVELOPMENT.md`.

Main branch semantics:

```text
main = readable, explainable, preferably buildable history
```

Use engineering branch names, not tool names.

Recommended prefixes:

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

Examples:

```text
docs/project-map
docs/foundations
arch/framebuffer-display
render/raster-baseline
test/render-core
debug/pipeline-trace
exp/gjk-2d
```

Use local branches by default. Push remote branches only for long-running, high-risk, cross-machine, backup, CI, or PR/discussion work.

Do not keep remote branches as permanent history. The readable project history should live on `main`, docs, tags, and ADRs.

Important git operations are user-owned by default. Agents should not run these unless the user explicitly asks the agent to operate git directly:

```text
git switch -c
git branch -d
git add
git commit
git merge
git rebase
git push
git reset
```

For these operations, prefer giving the user:

```text
recommended branch name
files to stage
commit message
merge / cleanup sequence
pre-merge checks
```

Agents may still inspect repository state with read-only commands such as `git status`, `git log`, `git diff`, `git branch`, and `git show`.

## Commit Messages

Use simplified Conventional Commits:

```text
type(scope): summary
```

Common types:

```text
docs
feat
fix
refactor
test
debug
perf
build
chore
exp
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

Branch commits can be exploratory, but commits merged into `main` should be readable logical steps.

## Experiment Rules

Experiments are allowed and useful.

Use `exp/*` for work like:

```text
DDA vs Bresenham comparison
triangle edge-rule exploration
GJK collision prototype
temporary math experiments
```

Rules:

```text
1. exp/* may contain messy commits.
2. exp/* may contain competing implementations.
3. exp/* should not merge directly into main.
4. Extract useful results into docs, tests, or a clean render/arch/test branch.
```

For example, DDA may become a reference implementation or teaching note, while Bresenham becomes the production `Rasterizer::DrawLine` path.

## Code Editing Guidance

Before editing:

```text
git status --short
rg --files src docs
```

Do not revert user changes unless explicitly asked.

Keep implementation scoped. Avoid large unrelated refactors.

For renderer work, prefer this loop:

```text
1. write a naive/reference version
2. identify failure cases
3. implement the selected production path
4. add small tests or deterministic examples
5. document the convention or tradeoff
6. commit a logical step
```

For Windows development, respect the existing Win32 path until the backend abstraction is intentionally changed. Do not replace the display path with SDL just because SDL exists; first define the ownership and backend boundary.

## Build And Generated Files

Current project historically uses a Makefile for Windows / MinGW style builds. Verify the current build commands in `README.md`, `makefile`, and docs before changing them.

Generated outputs should not be committed unless they are intentional fixtures.

Likely ignored or generated:

```text
build/
*.exe
*.o
test-output/
golden-diff/
trace-output/
```

Golden images may be committed only when they are stable reference fixtures. Failed diffs and temporary traces should stay ignored.

## Communication Style

When explaining to the user:

```text
use Traditional Chinese
keep technical names in English
be concrete and actionable
explain tradeoffs from first principles
avoid generic motivational advice
```

For conceptual topics, prefer:

```text
concrete case -> naive model -> failure -> deeper mechanism -> math -> code
```

For implementation/debugging tasks, be concise first, then add deeper explanation only if useful.
