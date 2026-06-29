# Git 情境教學: merge main 到 notes/journal 時保留 journal-only 檔案

## Question

為什麼跑完下面這組操作後, `docs/notes/`, `docs/tutorial-cpp/`, `docs/tutorial-soft-renderer/` 下面一堆原本已經存在的檔案, 會突然變成 `?? untracked`?

```bash
git switch notes/journal
git merge main --no-commit
git restore --source=HEAD -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
git commit -m "docs(notes): sync journal with main cleanup"
```

直覺上會覺得怪:

```text
這些檔案本來就在 notes/journal 上,
為什麼 Git 好像又要我重新 commit 一次?
```

## Context

這次 repo 正在把文件責任拆開:

```text
main
  source
  build and test files
  stable project docs

notes/journal
  long-form notes
  tutorial drafts
  historical reasoning
```

所以 `main` 清掉這些 journal-only path 是合理的:

```text
docs/notes/
docs/tutorial-cpp/
docs/tutorial-soft-renderer/
```

但 `notes/journal` 應該保留它們. 這次真正想做的事情是:

```text
merge main into notes/journal
  接收 main 上新的 source / stable docs 狀態
  但保留 notes/journal 專屬的 notes / tutorial drafts
```

## First-Principles Model

Git 這裡至少要分清楚三層狀態:

```text
HEAD commit
  目前 branch 最後一個 commit 的檔案快照

index / staging area
  下一個 commit 會寫進去的檔案快照

working tree
  硬碟上目前實際看到的檔案
```

最重要的規則是:

```text
git commit 記錄的是 index, 不是直接記錄 working tree.
```

所以一個檔案可以「硬碟上存在」, 但「下一個 commit 不會包含它」. 只要 index 裡面記錄的是 deleted, commit 出來就是 deleted.

## ASCII Mental Model

先記住這條資料流:

```text
working tree --git add / git restore --staged--> index --git commit--> HEAD

硬碟上的檔案                                  下一個 commit 會寫什麼      commit 結果
```

錯誤直覺通常是:

```text
working tree 有檔案
  -> commit 就會有檔案
```

但 Git 的真實規則是:

```text
index 有檔案
  -> commit 才會有檔案
```

這次的 branch graph 大概是:

```text
before sync

                 main
                  |
                  B  docs(development): clarify journal update flow
                  |
                  A  docs(project): keep main focused on stable docs
                 /
base -----------o
                 \
                  J  notes/journal: add project journal index
                  |
                 notes/journal
```

`main` 的方向是清掉 journal-only docs, `notes/journal` 的方向是保留 journal-only docs. 所以 merge 時真正要做的是:

```text
after intended merge

                 main
                  |
                  B
                  |
                  A
                 / \
base -----------o   M  notes/journal: sync journal with main cleanup
                 \ /
                  J
                  |
             origin/notes/journal

M should contain:
  main 的 stable docs / source changes
  notes/journal 的 journal-only files
```

下面用一個檔案代表整批 journal-only files:

```text
P = docs/notes/a.md

E = exists
D = deleted / absent
?? = exists in working tree, but not tracked by current HEAD
```

### Step 0: merge 之前

```text
notes/journal HEAD still owns P

+------------------+     +------------------+     +------------------+
| HEAD             |     | index            |     | working tree     |
+------------------+     +------------------+     +------------------+
| P: E             |     | P: E             |     | P: E             |
+------------------+     +------------------+     +------------------+
```

### Step 1: `git merge main --no-commit`

```text
main says: P should be deleted

git merge main --no-commit
  -> compute merge result
  -> put merge result into index
  -> update working tree
  -> stop before commit

+------------------+     +------------------+     +------------------+
| HEAD             |     | index            |     | working tree     |
+------------------+     +------------------+     +------------------+
| P: E             |     | P: D             |     | P: D             |
+------------------+     +------------------+     +------------------+
                         ^
                         next commit will read this
```

這一步還沒錯. `--no-commit` 給你一個機會檢查和修正 merge result.

### Step 2: wrong restore

```bash
git restore --source=HEAD -- docs/notes
```

沒有 `--staged` 時, 這只改 working tree:

```text
+------------------+     +------------------+     +------------------+
| HEAD             |     | index            |     | working tree     |
+------------------+     +------------------+     +------------------+
| P: E             |     | P: D             |     | P: E             |
+------------------+     +------------------+     +------------------+
                         ^
                         still deleted in next commit
```

這是最危險的狀態:

```text
你在檔案總管或 editor 裡看得到 P,
但 `git commit` 還是會記錄 P deleted.
```

### Step 3: commit after wrong restore

```bash
git commit -m "docs(notes): sync journal with main cleanup"
```

`git commit` 讀取 index, 所以新的 `HEAD` 會變成:

```text
+------------------+                              +------------------+
| new HEAD         |                              | working tree     |
+------------------+                              +------------------+
| P: D             |                              | P: E             |
+------------------+                              +------------------+
                                                    ^
                                                    file exists on disk
                                                    but new HEAD does not track it
```

因此 `git status` 顯示:

```text
?? docs/notes/a.md
```

這不是 Git 突然忘記歷史, 而是剛剛那個 commit 的 snapshot 真的沒有包含 `P`.

### Correct State

正確 command 應該同時 restore `index` 和 `working tree`:

```bash
git restore --source=HEAD --staged --worktree -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
```

狀態會是:

```text
+------------------+     +------------------+     +------------------+
| HEAD             |     | index            |     | working tree     |
+------------------+     +------------------+     +------------------+
| P: E             |     | P: E             |     | P: E             |
+------------------+     +------------------+     +------------------+
                         ^
                         next commit will preserve P
```

再 commit 才會得到正確的 merge commit:

```text
+------------------+     +------------------+
| new HEAD         |     | working tree     |
+------------------+     +------------------+
| P: E             |     | P: E             |
+------------------+     +------------------+
```

## What Happened

在 merge 之前, `notes/journal` 上有這些 journal-only 檔案:

```text
HEAD
  docs/notes/a.md                         exists
  docs/tutorial-cpp/index.html            exists
  docs/tutorial-soft-renderer/index.html  exists

index
  same as HEAD

working tree
  same as HEAD
```

接著執行:

```bash
git merge main --no-commit
```

`--no-commit` 的意思不是「什麼都不改」. 它的意思是:

```text
先算出 merge 結果,
把 merge 結果放進 index 和 working tree,
但先不要真的 commit.
```

因為 `main` 已經把 journal-only path 刪掉, 所以 Git 算出的 merge result 也準備刪掉它們:

```text
after git merge main --no-commit

HEAD
  docs/notes/a.md                         exists

index
  docs/notes/a.md                         deleted

working tree
  docs/notes/a.md                         deleted
```

這時候我們的意圖是: 其他 main changes 可以收下, 但這些 journal-only path 要從目前 `notes/journal` 的 `HEAD` 拿回來.

當時用的是:

```bash
git restore --source=HEAD -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
```

問題就在這裡. 這個 command 預設只 restore working tree, 不 restore index.

所以狀態變成:

```text
after git restore --source=HEAD -- docs/...

HEAD
  docs/notes/a.md                         exists

index
  docs/notes/a.md                         deleted

working tree
  docs/notes/a.md                         exists
```

這是一個很容易誤判的狀態:

```text
硬碟上看得到檔案,
但下一個 commit 仍然準備刪掉它.
```

接著執行:

```bash
git commit -m "docs(notes): sync journal with main cleanup"
```

因為 `git commit` 記錄的是 index, 所以這個 merge commit 其實記錄了:

```text
new HEAD merge commit
  docs/notes/a.md                         deleted
  docs/tutorial-cpp/index.html            deleted
  docs/tutorial-soft-renderer/index.html  deleted
```

但 working tree 上那些檔案仍然存在, 因為剛剛 `git restore` 已經把它們放回硬碟.

commit 完之後, Git 看到的是:

```text
HEAD
  docs/notes/a.md                         absent

working tree
  docs/notes/a.md                         exists
```

所以 `git status` 顯示:

```text
?? docs/notes/a.md
```

`??` 的意思不是「Git 忘記了歷史」, 而是:

```text
這個檔案現在存在於 working tree,
但目前 HEAD 沒有追蹤它.
```

## Correct Command

在 `git merge --no-commit` 期間, 如果要把某些 path 從 merge 前的 current branch 保留下來, 要同時 restore `index` 和 `working tree`:

```bash
git restore --source=HEAD --staged --worktree -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
```

這樣狀態才會變成:

```text
HEAD
  docs/notes/a.md                         exists

index
  docs/notes/a.md                         exists

working tree
  docs/notes/a.md                         exists
```

此時再 commit, merge commit 才會真的保留這些檔案.

## If The Wrong Commit Already Happened

如果錯誤的 merge commit 還沒有 push, 最乾淨的修法是 amend 剛剛那個 commit:

```bash
git add docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
git commit --amend --no-edit
```

這不是新增一個「刪掉又加回來」的 commit, 而是修正剛剛那個 merge commit 的內容:

```text
before amend
  merge main, but accidentally deletes journal-only docs

after amend
  merge main, and correctly keeps journal-only docs
```

如果那個 merge commit 已經 push 或已經被別人拿去用了, 就不要 rewrite shared history. 改用正常補救 commit:

```bash
git add docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
git commit -m "docs(notes): restore journal-only docs"
```

## Diagnostic Commands

遇到複雜 merge, commit 前要先看「下一個 commit 到底會寫什麼」:

```bash
git status --short --branch
git diff --cached --name-status -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
git diff --name-status -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
```

這兩個 diff 的意思不同:

```text
git diff --cached
  index vs HEAD
  下一個 commit 會記錄什麼

git diff
  working tree vs index
  硬碟上有哪些還沒有 staged 的變化
```

也可以確認目前 commit 是否追蹤某些 path:

```bash
git ls-tree -r --name-only HEAD -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
```

確認 index 是否追蹤某些 path:

```bash
git ls-files -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
```

## Practical Rule

在 `git merge --no-commit` 裡保留 branch-specific files 時, 不要只確認檔案有沒有出現在硬碟上. 要確認 index 是否也代表你想 commit 的結果.

比較穩的流程:

```bash
git merge main --no-commit
git restore --source=HEAD --staged --worktree -- <paths-to-keep-from-current-branch>
git status --short
git diff --cached --name-status
git commit -m "<message>"
```

套到這次 repo, 第一次 reconciliation 應該是:

```bash
git switch notes/journal
git merge main --no-commit
git restore --source=HEAD --staged --worktree -- docs/notes docs/tutorial-cpp docs/tutorial-soft-renderer
git status --short
git commit -m "docs(notes): sync journal with main cleanup"
```

這次真正壞掉的不是 branch 策略, 而是把:

```text
files restored in working tree
```

誤當成:

```text
files restored in the next commit
```
