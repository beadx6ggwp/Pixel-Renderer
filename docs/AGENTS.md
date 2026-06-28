# docs/AGENTS.md

這份文件定義 `docs/` 子樹的文件協作規則. 它補充 root `AGENTS.md`, 不取代 root 對專案方向, source work, git workflow, renderer priority 的指引.

一般 docs 編輯不必先讀 `docs/HANDOFF.md`. 只有 fresh cross-machine session, 久未開專案, 上下文不確定, 或使用者要求完整現況交接時, 才把 `docs/HANDOFF.md` 當作起點.

## 文件角色

`docs/HANDOFF.md` 是跨裝置 / 跨 session 的交接入口. 保持精簡, 只記 future session 無法從 git history 或 stable docs 快速推回來的 durable context.

Long-form learning notes, historical reasoning, tutorial drafts, and exploratory records are kept on branch `notes/journal`, not on `main`.

Historical teaching tracks such as `docs/tutorial-soft-renderer/` and `docs/tutorial-cpp/` live on `notes/journal`. They are useful learning context, but they are not current source roadmap.

`docs/adr/` 只放會約束未來 branch 的 architecture decisions, 例如 owned `Framebuffer`, `DisplayBackend` boundary, depth convention, screen-space triangle before MVP, `CommandQueue` sequencing.

其他 stable docs 依主題使用, 例如 `foundations/`, `architecture/`, `verification/`, `mapping/`, `roadmap/`. 這些資料夾是可用出口, 不是唯一出口.

## Project Journal 工作流

新的 rough idea, learning trace, 長篇推導, brainstorming, 或 teaching draft, 先放在 branch:

```text
notes/journal
```

`main` 不保存完整學習歷程. `main` 只保存 source, build/test, stable project docs, renderer conventions, architecture boundaries, verification strategy, roadmap, and ADR.

從 `main` 讀歷史脈絡時, 不需要切 branch. 使用:

```bash
git show notes/journal:docs/notes/JOURNAL_INDEX.md
```

Do not treat `notes/journal` as current source truth.

如果 journal note 變成 durable project rule, 只抽 stable conclusion 回 `main`:

```bash
git switch main
git switch -c docs/<topic>

# rewrite or copy only the stable conclusion

git add <stable-docs>
git commit -m "docs(<scope>): <summary>"
```

只有當內容變成 project-wide convention, architecture boundary, verification strategy, roadmap decision, 或 durable ADR 時, 才放進對應 stable docs. 不要只是因為一篇 journal note 看起來完整, 就急著搬回 `main`.

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

如果只是探索, 留在 `notes/journal`. 只有真正變成 project-wide rule 時, 才放到 stable docs 或 ADR.

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
notes/journal historical records
chat memory
```

`docs/HANDOFF.md` 不應變成巨大 roadmap. 若一個 durable direction 需要長篇說明, 把詳細內容放到相應 docs 或 notes, handoff 只保留入口與摘要.
