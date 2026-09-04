# Git Rebase Branch Workflow Note

日期：2026-05-22

這份筆記記錄目前專案採用的 Git branch / rebase mental model。目標不是背指令，而是記住「誰搬到誰後面」。

---

## 1. Core Mental Model

`rebase` 不是 merge。

比較準確的說法是：

```text
把某條 branch 上的 commits 拿起來，
重新接到另一個 base 後面。
```

最重要的記法：

```text
git rebase <new-base> <branch-to-move>
```

白話：

```text
前面是新地基。
後面是要搬家的 branch。
```

所以：

```bash
git rebase A B
```

意思是：

```text
把 B 搬到 A 後面。
```

---

## 2. One-Argument Form

```bash
git rebase A
```

意思是：

```text
把目前所在 branch 搬到 A 後面。
```

例子：

```bash
git switch docs/project-architecture
git rebase main
```

意思是：

```text
把 docs/project-architecture 搬到 main 後面。
```

不是把 `main` 搬過來。

口訣：

```text
rebase main = 把我搬到 main 後面
```

因此執行前先確認自己在哪：

```bash
git branch --show-current
```

---

## 3. Two-Argument Form

```bash
git rebase A B
```

等價於：

```bash
git switch B
git rebase A
```

也就是：

```text
Git 會先切到 B，
再把 B 搬到 A 後面。
```

例子：

```bash
git rebase main docs/project-architecture
```

意思是：

```text
把 docs/project-architecture 搬到 main 後面，
並且 rebase 完通常會留在 docs/project-architecture 上。
```

---

## 4. Diagram

原本：

```text
main:
A - D

feature:
A - B - C
```

在 `feature` 上執行：

```bash
git rebase main
```

結果：

```text
main:
A - D

feature:
A - D - B' - C'
```

`B'`、`C'` 是重新套用後的新 commits，所以 commit hash 會改變。

---

## 5. Merge vs Rebase in This Project

目前 Pixel-Renderer 比較適合：

```text
branch 上允許探索，
main 上保留整理過的線性故事。
```

因此個人 local branch 預設 workflow：

```bash
git switch -c docs/project-architecture
# work, commit, commit, commit

git switch main
git pull --ff-only

git switch docs/project-architecture
git rebase main

git switch main
git merge --ff-only docs/project-architecture
git push origin main
```

這段流程的意思：

```text
1. 從 main 開一條工作 branch
2. 在 branch 上分批 commit
3. 回 main，確認 main 是遠端最新版
4. 把工作 branch 搬到最新 main 後面
5. 讓 main fast-forward 到工作 branch
6. 推送整理好的 main
```

---

## 6. Why `merge --ff-only`

```bash
git merge --ff-only docs/project-architecture
```

意思是：

```text
只有能 fast-forward 才 merge。
如果 main 和 branch 分岔，就直接失敗。
```

它的用途是避免 Git 自動產生不預期的 merge commit。

這是 main history 的 guardrail：

```text
main should stay readable, linear, and explainable.
```

如果 `--ff-only` 失敗，通常先做：

```bash
git switch docs/project-architecture
git rebase main
git switch main
git merge --ff-only docs/project-architecture
```

---

## 7. Safety Rule

可以放心 rebase：

```text
local branch
only used by yourself
not pushed, or nobody else depends on it
```

要小心 rebase：

```text
remote branch
shared branch
PR branch already reviewed by others
branch used across machines
```

原因：

```text
rebase rewrites commit hashes.
```

如果已經推到遠端且別人可能基於它工作，預設不要任意 rebase。這種情況可以用 merge 保留歷史，或先明確決定要重寫 branch。
