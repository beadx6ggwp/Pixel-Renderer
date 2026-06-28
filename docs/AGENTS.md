# docs/AGENTS.md

這份文件定義 `docs/` 子樹的文件協作規則. 它補充 root `AGENTS.md`, 不取代 root 對專案方向, source work, git workflow, renderer priority 的指引.

一般 docs 編輯不必先讀 `docs/HANDOFF.md`. 只有 fresh cross-machine session, 久未開專案, 上下文不確定, 或使用者要求完整現況交接時, 才把 `docs/HANDOFF.md` 當作起點.

## 文件角色

`docs/HANDOFF.md` 是跨裝置 / 跨 session 的交接入口. 保持精簡, 只記 future session 無法從 git history 或 stable docs 快速推回來的 durable context.

`docs/notes/` 用來保存 rough ideas, learning traces, 長篇討論紀錄, 當下推理, 方案比較, 現況整理. 它可以是 exploratory, 但仍要能搜尋, 能被未來整理重用.

`docs/tutorial-soft-renderer/` 和 `docs/tutorial-cpp/` 是 teaching tracks. 它們可以很完整, 很教學, 但不要把 tutorial chapter 直接視為 immediate source roadmap.

`docs/adr/` 只放會約束未來 branch 的 architecture decisions, 例如 owned `Framebuffer`, `DisplayBackend` boundary, depth convention, screen-space triangle before MVP, `CommandQueue` sequencing.

其他 stable docs 依主題使用, 例如 `foundations/`, `architecture/`, `verification/`, `mapping/`, `roadmap/`. 這些資料夾是可用出口, 不是唯一出口.

## Notes 工作流

新的 rough idea, 學習痕跡, 長篇討論紀錄, 先放在:

```text
docs/notes/
```

保留目前 Pixel-Renderer 的命名格式:

```text
docs/notes/YYYY-MM-DD-short-title.md
```

範例:

```text
docs/notes/2026-06-28-current_status_and_decision_map.md
docs/notes/2026-05-22-pixel_renderer_next_trusted_pipeline_plan.md
```

不要導入 `MMDD_HHMM_short-title.md`, 也不要強制建立 sidecar log workflow. 這個 repo 已經有自己的歷史格式, 保持一致比移植另一個專案的格式更重要.

當 notes 累積到可重用, 可教學, 或可反覆引用時, 可以考慮整理成 topic article 或 topic folder. 這個 topic 不一定要進 `foundations/`, `architecture/`, `verification/`, `roadmap/` 或 `adr/`; 先看內容真正回答的是什麼問題.

常見整理方向:

```text
docs/notes/<topic>/
docs/<existing-topic-folder>/
docs/tutorial-*/
docs/foundations/
docs/architecture/
docs/verification/
docs/roadmap/
docs/adr/
```

只有當內容變成 project-wide convention, architecture boundary, verification strategy, roadmap decision, 或 durable ADR 時, 才放進對應 stable docs. 不要只是因為一篇 note 看起來完整, 就急著移出 `docs/notes/`.

## Note 結構

一般 note 可以依需要使用這些 section:

```markdown
# Title

## Question
## Context
## Reasoning
## Options
## Decision
## Follow-ups
```

不是每篇都需要完整模板. 短 note 可以短, 長 note 要讓未來讀者知道:

```text
這篇在問什麼
當時有哪些 constraints
naive idea 為什麼不夠
目前 conclusion 是什麼
後續要去哪裡找更穩定的版本
```

如果是在比較下一步, 給 2-4 個 viable choices, 說明每個選項適合什麼情況, tradeoffs, recommended default. 不要把單一路線寫成唯一正解, 除非其他路線已經被 source facts 或 durable decisions 排除.

## Stable Docs 規則

stable docs 應該記錄長期可依賴的規則, 架構邊界, 命名, 測試策略, roadmap decision. 它們不是聊天紀錄, 也不是把所有 notes 壓縮成總結.

更新 stable docs 前, 先確認:

```text
這是 durable decision 還是 temporary thought?
這會不會約束未來 branch?
它應該成為 project convention 嗎?
有沒有 current source 或 git state 需要驗證?
```

如果只是探索, 留在 `docs/notes/`. 如果是主題成熟後的教學整理, 可以放 topic article 或 topic folder. 只有真正變成 project-wide rule 時, 才放到 stable docs 或 ADR.

## 寫作風格

使用繁體中文. 中文 prose 使用 ASCII half-width punctuation. 保留 English technical terms, 例如 class names, API, algorithm, compiler flags, file paths, branch names.

偏好的說明順序:

```text
concrete case
  -> naive model
  -> failure point
  -> deeper mechanism
  -> math / geometry
  -> code / engineering consequence
```

寫建議時要具體, 先講 constraint, 再講 naive 方法為什麼會失敗, 再講更好的方向與 tradeoff.

避免空泛分類與資料夾:

```text
organized/
misc/
guides/
notes2/
new/
```

如果需要新 topic folder, 名稱要能回答 "這組文章解決什麼問題".

## Source 與 Docs 的關係

不要只憑舊 note 斷言 current source state. 涉及目前 code, build, branch, 檔案存在與否時, 先查 current source 或 git state.

如果舊 note, tutorial, stable docs, current source 互相衝突, 優先序是:

```text
current source / git state
root AGENTS.md and docs/AGENTS.md
stable docs
recent handoff when this is actually a handoff context
old notes
tutorial chapters
chat memory
```

`docs/HANDOFF.md` 不應變成巨大 roadmap. 若一個 durable direction 需要長篇說明, 把詳細內容放到相應 docs 或 notes, handoff 只保留入口與摘要.
