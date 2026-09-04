# Game UI 彈窗管理與 Navigation Architecture

日期: 2026-08-21

定位: 從 hardcoded Panel 互相開關開始, 逐步推導 `Navigation Stack`, `Modal`, `Popup Queue`, lifecycle, state ownership, async safety, presentation architecture, 以及它們如何接到 Pixel-Renderer 的 Immediate-mode UI 與 software rendering stack.

狀態: `notes/journal` 上的 long-form learning note. 這不是 current source truth, 也不是已採納的 architecture decision.

---

## 0. Source Boundary

這份筆記整理自 2026-08-21 匯出的 ChatGPT 對話:

```text
ChatGPT-彈窗管理架構解析-20260821-0435.md
```

原始對話包含多張影片截圖與兩輪教學回答. 本文保留它的問題驅動主線, 但不是逐字轉錄. 主要改動如下:

```text
1. 移除聊天過程, 重複段落與暫時性的下一輪邀請.
2. 將概念依賴重新排序成一篇可獨立閱讀的 note.
3. 補上 Screen / Modal / Overlay / Toast 的語義邊界.
4. 補上 Popup scheduler, input routing, focus, async race, testing.
5. 修正 call stack 類比, Dependency Inversion, Close/Pop 等容易過度簡化的說法.
6. 對照 Pixel-Renderer 現有 docs, 明確區分 future learning direction 與 current roadmap.
```

原始影片名稱, framework 與完整上下文無法只靠匯出截圖確認. 因此本文不宣稱「影片原作者的正式架構就是如此」, 只把截圖引出的問題當成 Game UI Architecture 的研究入口.

---

## 閱讀地圖

這份 NOTE 現在分成兩個閱讀模式:

```text
Part I
  完整教學正文.
  從一個真的能運作的 naive Panel 開始,
  用 15 章與逐步 Dry Run 推導整個 system.

Part II
  Architecture Reference Map.
  理解主線後, 用來快速查 type, policy, tradeoff 與 testing checklist.
```

Part I 的概念依賴:

```text
Ch01 Panel 互相 Show/Hide 可以運作
  |
  v
Ch02 Reusable Detail 讓 hardcoded Back 失敗
  |
  v
Ch03 Navigation Stack 擁有 history
  |
  v
Ch04 Push/Pop 與 Open/Close 分層
  |
  +--------------------+
  |                    |
  v                    v
Ch05 Instance       Ch06 History semantics
lifetime/cache      Replace/PopTo/Reset
  |                    |
  +----------+---------+
             |
             v
Ch07 Screen Stack 遇到 Confirm Modal
  |
  v
Ch08 Nested Modal 與 typed result
  |
  v
Ch09 Popup Queue / Scheduler
  |
  v
Ch10 Input routing / focus / Back
  |
  +--------------------+
  |                    |
  v                    v
Ch11 Service /       Ch12 Immediate-mode UI
ViewModel            與 UiCommandList
  |                    |
  +----------+---------+
             |
             v
Ch13 Async lifetime / cancellation
  |
  v
Ch14 End-to-End Full Dry Run
  |
  v
Ch15 Pixel-Renderer learning labs
```

如果第一次讀, 建議不要跳過 Ch01-Ch04. 那幾章故意走得慢, 因為後面的每個 manager/policy 都依賴「history 與 lifecycle 不是同一份 state」這個核心區分.

---

## Part I. 從 Panel 開關一路推導完整 UI System

這一部分不是摘要. 你可以把它當成一門小課, 真的跟著每次需求變化去跑 UI state. 每一章都採用同一條學習路徑:

```text
具體畫面
  -> 最直覺的 code
  -> Dry Run
  -> 新需求出現
  -> 原本哪個 assumption 失效
  -> 推導新的 abstraction
  -> 再跑一次驗證
```

先不要急著背 `UIManager`, `MVVM`, `Popup Queue` 等名詞. 如果沒有親眼看到前一個方案如何失敗, 新名詞只會變成另一層 cargo cult.

---

### Chapter 1. 只有三個畫面時, naive code 到底有沒有錯?

#### 1.1 我們先把世界縮到最小

假設遊戲目前只有:

```text
MainMenu
  |
  v
Shop
  |
  v
ItemDetail
```

使用者實際看到的三個畫面如下.

MainMenu:

```text
+------------------------------+
|          MAIN MENU           |
|                              |
|          [ Shop ]            |
|       [ Inventory ]          |
|                              |
+------------------------------+
```

Shop:

```text
+------------------------------+
|             SHOP             |
|                              |
| Iron Sword       [ Detail ]  |
| Leather Armor    [ Detail ]  |
|                              |
|            [ Back ]          |
+------------------------------+
```

ItemDetail:

```text
+------------------------------+
|         ITEM DETAIL          |
|                              |
| Iron Sword                   |
| ATK +100                     |
| Price: 800 Gold              |
|                              |
|            [ Back ]          |
+------------------------------+
```

這一章只問兩件事:

```text
現在顯示誰?
按 Back 回哪裡?
```

先故意不討論 gold, network, animation, renderer 或 ViewModel. 把變因隔離後, 才看得出 navigation 本身的形狀.

#### 1.2 最直覺的 implementation

```cpp
class MainMenuPanel {
public:
    void OnShopClicked() {
        Hide();
        shopPanel->Show();
    }
};

class ShopPanel {
public:
    void OnItemClicked(ItemId id) {
        Hide();
        detailPanel->Show(id);
    }

    void OnBackClicked() {
        Hide();
        mainMenuPanel->Show();
    }
};

class ItemDetailPanel {
public:
    void OnBackClicked() {
        Hide();
        shopPanel->Show();
    }
};
```

很多 architecture 文章會立刻說這樣「耦合很差」. 但先不要接受結論. 我們真的跑一次.

#### 1.3 Dry Run 1: MainMenu -> Shop -> Detail -> Back

初始 state:

```text
Frame 0

Panel          Visible
----------------------
MainMenu       true
Shop           false
ItemDetail     false
```

使用者按 `Shop`:

```text
Event
  MainMenu.OnShopClicked()

Commands
  MainMenu.Hide()
  Shop.Show()

Frame 1

Panel          Visible
----------------------
MainMenu       false
Shop           true
ItemDetail     false
```

使用者按 Iron Sword 的 `Detail`:

```text
Event
  Shop.OnItemClicked(Sword)

Commands
  Shop.Hide()
  ItemDetail.Show(Sword)

Frame 2

Panel          Visible       Args
-----------------------------------
MainMenu       false         -
Shop           false         -
ItemDetail     true          Sword
```

使用者按 `Back`:

```text
Event
  ItemDetail.OnBackClicked()

Commands
  ItemDetail.Hide()
  Shop.Show()

Frame 3

Panel          Visible
----------------------
MainMenu       false
Shop           true
ItemDetail     false
```

結果完全正確.

這裡的第一個重要觀念是:

> Naive code 不一定一出生就錯. 它通常是依賴了一組尚未被新需求打破的 assumption.

目前 hidden assumption 是:

```text
ItemDetail 永遠只會從 Shop 開啟.
```

只要這句永遠成立, `Back -> Shop` 就沒有錯.

#### 1.4 為什麼現在還不需要 UIManager?

如果此時立刻建立:

```text
UIManager
NavigationService
PanelFactory
ScreenRegistry
TransitionController
PopupScheduler
```

你不是在解決已知問題, 而是在為想像中的 scale 付 complexity cost. First Principles 的做法應該是:

```text
先寫下 current invariant.
等需求真的破壞 invariant.
再引入剛好能表達新事實的 abstraction.
```

#### 本章學到的三件事

1. 三個固定畫面互相 `Show/Hide` 可以完全正確, pattern 不是越早套越好.
2. 判斷 architecture 要找 hidden assumption, 不能只看 class 名稱是否漂亮.
3. 目前的問題只有 visible state 與 Back destination, 先不要把 renderer 和 business logic 混進來.

---

### Chapter 2. 同一個 Detail 被兩條路徑 reuse, naive model 才真正破掉

#### 2.1 新需求只多了一條 edge

產品說:

> Inventory 也可以點物品進入同一個 ItemDetail.

Navigation graph 從一條線變成:

```text
                 +---- Shop --------+
                 |                  |
MainMenu --------+                  +----> ItemDetail
                 |                  |
                 +---- Inventory ---+
```

兩條合法 history:

```text
History A:
  MainMenu -> Shop -> ItemDetail(Sword)

History B:
  MainMenu -> Inventory -> ItemDetail(Potion)
```

注意 `ItemDetail` 本身沒有改. 它仍然只需要顯示一個 `ItemId`. 改變的是「它可能從哪裡被開啟」.

#### 2.2 Dry Run 2A: 從 Shop 開啟仍然正確

```text
Step 0
  visible = MainMenu

Step 1
  click Shop
  visible = Shop

Step 2
  click Sword
  visible = ItemDetail(Sword)

Step 3
  click Back
  ItemDetail.OnBackClicked()
    -> ItemDetail.Hide()
    -> Shop.Show()

Result
  visible = Shop
```

舊路徑仍然正常, 所以這種 bug 很容易在只測 happy path 時漏掉.

#### 2.3 Dry Run 2B: 從 Inventory 開啟就走錯

Inventory code:

```cpp
void InventoryPanel::OnItemClicked(ItemId id) {
    Hide();
    detailPanel->Show(id);
}
```

實際執行:

```text
Step 0
  visible = MainMenu

Step 1
  click Inventory
  visible = Inventory

Step 2
  click Potion
  visible = ItemDetail(Potion)

Step 3
  click Back
  ItemDetail.OnBackClicked()
    -> ItemDetail.Hide()
    -> Shop.Show()            <-- hardcoded destination

Expected
  Inventory

Actual
  Shop
```

畫面上看起來像一個簡單的「回錯頁」bug, 但其實它暴露了一個 object responsibility 錯置:

```text
ItemDetail 正在回答一個不屬於它的問題:
  「使用者剛才從哪裡來?」
```

#### 2.4 第一個自然補丁: 傳入 source

```cpp
enum class DetailSource {
    Shop,
    Inventory,
};

class ItemDetailPanel {
public:
    void Show(ItemId id, DetailSource source) {
        itemId_ = id;
        source_ = source;
        visible_ = true;
    }

    void OnBackClicked() {
        Hide();

        switch (source_) {
        case DetailSource::Shop:
            shopPanel->Show();
            break;
        case DetailSource::Inventory:
            inventoryPanel->Show();
            break;
        }
    }
};
```

再 Dry Run 一次:

```text
Show(Potion, Inventory)
  -> source_ = Inventory

Back
  -> switch(source_)
  -> Inventory.Show()

Result
  correct
```

所以這個補丁也不是立刻錯. 真正的問題是 dependency graph 如何隨需求成長.

#### 2.5 source enum 的成長 Dry Run

三個月後, ItemDetail 可以來自:

```text
Shop
Inventory
Equipment
QuestReward
Mail
AuctionHouse
Crafting
CharacterPreview
```

Detail 的 dependency graph 變成:

```text
Shop ------------+
Inventory -------+
Equipment -------+
Mail ------------+----> ItemDetail ----> Shop
Crafting --------+          |           Inventory
Quest -----------+          |           Equipment
Auction ---------+          |           Mail
Preview ---------+          +---------- ...
```

每增加一個入口, 你不只改 caller, 還要改 `DetailSource`, `ItemDetail::OnBackClicked()`, test cases, 甚至 include/reference.

更糟的是 nested flow:

```text
Inventory -> ItemDetail -> Compare -> ItemDetail
```

一個 `source_` 只能記住一層 caller, 不能表示完整 history. 你會開始加:

```text
previousSource
previousPreviousSource
returnCallback
returnPanelId
```

本質上是在 Panel 裡偷偷手刻一個殘缺的 history system.

#### 2.6 First Principles: 哪個 object 擁有哪個 fact?

`ItemDetail` 必須知道:

```text
ItemId
UI-ready item data
user pressed Back / Buy / Equip
```

`ItemDetail` 不必知道:

```text
整個 App 的 screen topology
previous screen
previous previous screen
哪個 feature team 開啟它
```

真正擁有這個 fact 的 object 應該是:

```text
Navigation History
```

這就是導出 `Navigator` 的壓力. 不是因為遊戲公司都會有 `UIManager`, 而是因為 history 已經成為一份獨立 state.

#### 本章學到的三件事

1. Reuse destination 會打破 `Back` destination 寫在 Panel 裡的 assumption.
2. `source enum` 能暫時修 bug, 但會讓 reusable View 吸收整個 App topology.
3. `previous screen` 是 navigation history 的 fact, 不是 `ItemDetail` 的 intrinsic data.

---

### Chapter 3. Navigation Stack 不是魔法, 它只是把 history 放回正確 owner

#### 3.1 先用實體卡片建立直覺

想像桌上放著一疊 screen cards.

初始:

```text
TOP
 +------------------+
 | MainMenu         |
 +------------------+
BOTTOM
```

打開 Shop 時, 不是叫 Shop 記住 MainMenu, 而是把 Shop card 放到最上面:

```text
TOP
 +------------------+
 | Shop             |
 +------------------+
 | MainMenu         |
 +------------------+
BOTTOM
```

再打開 Detail:

```text
TOP
 +------------------+
 | ItemDetail(123)  |
 +------------------+
 | Shop(Weapon)     |
 +------------------+
 | MainMenu         |
 +------------------+
BOTTOM
```

Back 只做一件事: 拿掉 top card.

```text
POP ItemDetail(123)

TOP
 +------------------+
 | Shop(Weapon)     |  <-- 自然成為 current
 +------------------+
 | MainMenu         |
 +------------------+
BOTTOM
```

`ItemDetail` 不需要問「下面是不是 Shop」. Stack 已經保存實際走過的路徑.

#### 3.2 Call stack 類比在哪裡有幫助?

```cpp
void A() { B(); }
void B() { C(); }
void C() { /* return */ }
```

Call stack:

```text
TOP
 +---------+
 | C frame |  return 後移除
 +---------+
 | B frame |  自然恢復 execution
 +---------+
 | A frame |
 +---------+
BOTTOM
```

它幫助你看到:

```text
C 不需要 hardcode returnTo(B).
```

但 UI route 不等於 function frame. UI stack 還要自行定義 args, saved state, animation, cache, deep link 與 async lifetime. 類比只用來理解 history ownership.

#### 3.3 最小 Navigator

```cpp
enum class ScreenId {
    MainMenu,
    Shop,
    Inventory,
    ItemDetail,
};

struct Route {
    ScreenId id;
    ScreenArgs args;
};

class Navigator {
public:
    void Push(Route route);
    void Pop();

private:
    std::vector<Route> history_;
};
```

注意 stack 裡優先存 `Route`, 不是只有 `Panel*`:

```text
ScreenId::ItemDetail
```

只能說 screen kind. 真正 current route 是:

```text
ItemDetail(itemId = 123, routeInstance = 42)
```

#### 3.4 Dry Run 3A: 從 Shop 進 Detail

初始:

```text
history = [ MainMenu ]
current = history.back() = MainMenu
```

MainMenu button 不再直接 `Shop.Show()`:

```cpp
void MainMenuPanel::OnShopClicked() {
    navigator.Push(Route::Shop(Weapon));
}
```

`Push` 前後:

```text
Before
  history = [ MainMenu ]

Action
  Push(Shop(Weapon))

After
  history = [ MainMenu, Shop(Weapon) ]
  current = Shop(Weapon)
```

點 Sword:

```text
Before
  [ MainMenu, Shop(Weapon) ]

Action
  Push(ItemDetail(Sword123))

After
  [ MainMenu, Shop(Weapon), ItemDetail(Sword123) ]
```

Detail 的 Back code:

```cpp
void ItemDetailPanel::OnBackClicked() {
    navigator.Pop();
}
```

Pop:

```text
Before
  [ MainMenu, Shop(Weapon), ItemDetail(Sword123) ]

Remove top
  ItemDetail(Sword123)

After
  [ MainMenu, Shop(Weapon) ]
  current = Shop(Weapon)
```

#### 3.5 Dry Run 3B: 同一份 Detail 從 Inventory 開啟

```text
Initial
  [ MainMenu ]

Push Inventory
  [ MainMenu, Inventory ]

Push ItemDetail(Potion9)
  [ MainMenu, Inventory, ItemDetail(Potion9) ]

ItemDetail emits Pop
  [ MainMenu, Inventory ]

current
  Inventory
```

ItemDetail 的 code 完全沒有 `if (fromInventory)`.

同一個 `Pop()` 在不同 history 上得到正確結果:

```text
[Main, Shop, Detail]      --Pop--> [Main, Shop]
[Main, Inventory, Detail] --Pop--> [Main, Inventory]
```

這就是 aha moment:

> Back 不是「前往某個特定 screen」, 而是「撤銷目前這一層 navigation entry」.

#### 3.6 用數學語言描述 state transition

如果 stack 為 `S`, route 為 `r`:

```text
Push(S, r) = S concatenated with [r]
```

如果 `S = P concatenated with [top]`:

```text
Pop(S) = P
```

所以 previous route 不需要存成 current Panel 的 field. 它已經存在 `P` 的最後一個 element.

#### 本章學到的三件事

1. Stack 保存的是實際 history, 所以 reusable destination 不需要知道 caller.
2. `Back` 應表達 `Pop current entry`, 不是 `Open(previousPanelId)`.
3. Stack 應描述 route identity 與 arguments, 不只是保存一串 raw `Panel*`.

---

### Chapter 4. `Push` 不是 `Show`, `Pop` 也不是 `Close`

#### 4.1 兩層 API 看起來相似, 但修改不同 state

```text
+--------------------------------------+
| Navigation Layer                     |
|                                      |
| Push / Pop / Replace                 |
| modifies route history               |
+------------------+-------------------+
                   | coordinates
                   v
+--------------------------------------+
| Panel Lifecycle Layer                |
|                                      |
| Open / Enter / Suspend / Resume      |
| Exit / Close                         |
| modifies view instance state         |
+--------------------------------------+
```

`Show()` 只表示某個 instance visible. `Push()` 則表示:

```text
1. history 多一個 entry.
2. current destination 改變.
3. previous screen 進入某種 inactive/suspended policy.
4. new screen 開始 lifecycle.
```

#### 4.2 Dry Run 4A: 只 Close, 不 Pop

正確 history:

```text
[ MainMenu, Shop, ItemDetail ]
                         ^
                       current
```

錯誤 code:

```cpp
void ItemDetailPanel::OnBackClicked() {
    Close();
    shopPanel->Show();
}
```

執行後有兩份互相矛盾的 truth:

```text
Navigation state
  current = ItemDetail

Visible state
  Shop = true
  ItemDetail = false
```

下一個系統詢問 current route 時會得到 `ItemDetail`, 但使用者看到 Shop. 可能造成:

```text
Back 再按一次走錯
analytics 記錯 page
input routing 指向 hidden Detail
async callback 認為 Detail 仍 active
save/restore 寫出錯誤 history
```

#### 4.3 Dry Run 4B: 只 Pop, 但沒有 lifecycle coordination

另一個極端:

```cpp
history_.pop_back();
```

如果沒有 lifecycle layer 配合:

```text
history = [MainMenu, Shop]

but
ItemDetail instance still visible
ItemDetail listeners still subscribed
Shop still hidden
focus still belongs to Detail button
```

所以 Navigation state 是 authoritative intent, 但系統還需要把 state difference 落實到 view instances.

#### 4.4 完整 Push Dry Run

假設 policy 是 suspend previous:

```text
Before
  history = [MainMenu]
  MainMenu.lifecycle = Active

Action
  Push(Shop)

Phase 1: navigation mutation
  history = [MainMenu, Shop]

Phase 2: lifecycle diff
  MainMenu.Active -> Suspended
  Shop.Unloaded -> Entering -> Active

Phase 3: input/focus
  focus owner = Shop

Phase 4: render
  visible screen = Shop
```

完整 Pop:

```text
Before
  history = [MainMenu, Shop]
  Shop = Active
  MainMenu = Suspended

Action
  Pop()

Phase 1
  Shop.Active -> Exiting -> Closed

Phase 2
  history = [MainMenu]

Phase 3
  MainMenu.Suspended -> Resume -> Active

Phase 4
  focus owner = MainMenu
```

這裡 `Pop` 是 high-level operation. `Close current` 與 `Resume previous` 是它協調的 lifecycle consequence.

#### 4.5 Frame 100 / 150 / 170 再走一次

```text
Frame 100
  history = [MainMenu]
  active  = MainMenu

Input
  click Shop

End of Frame 100
  queued action = Push(Shop)
```

```text
Frame 101
  apply Push(Shop)
  history = [MainMenu, Shop]
  MainMenu = Suspended
  Shop = Active
```

```text
Frame 150
  input = click Sword
  queued action = Push(ItemDetail(Sword))

Frame 151
  history = [MainMenu, Shop, Detail(Sword)]
  Shop = Suspended
  Detail = Active
```

```text
Frame 170
  input = Back
  queued action = Pop()

Frame 171
  Detail = Closed
  history = [MainMenu, Shop]
  Shop = Active
```

把 navigation mutation 延後到 stable phase, 能避免 button callback 執行中就 destroy 自己造成 reentrancy 問題. 這不是唯一 implementation, 但 state phase 必須清楚.

#### 本章學到的三件事

1. `Push/Pop` 修改 navigation history, `Open/Close` 修改 Panel lifecycle.
2. 只改其中一層會產生「資料說 current 是 A, 畫面卻顯示 B」的 split-brain state.
3. High-level transition 必須協調 history, lifecycle, input focus 與 rendering, 但不代表它們是同一個概念.

---

### Chapter 5. Push 新頁面時, 舊頁面究竟該 Destroy, Hide 還是 Cache?

Navigation stack 只回答 history. 它沒有自動回答 instance lifetime.

假設:

```text
Shop -> ItemDetail
```

Detail 開啟時, Shop 有三個常見策略.

#### 5.1 Strategy A: Destroy previous view instance

```text
Before Push

Route History          View Instances
[Main, Shop]           Main(hidden)
                       Shop(active)

Push Detail

Route History          View Instances
[Main, Shop, Detail]   Main(hidden)
                       Shop(destroyed)
                       Detail(active)
```

注意 history 仍保留 `Shop` route. Pop Detail 時, 系統依 route 重新 create Shop:

```text
Pop Detail
  destroy Detail
  read previous route: Shop(category=Weapon)
  create Shop instance
  restore saved state if available
```

優點:

```text
memory pressure 低
inactive view 不會持續訂閱 event
recreate 後 state 較乾淨
```

代價:

```text
prefab/resource load cost
layout rebuild
scroll position / selection 要另外保存
return transition 可能卡頓
```

#### 5.2 Strategy B: Suspend / Hide previous instance

```text
View Stack

+------------------------------+
| Detail       visible, active |
+------------------------------+
| Shop         hidden, cached  |
+------------------------------+
| MainMenu     hidden, cached  |
+------------------------------+
```

Pop 時:

```text
Detail.Close()
Shop.Resume()
```

Shop 原本的 scroll position 仍在 instance 裡, return 很快.

代價是:

```text
hidden view 仍占 memory
subscription 若未 suspend 會偷跑
cached data 可能 stale
跨多層 navigation 時 instance 數量累積
```

#### 5.3 Strategy C: Route state + reusable instance cache

```text
Navigation History
  [Main, Shop(category=Weapon), Detail(item=123)]

Panel Cache
  MainMenuPanel  -> instance A
  ShopPanel      -> instance B
  DetailPanel    -> instance C

Route State Store
  Shop route #8  -> scroll=420, selected=Sword
  Detail route#9 -> item=123
```

這個模型把三件事分開:

```text
Route history
  使用者走過哪裡.

View instance cache
  有沒有可 reuse 的 object.

Route-local state
  回來時需要恢復什麼.
```

#### 5.4 Dry Run 5: Scroll state 的差別

使用者在 Shop 捲到第 80 個 item:

```text
Shop.scrollY = 1400
```

進 Detail 再 Back.

Destroy without saved state:

```text
Shop destroyed
Shop recreated
scrollY = 0                 <-- UX regression
```

Suspend:

```text
same Shop instance resumes
scrollY = 1400              <-- naturally preserved
```

Destroy with route state:

```text
save route#8.scrollY = 1400
destroy Shop
recreate Shop on Pop
restore scrollY = 1400      <-- preserved without retaining instance
```

所以「保留畫面狀態」不等於「一定要保留整個 Panel object」.

#### 5.5 `Open / Refresh / Close` 的完整語義

在 retained Panel framework 中:

```text
Open(args)
  first bind
  register listeners
  request/load resources
  start enter animation

Refresh(viewModel)
  apply new presentation data
  update enabled/visible/text/icon
  do not mutate navigation history

Close()
  stop accepting input
  unsubscribe
  cancel owned work
  start exit animation
  hide/cache/destroy according to policy
```

Dry Run:

```text
Open Shop(gold=1000)
  label = "1000"

Buy item succeeds
  domain gold = 800
  derive ShopViewModel(goldText="800")
  Refresh(viewModel)
  label = "800"

Navigation history remains
  [MainMenu, Shop]
```

在 declarative / Immediate-mode UI 裡, `Refresh()` 可能不是 explicit method. 下一 frame 重新以 `gold=800` 宣告 UI, output 自然更新. 重要的不是 method 名稱, 而是 source of truth 與 update path.

#### 本章學到的三件事

1. Route history, Panel instance 與 route-local view state 是三種不同 lifetime.
2. Destroy, suspend, cache 與 pool 是 policy choice, 不是 `Pop()` 的固定定義.
3. `Refresh` 更新 presentation, 不修改 navigation; declarative UI 甚至可能不需要 explicit `Refresh()`.

---

### Chapter 6. `Replace`, `PopTo`, `Reset` 為什麼不是 `Push` 的別名?

有了 stack 後, 很容易把所有 navigation 都寫成 `Push`. 新問題是: 並非每個 destination 都應該留在 Back history.

#### 6.1 Login -> Lobby 的 naive Push

初始 login flow:

```text
history = [Splash, Login]
```

登入成功後:

```cpp
navigator.Push(Route::Lobby());
```

得到:

```text
[Splash, Login, Lobby]
```

現在使用者在 Lobby 按 Back:

```text
Pop Lobby
  -> current = Login
```

這通常違反 product semantics. 已登入使用者不應回到可再次輸入帳密的舊 Login screen.

#### 6.2 Replace 的 Dry Run

```text
Before

TOP
 +---------+
 | Login   |
 +---------+
 | Splash  |
 +---------+
BOTTOM
```

```text
Action
  Replace(Lobby)
```

```text
After

TOP
 +---------+
 | Lobby   |
 +---------+
 | Splash  |
 +---------+
BOTTOM
```

形式上:

```text
Replace(S, r) = Push(Pop(S), r)
```

但 implementation 可能把 exit/enter animation 做成一個 atomic transition, 不必真的公開兩次中間 state.

#### 6.3 PopTo: 一次結束一段 flow

購買流程:

```text
MainMenu
  -> Shop
    -> ItemDetail
      -> Checkout
        -> PaymentResult
```

history:

```text
[MainMenu, Shop, ItemDetail, Checkout, PaymentResult]
```

如果完成後設計要求回 Shop, 連續手寫三次 `Pop()` 很脆弱:

```cpp
Pop();
Pop();
Pop();
```

它依賴 stack 恰好有三層. 更精確的 intent 是:

```cpp
navigator.PopTo(ScreenId::Shop);
```

Dry Run:

```text
Search from top
  PaymentResult  remove
  Checkout       remove
  ItemDetail     remove
  Shop           stop here

Result
  [MainMenu, Shop]
```

需要另外定義 `inclusive`:

```text
PopTo(Shop, inclusive=false)
  keep Shop.

PopTo(Shop, inclusive=true)
  remove Shop too.
```

#### 6.4 Reset: 建立新的 root

Logout 常不是 `PopTo(Login)`:

```text
Current history
  [Main, Lobby, Shop, Detail]

Logout
  authentication state cleared
  old authenticated screens should not survive
```

適合表達成:

```text
Reset(Login)

After
  [Login]
```

這裡還要 cancel 舊 route owner 的 async work 與 pending popup, 不能只清 vector.

#### 6.5 Tabs 為什麼常需要 multiple back stacks?

手機或遊戲主介面可能有:

```text
[Home] [Inventory] [Social]
```

使用者行為:

```text
Home tab:
  Home -> NewsDetail

Inventory tab:
  Inventory -> SwordDetail
```

如果共用一個 stack:

```text
[Home, NewsDetail, Inventory, SwordDetail]
```

切回 Home 時很難自然恢復 `NewsDetail`. 較合理的 state:

```text
selectedTab = Inventory

HomeStack
  [Home, NewsDetail]

InventoryStack
  [Inventory, SwordDetail]

SocialStack
  [Social]
```

Back policy 再定義為:

```text
1. Pop selected tab's stack.
2. 若已在該 tab root, 切回 default tab 或交給 app quit policy.
```

這說明 stack 是一種 history mechanism, 不是「整個 App 永遠只能有一個 vector」.

#### 本章學到的三件事

1. `Push`, `Replace`, `PopTo`, `Reset` 表達不同 UX history semantics.
2. 操作名稱應描述 intent, 不要用固定次數 `Pop()` 猜目前 stack shape.
3. Tabs, deep links 與 multi-window 可能需要多個 stack 或 graph/state machine, 單 stack 不是宇宙真理.

---

### Chapter 7. Confirm Dialog 出現後, 為什麼只有 Screen Stack 開始不自然?

#### 7.1 新需求: Detail 上刪除物品

目前 current screen:

```text
+--------------------------------+
| ITEM DETAIL                    |
|                                |
| Iron Sword                     |
| ATK +100                       |
|                                |
| [Delete]              [Back]   |
+--------------------------------+
```

按 Delete 後, 希望看到:

```text
+--------------------------------+
| ITEM DETAIL                    |
|                                |
|      +------------------+      |
|      | Delete item?     |      |
|      |                  |      |
|      | [Yes]      [No]  |      |
|      +------------------+      |
|                                |
+--------------------------------+
```

這個 UI 與 ItemDetail 的關係有四個重要 fact:

```text
1. ItemDetail 仍然 visible.
2. ItemDetail 暫時不能接 input.
3. Confirm 完成後回到同一個 ItemDetail instance/context.
4. Confirm 表達局部 decision, 不是使用者前往新功能頁.
```

#### 7.2 Naive 方案 A: 把 Confirm 當普通 Screen Push

```text
Screen Stack
  [MainMenu, Shop, ItemDetail, DeleteConfirm]
```

如果 Screen policy 是「只畫 top screen」:

```text
DeleteConfirm 取代 ItemDetail
背景 Detail 消失
```

你可能在 Confirm screen 裡手動重畫 Detail screenshot 或複製資料, 但那是 presentation duplication.

如果改成「Screen stack 全部疊著畫」:

```text
MainMenu, Shop, ItemDetail, Confirm 全都可能 render
```

又會遇到:

```text
哪些 hidden screen 要畫?
誰 dim background?
input 到哪層停止?
Screen transition animation 是否套在 Confirm?
analytics 是否把 Confirm 當 page view?
```

問題不是 vector 不能存 Confirm. 問題是同一個 stack policy 被迫同時表達兩種不同語義.

#### 7.3 Naive 方案 B: ItemDetail 自己保存 `bool showConfirm`

```cpp
class ItemDetailPanel {
    bool showDeleteConfirm_ = false;
};
```

Immediate-mode 寫法可能是:

```cpp
DrawItemDetail();

if (showDeleteConfirm_) {
    DrawDimLayer();
    DrawDeleteConfirm();
}
```

對一個 local confirm, 這可能正是最小且正確的方案. 不必每個小 confirm 都進 global manager.

它開始不足的時機是:

```text
多個 feature 都需要一致的 modal policy
global Back/Escape 要先 dismiss modal
需要 nested modal 或 cross-feature error
需要統一 animation, focus trap, input blocking
需要 popup scheduling
```

再次提醒: abstraction 應由 pressure 推導, 不是由名詞推導.

#### 7.4 推導出兩種 state channel

```text
Navigation State
  Screen Stack:
    [MainMenu, Shop, ItemDetail]

Presentation State
  Modal Stack:
    [DeleteConfirm]
```

畫面 composition:

```text
render MainMenu?       no, suspended screen policy
render Shop?           no, suspended screen policy
render ItemDetail?     yes, current screen
render dim barrier?    yes, modal policy
render DeleteConfirm?  yes, top modal
```

Input composition:

```text
pointer / gamepad / keyboard
          |
          v
   DeleteConfirm
          |
          +-- consumed -> stop
          |
          +-- not consumed but modal blocks -> stop

ItemDetail never receives this frame's click
```

#### 7.5 Dry Run 7: Present 與 Dismiss Confirm

Before:

```text
screens = [MainMenu, Shop, ItemDetail#42]
modals  = []
focus   = ItemDetail#42/DeleteButton
```

User presses Delete:

```text
ItemDetail emits
  PresentModal(DeleteConfirm(item=123, ownerRoute=42))
```

After present:

```text
screens = [MainMenu, Shop, ItemDetail#42]
modals  = [DeleteConfirm#7]
focus   = DeleteConfirm#7/NoButton

ItemDetail
  visible = true
  inputEnabled = false

DeleteConfirm
  visible = true
  inputEnabled = true
```

User presses No:

```text
DeleteConfirm emits
  DismissModal(result=Cancelled)
```

After dismiss:

```text
screens = [MainMenu, Shop, ItemDetail#42]
modals  = []
focus   = ItemDetail#42/DeleteButton   <-- restore previous focus
```

Screen history 完全沒變. 這正是 Modal 與 Screen navigation 分開後得到的語義清晰度.

#### 本章學到的三件事

1. Modal 的核心不是浮在上面, 而是保留背景 context 並阻斷下層 input.
2. Local `bool showConfirm` 可以是小規模正解; 出現一致 policy 壓力後才抽 global modal state.
3. Screen history 與 Modal presentation 是兩條 state channel, Present/Dismiss 不應偷偷改 Screen Stack.

---

### Chapter 8. Modal Stack 如何處理 nested dialog, 又在哪裡會誤導你?

#### 8.1 從 Confirm 再跳 Network Error

使用者在 Delete Confirm 按 Yes, 系統發 request, network 失敗.

最機械化的做法:

```text
Before request
  modals = [DeleteConfirm]

Network failed
  PresentModal(NetworkError)

After
  modals = [DeleteConfirm, NetworkError]
```

畫面:

```text
Layer 3  +---------------------+
         | Network Error       |  <-- top, owns input
         | [Retry] [Close]     |
         +---------------------+

Layer 2  +---------------------+
         | Delete item?        |  <-- suspended modal
         +---------------------+

Layer 1  ItemDetail                <-- blocked screen
```

Dismiss NetworkError:

```text
[DeleteConfirm, NetworkError]
                    |
                    +-- pop

[DeleteConfirm]
```

資料結構完全合理, 但 UX 未必合理.

#### 8.2 Dry Run 8A: 為什麼回 Confirm 可能造成 duplicate submit?

```text
Step 1
  DeleteConfirm.Active
  user presses Yes

Step 2
  request #100 starts
  DeleteConfirm should enter Submitting

Step 3
  request #100 fails
  NetworkError pushed

Step 4
  user dismisses NetworkError
  DeleteConfirm becomes Active again

Step 5
  if Yes button was never disabled/reset,
  user presses Yes again
  request #101 starts
```

所以「Error 疊在 Confirm 上, close 後回 Confirm」需要明確 retry semantics. Modal Stack 只知道 LIFO, 不知道 use case 是否能重試.

#### 8.3 三種 product policy

Policy A: Nested error:

```text
[DeleteConfirm, NetworkError]

Close error
  -> return Confirm
```

適合:

```text
錯誤只是補充資訊
原 decision 仍有效
retry 安全且 idempotent
```

Policy B: Replace modal:

```text
[DeleteConfirm]
   ReplaceTop
[DeleteError]

Close error
  -> return ItemDetail
```

適合:

```text
原 confirm flow 已結束
不希望自動回到可再次 submit 的 state
```

Policy C: Confirm 自己進 Failed state:

```text
DeleteConfirmState

Idle
  -> Submitting
  -> Failed(error)
  -> Submitting on explicit Retry
  -> Completed
```

畫面仍是一個 modal instance:

```text
+--------------------------+
| Delete item?             |
|                          |
| Network failed.          |
| [Retry]       [Cancel]   |
+--------------------------+
```

這通常最能表達「error 屬於同一個 use case」.

#### 8.4 Modal result 不能 hardcode caller

錯誤設計:

```cpp
void DeleteConfirm::OnYes() {
    itemDetailPanel->DeleteItem();
    Close();
}
```

Confirm 開始知道 caller 與 domain mutation.

較乾淨的 result:

```cpp
enum class ConfirmResult {
    Confirmed,
    Cancelled,
};
```

Possible flow:

```text
DeleteConfirm emits Confirmed(item=123)
  -> application/use case starts delete
  -> modal state becomes Submitting or dismissed by policy
```

使用 callback, Future/Promise 或 typed action 都可以, 但 caller lifetime 必須明確. 如果 owner route #42 已被 pop, result 應取消或被 state transition 拒絕.

#### 8.5 Dismiss 不等於 Destroy

```text
DismissModal
  removes presentation entry.

Close lifecycle
  stops input/subscription/animation.

Destroy
  frees instance/resources.
```

Modal cache 可能讓 Dismiss 後 instance 仍存在. 所以不要讓 `Close()` 一個名字同時隱含三層 policy.

#### 本章學到的三件事

1. Modal Stack 能保存 nested presentation history, 但不能替 use case 決定 retry/replace semantics.
2. Error 屬於 confirm、global system 還是獨立 modal, 必須看 product meaning, 不能只看畫面長得像 popup.
3. Modal 應回傳 typed result, 不直接 hardcode caller 或執行不屬於它的 domain mutation.

---

### Chapter 9. 五個 popup 同時抵達時, Stack 為什麼回答不了「誰先」?

#### 9.1 Login 完成的一瞬間

同一個 frame 可能收到:

```text
Daily Reward
Event Notice
Update Notice
Friend Invite
Achievement Unlocked
Network Warning
```

如果每個 subsystem 都直接:

```cpp
ui.PresentModal(...);
```

當幀 execution order 可能變成 presentation order:

```text
Achievement system callback first
  -> Achievement modal

Reward system callback second
  -> Reward modal over Achievement

Network callback third
  -> Network modal over Reward
```

結果取決於 callback timing, 不是明確 product policy.

#### 9.2 Stack 與 Queue 的問題不同

```text
Stack asks:
  current 關掉後回誰?

Queue asks:
  多個尚未顯示的 request, 下一個選誰?
```

所以要先分出:

```text
Pending Requests
  [DailyReward, EventNotice, Achievement]

Active Modal Stack
  [NetworkWarning]
```

Pending request 尚不是 visible Panel.

#### 9.3 最小 FIFO Dry Run

```text
t=0 enqueue DailyReward
  queue = [DailyReward]

t=1 enqueue EventNotice
  queue = [DailyReward, EventNotice]

t=2 scheduler sees no active modal
  dequeue DailyReward
  active = DailyReward
  queue  = [EventNotice]

t=3 enqueue Achievement
  active = DailyReward
  queue  = [EventNotice, Achievement]

t=4 dismiss DailyReward
  scheduler picks EventNotice
  active = EventNotice
  queue  = [Achievement]
```

FIFO 已經解決 callback timing 直接控制顯示順序的問題. 但 Network Error 可能不能等在 reward 後面.

#### 9.4 加入 priority 的 Dry Run

```text
Request             Priority
----------------------------
NetworkDisconnected 100
PurchaseConfirm      80
DailyReward          30
Achievement          10
```

Active 是 interruptible DailyReward, 此時 NetworkDisconnected 抵達:

```text
Before
  active = DailyReward
  queue  = [Achievement]

Enqueue NetworkDisconnected

Scheduler policy
  100 > 30
  active is interruptible
  suspend DailyReward
  present NetworkDisconnected

After
  active = NetworkDisconnected
  suspended = DailyReward
  queue = [Achievement]
```

Network restored:

```text
dismiss NetworkDisconnected
resume DailyReward
```

但如果 DailyReward 已播放不可逆 reward animation, resume 是否合理? 這又需要 `preempt and resume` 或 `preempt and cancel/requeue` policy.

#### 9.5 Dedupe Dry Run

網路抖動連續產生三個相同 request:

```text
NetworkDisconnected(connectionId=main)
NetworkDisconnected(connectionId=main)
NetworkDisconnected(connectionId=main)
```

沒有 dedupe:

```text
queue = [NetworkError, NetworkError, NetworkError]
```

使用 `dedupeKey = network/main/disconnected`:

```text
first request
  insert

second request
  same key exists -> merge/drop

third request
  same key exists -> merge/drop

queue = [NetworkError]
```

#### 9.6 只看 priority 會 starvation

假設每秒都有 priority 50 event notice, priority 10 achievement 會一直被往後推.

```text
t0 queue: Achievement(10)
t1 EventA(50) arrives -> show EventA
t2 EventB(50) arrives -> show EventB
t3 EventC(50) arrives -> show EventC
...
Achievement never appears
```

可選 policy:

```text
aging: wait 越久 effective priority 越高
channel separation: Achievement 改走 Toast channel
rate limit: Event Notice 每段時間只顯示一次
batching: 多個 achievement 合併
explicit UX order: login flow 用 scripted sequence
```

這解釋為什麼 production 系統更接近 scheduler, 不只是 `std::queue<PanelId>`.

#### 9.7 一個完整 request state machine

```text
Created
  |
  v
Pending --dedupe/cancel/expire--> Dropped
  |
  v
Selected
  |
  v
Entering
  |
  v
Active <----> Suspended
  |
  v
Exiting
  |
  v
Completed
```

每條 transition 都需要 owner 與 trace. 否則 user logout 後, 舊帳號的 reward popup 仍可能從 queue 冒出來.

#### 本章學到的三件事

1. Stack 管 active presentation history, Queue/Scheduler 管尚未顯示 request 的選擇順序.
2. Production popup policy 還需要 priority, dedupe, interruptibility, expiry, ownership 與 fairness.
3. 不同語義應拆 channel; Fatal system error, modal decision 與 non-blocking toast 不必競爭同一個 queue.

---

### Chapter 10. Modal 真正困難的地方是 Input Routing, 不是畫黑色背景

#### 10.1 一個會穿透的 click

畫面上:

```text
+--------------------------------+
| ItemDetail                     |
|                                |
|      +------------------+      |
|      | Delete item?     |      |
|      | [Yes]      [No]  |      |
|      +------------------+      |
|                  [Equip]       |
+--------------------------------+
```

假設 `No` button 與底下 `Equip` button 的 screen-space rect 部分重疊. Mouse release 發生在 `(220, 180)`.

沒有 input barrier 的 naive dispatch:

```text
MouseUp(220, 180)
  -> DeleteConfirm hit test
       No button receives click
       Dismiss modal

  -> dispatch continues
  -> ItemDetail hit test
       Equip button also receives same MouseUp
       Equip item
```

使用者只想關掉 dialog, 卻順便裝備物品. 這就是 click-through.

#### 10.2 正確 dispatch 要從 top layer 往下

```text
Input Event
    |
    v
+---------------------------+
| System Overlay            |
+-------------+-------------+
              | not blocking?
              v
+---------------------------+
| Top Modal                 |
+-------------+-------------+
              | no modal barrier?
              v
+---------------------------+
| Current Screen            |
+-------------+-------------+
              | not consumed?
              v
+---------------------------+
| HUD / Game World          |
+---------------------------+
```

Modal barrier 的 semantics:

```text
event consumed by modal control
  -> stop.

event not hitting a modal control
but modal blocks lower layers
  -> stop at barrier.

no blocking modal
  -> continue downward.
```

#### 10.3 Dry Run 10: Same-frame dismiss 特別危險

```text
Start of Frame
  modals = [DeleteConfirm]
```

MouseUp 進來, modal button callback 立刻執行:

```cpp
DismissModal();
```

如果它當下就把 modal 從 container 移除, input loop 接著看到:

```text
modals.empty() == true
```

便可能把同一個 MouseUp 傳給 screen. 解法之一:

```text
1. 本次 event 的 blocking decision 在 dispatch 開始時 snapshot.
2. callback 只 enqueue DismissModal action.
3. event dispatch 結束後才 apply modal mutation.
```

Dry Run:

```text
Dispatch start
  blockingOwner = DeleteConfirm#7

No clicked
  enqueue DismissModal#7
  mark event consumed

Dispatch end
  do not route event below blockingOwner

Stable phase
  apply DismissModal#7
  restore ItemDetail focus
```

#### 10.4 Focus 不是 hover

至少分開:

```text
hot
  pointer currently over which widget.

active
  which widget owns current press/drag interaction.

focused
  which widget receives keyboard/text/gamepad action.

navigation focus
  gamepad/keyboard selection cursor.
```

Present modal 時:

```text
save previous focus path
  ItemDetail#42/DeleteButton

move focus into modal
  DeleteConfirm#7/NoButton

trap Tab/gamepad navigation inside modal
```

Dismiss 後:

```text
if previous widget still exists and enabled
  restore it.
else
  choose current screen's default focus.
```

如果不做這件事, keyboard/gamepad 使用者可能關 dialog 後完全失去操作位置.

#### 10.5 Pointer capture

使用者在 slider/button 上 press 後拖出 rect, release 仍應送回原 active widget. 如果中途跳出 modal:

```text
pointer captured by ItemDetail slider
  -> system modal appears
```

policy 必須決定:

```text
cancel underlying capture
or
finish capture before modal becomes active
```

不能同時讓 modal 與 hidden widget 都認為自己擁有 drag.

#### 10.6 Back / Escape priority

```text
BackPressed
  |
  +-- non-dismissible SystemOverlay active?
  |     -> ignore / explain
  |
  +-- top Modal active?
  |     -> modal handles or dismisses
  |
  +-- temporary Overlay active?
  |     -> close overlay
  |
  +-- Screen Stack has previous?
  |     -> Pop screen
  |
  +-- at root
        -> app/OS exit policy
```

Back 的順序是 product policy, 但必須只有一個 authoritative resolver. 否則 modal 與 screen 同時監聽 Escape, 一次按鍵可能關兩層.

#### 10.7 Block input 不等於 pause simulation

```text
Single-player pause dialog
  UI blocks input
  simulation may pause.

Online network warning
  UI blocks local control
  server/world cannot pause.

Inventory overlay
  may block movement
  background animation/audio may continue.
```

所以 `PresentModal()` 不應自動硬寫 `timeScale = 0`. Input policy 與 simulation policy 是不同 concern.

#### 本章學到的三件事

1. Modal 的硬 contract 是阻斷或接管 lower-layer input, 不只是多畫一層背景.
2. Same-frame dismiss, focus restore 與 pointer capture 都需要明確 phase/lifetime policy.
3. Back resolution, input blocking 與 simulation pause 是三件事, 不應由一個 `isPopupOpen` bool 隱式決定.

---

### Chapter 11. UI 直接扣 Gold 後, 為什麼會推導出 Service 與 ViewModel?

到目前為止只處理「顯示誰」. 現在讓 Shop 真的可以買東西.

#### 11.1 Naive business code

```cpp
void ShopPanel::OnBuyClicked(ItemId id) {
    Player& player = GameManager::Instance().Player();
    int price = ShopDatabase::Instance().Price(id);

    if (player.gold >= price) {
        player.gold -= price;
        Inventory::Instance().Add(id);

        goldLabel_->SetText(std::to_string(player.gold));
        inventoryPanel_->Refresh();
        OpenRewardPopup(id);
    } else {
        OpenInsufficientGoldPopup();
    }
}
```

它在一個 click callback 裡做了:

```text
read player state
lookup price
validate rule
mutate wallet
mutate inventory
format label
refresh another Panel
decide navigation/popup
```

小 prototype 仍然能跑. 我們同樣先 Dry Run, 不急著貼 pattern.

#### 11.2 Dry Run 11A: 買成功

```text
Initial domain state
  gold = 1000
  inventory = []

UI state
  goldLabel = "1000"
  item = Sword(price=800)
```

Click Buy:

```text
price = 800
1000 >= 800 -> pass
gold = 1000 - 800 = 200
inventory.add(Sword)
goldLabel = "200"
open reward popup
```

結果正確.

#### 11.3 新規則讓 callback 開始膨脹

後續增加:

```text
inventory capacity
event discount
VIP discount
item purchase limit
server-authoritative price
network transaction
rollback on failure
analytics
```

callback 變成:

```text
View
  knows wallet
  knows inventory capacity
  knows pricing campaign
  knows network protocol
  knows transaction rollback
  knows formatting
  knows navigation
```

換一版 Shop skin 或增加 console UI 時, 你被迫複製相同 purchase rule.

#### 11.4 把 user intent 與 use case 分開

View 只知道:

```text
user wants to buy item 123.
```

```cpp
void ShopView::OnBuyClicked(ItemId id) {
    actions_.Push(BuyItemRequested{id});
}
```

Application Service / Use Case 知道如何完成 purchase:

```cpp
BuyResult BuyItem::Execute(ItemId id) {
    const Price price = pricing_.Resolve(id, player_.status());

    if (!inventory_.HasSpace()) {
        return InventoryFull{};
    }

    if (!wallet_.CanAfford(price)) {
        return InsufficientFunds{};
    }

    wallet_.Debit(price);
    inventory_.Add(id);
    return BuySucceeded{id, wallet_.Balance()};
}
```

這只是示意. 線上交易可能必須由 server authoritative result 決定, 不能先 local commit.

#### 11.5 Dry Run 11B: Service flow

```text
Frame 200
  user clicks Buy(Sword)

View
  emits BuyItemRequested(Sword)
  does not change gold label yet
```

```text
Use Case
  resolve price = 800
  inventory has space = true
  wallet can afford = true
  execute mutation
  return BuySucceeded(balance=200)
```

```text
Presentation update
  derive ShopViewModel
    goldText = "200"
    swordButtonEnabled = false if limit reached

  navigation/presentation policy
    enqueue RewardPopup(Sword)
```

```text
Next render
  View displays ViewModel
```

#### 11.6 失敗結果也是 typed state

```cpp
using BuyResult = std::variant<
    BuySucceeded,
    InsufficientFunds,
    InventoryFull,
    ItemUnavailable,
    NetworkFailure
>;
```

Presentation policy 可以決定:

```text
InsufficientFunds
  -> PresentModal(InsufficientGold)

InventoryFull
  -> PresentModal(InventoryFull)

NetworkFailure
  -> update current confirm to Failed

ItemUnavailable
  -> Refresh Shop + show Toast
```

Service 回傳 domain/use-case result. 它不應直接呼叫 `goldLabel.SetText()` 或 `OpenPanel()`.

#### 11.7 ViewModel 是 representation adapter

Domain:

```cpp
struct Item {
    ItemId id;
    Money price;
    Timestamp expireAt;
    Rarity rarity;
};
```

View 真正想畫:

```cpp
struct ItemViewModel {
    std::string title;
    std::string priceText;
    std::string expireText;
    IconId icon;
    FrameStyle frameStyle;
    bool buyEnabled;
};
```

Mapping:

```text
expireAt = 1787347200
  -> expireText = "03:14:52"

rarity = Legendary
  -> frameStyle = LegendaryGold

wallet balance < price
  -> buyEnabled = false
```

ViewModel 不是另一份 authoritative inventory. 它是 UI-ready projection, 可以重新 derive.

#### 11.8 MVVM 不是唯一答案

Retained XAML/UI Toolkit 類 framework 常適合 data binding + ViewModel. Immediate-mode debug tool 可能直接從 application state 宣告 UI. Presenter 或 reducer/store 也可以建立相同邊界.

真正要守的是:

```text
View does not own domain truth.
Domain does not know widget API.
Presentation formatting has a named owner.
Navigation policy does not become purchase rule.
```

#### 本章學到的三件事

1. View 應表達 user intent, purchase rule 與 transaction 應落在可測試的 use-case/domain boundary.
2. ViewModel 把 domain representation 轉成 UI-ready representation, 不是第二份 domain source of truth.
3. Service, MVVM, Presenter 或 Store 都只是可選 mechanism; 真正目標是 data/business/presentation ownership 清楚.

---

### Chapter 12. Immediate-mode UI 和 Navigation Stack 到底怎麼接?

使用 Immediate-mode UI 並不會讓 navigation 問題消失. 它只是改變「View 如何被描述」.

#### 12.1 最小 state-driven model

```cpp
struct AppUiState {
    NavigationState navigation;
    ModalState modals;
    ShopPresentationState shop;
};
```

每 frame:

```cpp
void DrawUi(UiContext& ui, AppUiState& state) {
    const Route& current = state.navigation.Current();

    switch (current.id) {
    case ScreenId::MainMenu:
        DrawMainMenu(ui, state);
        break;
    case ScreenId::Shop:
        DrawShop(ui, state.shop);
        break;
    case ScreenId::ItemDetail:
        DrawItemDetail(ui, current.args);
        break;
    }

    DrawTopModal(ui, state.modals);
}
```

這裡的關係是:

```text
Navigation State
  decides which screen declaration runs.

UiContext
  decides widget identity, layout, interaction.

UiCommandList
  records what should be drawn this frame.
```

#### 12.2 Dry Run 12: Button click 到 next frame pixels

Frame N state:

```text
navigation = [MainMenu]
input.mouse = click on Shop button
```

UI declaration:

```cpp
if (ui.Button("Shop")) {
    actions.Push(PushScreen{Route::Shop()});
}
```

在 widget call 期間:

```text
UiContext
  computes button rect
  resolves hot/active/clicked
  emits DrawRect + DrawText

Action Queue
  receives PushScreen(Shop)
```

Frame N render output 仍可能是 MainMenu draw list. Stable update phase apply action:

```text
navigation
  [MainMenu] -> [MainMenu, Shop]
```

Frame N+1:

```text
current = Shop
DrawShop() runs
UiCommandList contains Shop widgets
software backend rasterizes them
```

#### 12.3 Immediate-mode 不代表 stateless

State owner map:

```text
Application-owned persistent state
  navigation history
  selected item
  wallet balance
  modal request state

UiContext interaction state
  hot widget ID
  active widget ID
  focused widget ID
  ID stack

Per-frame state
  layout cursor
  clip stack
  UiCommandList

Renderer-owned state/resources
  font atlas
  framebuffer
  blend/clip execution
```

所以 `Button()` 每 frame 被呼叫, 不代表 App 忘記 navigation. Navigation state 明確存在 application layer.

#### 12.4 Retained UI 的對照

Retained:

```text
Navigation State changes
  -> coordinator activates/creates Panel object
  -> Panel owns widget tree
  -> framework invalidates/layouts/renders changed nodes
```

Immediate:

```text
Navigation State changes
  -> next frame calls a different screen declaration
  -> UiContext rebuilds interaction/draw description
  -> renderer executes new draw list
```

兩者都仍需:

```text
navigation
modal policy
input routing
business separation
async lifetime
```

#### 12.5 Pixel-Renderer vertical slice

```text
InputState
    |
    v
AppUiState + UiAction Reducer
    |
    v
Screen / Modal declaration
    |
    v
UiContext
  ID, hot, active, focus, layout
    |
    v
UiCommandList
  rect, text, image, clip, layer
    |
    v
UI Software Renderer
  rasterize, sample font, alpha blend
    |
    v
Owned Framebuffer
    |
    v
DisplayBackend::Present()
```

這張圖的上半部是本篇 navigation/presentation. 下半部是 Pixel-Renderer 已規劃的 UI rendering boundary.

#### 本章學到的三件事

1. Immediate-mode UI 決定 View declaration 方式, 不取代 Navigation, Modal 或 business state.
2. App state, UiContext interaction state, per-frame draw list 與 renderer resource 各有不同 owner/lifetime.
3. Button click 到 pixels 是跨多層資料流; 每層分開後才可單獨 trace 與 test.

---

### Chapter 13. Async callback 回來時, 原本的 UI 可能早就不是原本那個 UI

UI 在同步 demo 裡很乾淨. 一加入 network/resource loading, route 與 request lifetime 就開始交錯.

#### 13.1 最典型的 stale callback

```text
Frame 10
  Push ItemDetail(item=123)
  route instance = Detail#42
  start LoadItem(123), request = Req#900

Frame 11
  user presses Back
  Pop Detail#42
  Detail Panel returned to cache

Frame 12
  user opens ItemDetail(item=456)
  route instance = Detail#43
  cached Detail Panel instance reused
  start LoadItem(456), request = Req#901

Frame 30
  old Req#900 completes
  callback writes "Item 123" into cached Panel

Current screen
  Detail#43(item=456)

Visible result
  route says item 456
  label shows item 123
```

這種 bug 不一定 crash. 它可能只是偶發顯示錯資料, 更難追.

#### 13.2 只檢查 pointer 不為 null 沒用

```cpp
if (detailPanel != nullptr) {
    detailPanel->SetData(result);
}
```

Panel 確實存在, 但已代表另一個 route. 所以至少需要:

```text
RouteInstanceId
  這一次 ItemDetail entry 的 identity.

RequestId
  這一次 async operation 的 identity.

Expected Args/Key
  result 應屬於哪個 ItemId.
```

#### 13.3 Safe completion Dry Run

Start request:

```text
route = Detail#42(item=123)
request = Req#900(ownerRoute=42, key=item/123)
```

Response arrives:

```text
Check 1
  route #42 still exists?
  no

Decision
  discard result for presentation
  domain cache may still accept data if policy allows
```

如果 route 還存在:

```text
Check 1: owner route exists       yes
Check 2: current request matches  yes
Check 3: key matches item/123     yes
Check 4: cancellation token live  yes

Then
  update presentation state for Detail#42
```

#### 13.4 Cancellation 不一定代表 network 真正停止

有些 API 可以 cancel underlying work, 有些只能讓 caller 忽略 result.

```text
Cancel token semantic A
  actively abort HTTP/resource request.

Cancel token semantic B
  request may continue,
  but completion must not update disposed owner.
```

兩者都要避免 stale presentation mutation.

#### 13.5 Out-of-order response

搜尋欄位:

```text
t0 type "s"
   request #1 Search("s")

t1 type "sw"
   request #2 Search("sw")

t2 request #2 returns first
   show results for "sw"

t3 request #1 returns later
   naive callback overwrites with results for "s"
```

解法:

```text
latestRequestId = #2

when #1 completes
  #1 != latestRequestId
  discard presentation update
```

#### 13.6 Loading state 不應是互相矛盾的 booleans

Naive:

```cpp
bool loading;
bool hasData;
bool hasError;
```

可能出現非法組合:

```text
loading = true
hasData = true
hasError = true
```

用 sum type/state machine:

```cpp
using DetailLoadState = std::variant<
    Idle,
    Loading<RequestId>,
    Loaded<ItemDetailViewModel>,
    Failed<UiError>
>;
```

Transition:

```text
Idle
  -> Loading(#900)
  -> Loaded(data)

or

Idle
  -> Loading(#900)
  -> Failed(error)
  -> Loading(#901) on Retry
```

#### 13.7 Double submit 與 idempotency

Purchase button 連按:

```text
Frame 50 click Buy
  request #A starts

Frame 51 click Buy again
  request #B starts

Server accepts both
  charged twice
```

UI 層應進 `Submitting` 並 disable button:

```text
Confirm.Idle
  -> click
  -> Confirm.Submitting
  -> button disabled
```

但 UI disable 不能取代 server/domain idempotency. Network boundary 仍應使用 idempotency key 或 transaction identity, 因為 retry, packet duplication 或 client bug 都可能重送.

#### 13.8 Owner token 與 popup queue

登入帳號 A 時 enqueue reward, 立刻 logout 再登入帳號 B:

```text
Pending popup
  Reward(account=A, ownerSession=Session#1)

Logout
  invalidate Session#1

Login B
  Session#2

Scheduler must not show
  Reward(account=A)
```

Pending UI request 也需要 owner/lifetime, 不只是 async data request.

#### 本章學到的三件事

1. Panel pointer 存在不代表它仍代表同一個 route; async completion 要核對 route identity, request identity 與 key.
2. Cancellation, out-of-order result, cache reuse 與 double submit 都是 lifetime/state machine 問題.
3. UI 防連點改善 interaction, 但不可取代 domain/network idempotency.

---

### Chapter 14. End-to-End Dry Run: 從 MainMenu 一路到刪除成功與 Reward Popup

這一章不再引入新名詞. 我們把前面所有 state 放在同一條 timeline, 看每一層到底何時改變.

#### 14.1 State columns

每一步都追五份 state:

```text
Screens
  navigation history.

Modals
  active modal presentation history.

Pending
  尚未顯示的 popup requests.

Focus
  目前 input owner/widget.

Async
  in-flight request and owner.
```

#### 14.2 Frame 0: MainMenu

```text
Screens  [MainMenu#1]
Modals   []
Pending  []
Focus    MainMenu#1/ShopButton
Async    []
```

Render tree:

```text
MainMenu declaration
  -> Button(Shop)
  -> Button(Inventory)
  -> UiCommandList
  -> Framebuffer
```

#### 14.3 Frame 10: Click Shop

Input:

```text
MouseUp on ShopButton
```

Widget result:

```text
ShopButton.clicked = true
```

Emitted action:

```text
PushScreen(Shop(category=Weapon))
```

Stable phase:

```text
Screens
  [MainMenu#1]
      ->
  [MainMenu#1, Shop#2(Weapon)]

Lifecycle
  MainMenu#1 Active -> Suspended
  Shop#2 Entering -> Active

Focus
  Shop#2/FirstItem
```

#### 14.4 Frame 30: Click Sword Detail

```text
Screens before
  [MainMenu#1, Shop#2]

Action
  PushScreen(ItemDetail(item=123))

Screens after
  [MainMenu#1, Shop#2, ItemDetail#42(item=123)]

Route state saved for Shop#2
  scrollY = 1400
  selected = Sword

Async
  LoadItem Req#900(owner=Route#42, key=item/123)

Focus
  ItemDetail#42/BackButton while loading
```

畫面可能先顯示 skeleton/loading state:

```text
+------------------------------+
| ITEM DETAIL                  |
|                              |
| Loading item...              |
|                              |
|                     [Back]   |
+------------------------------+
```

#### 14.5 Frame 40: Item data returns

Completion validation:

```text
Route#42 exists?      yes
Req#900 current?      yes
key item/123 matches? yes
```

State:

```text
DetailLoadState
  Loading(#900)
    -> Loaded(ItemDetailViewModel)
```

Next declaration renders Sword data.

#### 14.6 Frame 50: Click Delete

Before:

```text
Screens  [MainMenu#1, Shop#2, Detail#42]
Modals   []
Focus    Detail#42/DeleteButton
```

Action:

```text
PresentModal(DeleteConfirm#7(item=123, ownerRoute=42))
```

After:

```text
Screens  [MainMenu#1, Shop#2, Detail#42]
Modals   [DeleteConfirm#7]
Pending  []
Focus    DeleteConfirm#7/NoButton
Async    []
```

Render order:

```text
1. ItemDetail
2. Dim barrier
3. DeleteConfirm
```

Input order:

```text
DeleteConfirm -> barrier stop
Detail receives nothing
```

#### 14.7 Frame 55: Confirm Yes

Event:

```text
DeleteConfirm#7 emits DeleteConfirmed(item=123)
```

Use case:

```text
start DeleteItem Req#901
owner modal = #7
owner route = #42
```

Modal state:

```text
DeleteConfirm#7
  Idle -> Submitting(#901)

Yes button disabled
No/Cancel policy explicitly defined
```

Global state:

```text
Screens  [MainMenu#1, Shop#2, Detail#42]
Modals   [DeleteConfirm#7(Submitting)]
Pending  []
Focus    DeleteConfirm#7
Async    [Req#901 DeleteItem(123)]
```

#### 14.8 Frame 80: First request fails

Validation:

```text
Modal#7 exists?       yes
Route#42 exists?      yes
Req#901 current?      yes
```

Chosen product policy: confirm owns failure.

```text
DeleteConfirm state
  Submitting(#901)
    -> Failed(NetworkUnavailable)
```

畫面:

```text
+------------------------------+
| Delete Iron Sword?           |
|                              |
| Network unavailable.         |
|                              |
| [Retry]           [Cancel]   |
+------------------------------+
```

注意:

```text
Modal Stack 沒有 push NetworkError.
它仍是同一個 DeleteConfirm#7.
```

#### 14.9 Frame 90: Retry, then success

```text
Click Retry
  start Req#902
  Failed -> Submitting(#902)

Req#902 succeeds
  domain removes item 123
  inventory presentation invalidated/refreshed
  DeleteConfirm -> Completed
```

Presentation policy:

```text
DismissModal#7
Replace current Detail with ItemDeletedResult?
or Pop Detail back to Shop?
```

假設 design 要回 Shop 並顯示 toast:

```text
Actions
  DismissModal#7
  PopScreen()            // remove Detail#42
  EnqueueToast(ItemDeleted("Iron Sword"), owner=Session#5)
```

Apply after current event phase:

```text
Screens
  [MainMenu#1, Shop#2, Detail#42]
    -> [MainMenu#1, Shop#2]

Modals
  [DeleteConfirm#7]
    -> []

Pending Toast
  [ItemDeleted]

Focus
  Shop#2/previousSelectedItem or fallback

Async
  []
```

Shop route state restores:

```text
scrollY = 1400
selected Sword no longer exists
fallback focus = nearest remaining item
```

#### 14.10 Frame 91: Toast channel

Scheduler sees toast channel free:

```text
Toast active = ItemDeleted("Iron Sword")
```

Composition:

```text
+------------------------------+
| SHOP                  +-----+|
|                       |Item ||
|                       |deleted
| Potion                       |
| Armor                        |
+------------------------------+
```

Toast does not block Shop input. 它有 TTL, 到期後自行完成, 不進 Screen Back history.

#### 14.11 如果 Frame 56 使用者強制離開呢?

假設 Delete request 尚未完成, user/session transition 把 route 清掉:

```text
Reset(Login)
```

Cleanup:

```text
invalidate Route#42
invalidate Modal#7
cancel/ignore Req#901 result
drop pending UI requests owned by old session
restore new Login root
```

Req#901 之後成功回來也不能重新打開舊 reward/error UI. Domain 是否接受 server result, 則由 transaction/domain policy 處理, 和 presentation cancellation 分開.

#### 14.12 整條 vertical trace

```text
User click
  -> Widget event
  -> UiAction
  -> Navigation/Modal state transition
  -> lifecycle + focus diff
  -> use case / async effect
  -> domain result
  -> presentation state
  -> screen/modal declaration
  -> UiCommandList
  -> software rasterization
  -> Framebuffer
  -> DisplayBackend present
```

現在你可以指出每個 bug 屬於哪一段, 而不是只說「UIManager 好像壞了」.

#### 本章學到的三件事

1. 一次完整 UI flow 同時改變 navigation, modal, focus, async 與 presentation state, trace 必須把它們分欄觀察.
2. Product policy 決定 error/retry/success 後去哪; stack/queue 只提供 mechanism.
3. UI action 到 framebuffer 是 vertical pipeline, 每層都能建立 deterministic test 與 debug trace.

---

### Chapter 15. 要如何把這個議題真的學進 Pixel-Renderer, 而不是只多一份文件?

目前 Pixel-Renderer 的 stable priority 仍是可信 raster pipeline. 所以不應因為這篇很完整, 就立刻把 full production UI system 插進 `main`.

比較適合的四階段實驗.

#### 15.1 Lab A: Headless Navigation

完全不開 window:

```text
Input
  sequence of UiAction

State
  Screen Stack
  Modal Stack
  Pending Queue

Output
  text trace
```

Test:

```text
Push/Pop reusable Detail
Replace Login with Lobby
PopTo Shop
Present/Dismiss Confirm
Back priority
```

這一階段只驗證 state machine.

#### 15.2 Lab B: ASCII UI Simulator

把 state render 成 console diagram:

```text
Screens: [Main, Shop, Detail]
Modals : [DeleteConfirm]
Focus  : DeleteConfirm/No

+----------------------+
| Detail               |
|   +--------------+   |
|   | Delete?      |   |
|   +--------------+   |
+----------------------+
```

它讓 navigation bug 與 pixel renderer bug 完全隔離.

#### 15.3 Lab C: Immediate-mode Debug UI

當 `UiContext` 與 owned `Framebuffer` 時機成熟:

```text
Navigation State
  -> DrawCurrentScreen(ui)
  -> DrawTopModal(ui)
  -> UiCommandList dump
```

先做:

```text
Button
Label
simple window
modal dim barrier
focus indicator
```

不要一開始做 production animation, localization, accessibility, resource streaming 與 full MVVM.

#### 15.4 Lab D: Full trace + golden image

同一組 test fixture 同時產生:

```text
state transition trace
UiCommandList dump
small framebuffer golden image
```

故障定位:

```text
state trace 錯
  -> navigation/presentation bug.

state trace 對, command dump 錯
  -> widget/layout bug.

command dump 對, pixels 錯
  -> clip/font/blend/raster bug.
```

#### 15.5 與商業引擎做 Engine Mirror

同一個 case 可以在 Unity/Unreal/Godot UI 裡觀察:

```text
ItemDetail reuse
Back stack
modal focus/input blocking
panel cache/pool
async cancellation
```

比較的不是 class 名稱, 而是:

```text
history owner
view lifetime
input dispatch
data binding/state source
render backend boundary
```

這正符合 Pixel-Renderer 的同題雙解策略.

#### 本章學到的三件事

1. 先用 headless state experiment 學 navigation, 不要讓 Win32/widget/rendering bug 同時進場.
2. 再逐層接 ASCII simulator, Immediate-mode UI, command dump 與 golden image.
3. Pixel-Renderer 的價值是把 application state 一路追到 pixels, 但每一層仍要能獨立驗證.

---

## Part II. Architecture Reference Map

前半部已把第一條主線從 hardcoded Panel 推導到 navigation 與 lifecycle. 以下 reference map 擴大到 Modal, Popup scheduler, business separation, async 與 Pixel-Renderer integration. 編號重新從 1 開始, 方便未來當查表使用.

## 1. 核心問題不是「怎麼開一個 Panel」

小型 UI 只有一個 button 時, 最直接的 code 完全足夠:

```cpp
if (buttonClicked) {
    settingsVisible = true;
}
```

問題出現在 UI flow 開始形成 graph 之後:

```text
Main Menu
  +-- Settings
  +-- Shop
  |     +-- Item Detail
  |     +-- Buy Confirm
  |     +-- Insufficient Money
  +-- Inventory
  |     +-- Item Detail
  +-- Login Reward
  +-- Mail

Gameplay
  +-- HUD
  +-- Pause
  +-- Tutorial Overlay
  +-- Network Error
  +-- Toast
```

此時真正要回答的是:

```text
現在使用者在哪裡?
Back 應該回哪裡?
哪些東西可同時存在?
誰能接收 input?
關閉後要 destroy, cache, 還是保留 state?
多個 popup 同時要求顯示時, 誰先?
async request 完成時, 原本的 UI 還存在嗎?
UI 顯示資料與 business rule 各由誰負責?
最後這些 state 如何變成 widget 與 draw commands?
```

「彈窗管理」只是這個領域最容易被看見的一小塊.

---

## 2. 先把六個不同 layer 分開

最常見的混亂, 是把 UI 的所有問題都叫做 `UIManager`.

```text
Product Flow
  Shop -> Product -> Confirm -> Reward 的 UX 語義
                         |
                         v
Navigation / Window Policy
  Push, Pop, Replace, PresentModal, Dismiss, priority
                         |
                         v
Presentation Architecture
  View, ViewModel, Presenter, Service, Store, Use Case
                         |
                         v
Widget Framework
  Button, Text, layout, focus, event, animation, style
                         |
                         v
UI Rendering
  DrawRect, DrawText, clip, alpha blend, UiCommandList
                         |
                         v
Platform / Display
  input pump, Win32/SDL window, framebuffer present
```

它們回答的是不同問題:

| Layer | 核心問題 | 典型 object / API |
|---|---|---|
| Product Flow | 使用者應該經過哪些步驟? | flow spec, UX state |
| Navigation | 現在在哪裡, Back 回哪? | `Navigator`, route stack |
| Window Policy | 誰可以蓋住誰, 誰先顯示? | modal layer, popup scheduler |
| Presentation | UI data 與 business logic 如何分離? | ViewModel, Presenter, Service |
| Widget Framework | Button 如何互動與 layout? | widget tree, `UiContext` |
| UI Rendering | widget 最後如何變成 pixels? | draw list, software backend |
| Platform | pixels 如何出現在 OS window? | `DisplayBackend` |

一個專案可以使用 Immediate-mode UI, 但 navigation 仍然是 stack. 也可以使用 retained widget tree, 但 presentation state 採 reducer. 這些不是互斥套裝, 而是不同 axis 上的選擇.

---

## 3. 歷史脈絡: 不是一條「新架構淘汰舊架構」的直線

### 3.1 Event loop 與 retained object tree

傳統 desktop GUI 通常保存一棵 window/widget object tree:

```text
Window
  +-- MenuBar
  +-- Panel
        +-- Label
        +-- Button
```

OS 或 framework 收到 mouse/keyboard event 後, 做 hit test, dispatch event, 改變 widget/model state, 再 invalidation/repaint.

這個模型很適合長生命週期的 form, editor 與 desktop controls. 代價是 application state 與 widget state 可能重複, 需要 binding, observer, event subscription 或手動同步.

### 3.2 MVC 與 presentation separation

1979 年 Trygve Reenskaug 的 MVC note 已經把問題拆成 model knowledge, visual representation, 以及 user/controller coordination. 後來的 MVC, MVP, MVVM 實作彼此差異很大, 但持續追問同一件事:

> View 應該知道多少 domain 與 input coordination?

這條歷史線帶出 data binding, Presenter, ViewModel, Application Service 等做法. 它不是為了讓 class 數量變多, 而是為了避免 presentation code 同時承擔 data access, business rule 與 navigation topology.

### 3.3 Navigation history 成為 first-class state

Browser, desktop wizard, mobile application 與 game menu 都會遇到:

```text
A -> B -> C -> Back
```

成熟 framework 通常讓 `Navigator` 或 `NavController` 擁有 destination history. Android 的 official Navigation 文件也明確把 back stack 放在 `NavController`, 而不是個別 screen 裡.

### 3.4 Declarative / state-driven UI

Elm 類型的架構把互動程式壓縮成三件事:

```text
Model  = application state
View   = state -> UI
Update = (state, message) -> next state
```

React 等系統也強調 state 必須跨 render 保留, state update 再觸發 render. 這讓「UI 是 state 的 projection」變得更普及.

### 3.5 Immediate-mode UI 在 game/tooling 世界的價值

Immediate-mode UI 把重要 application data 留在 caller, 每 frame 重新描述 widget. 但它不代表 library 不能保存 ID, focus, window state, cache 或 draw list. Dear ImGui 也會輸出 vertex buffers 與 command lists, 而不是 widget call 當下直接亂打 GPU draw call.

現代 engine 常同時使用:

```text
retained runtime/game UI
  +
immediate-mode debug/editor tooling
  +
explicit navigation/window policy
```

所以真正重要的不是站隊 `retained` 或 `immediate`, 而是逐項回答:

```text
source of truth 在哪?
誰擁有 history?
誰擁有 widget identity?
誰擁有 panel instance?
誰做 input dispatch?
誰產生 draw data?
誰把 draw data rasterize?
```

---

## 4. Object / Type 地圖

在開始寫 `UIManager` 之前, 先把 object 類型分清楚.

| Object | 表示什麼 | 不應自動承擔什麼 |
|---|---|---|
| `ScreenId` | destination 的種類 | instance identity, arguments |
| `RouteEntry` | 一次具體 navigation entry | Panel instance 本身 |
| `Panel` / `View` | 顯示與互動單元 | 整個 app history |
| `Navigator` | screen history 與 transition intent | shop/payment rule |
| `ModalEntry` | 目前阻斷下層互動的 presentation | 任意 background job |
| `PopupRequest` | 想被顯示的 request | 已經可見的 Panel instance |
| `PopupScheduler` | 排序, dedupe, interrupt policy | widget layout |
| `ViewModel` | UI-ready representation | domain source of truth |
| `ApplicationService` | use case coordination | widget API |
| `UiContext` | widget ID, focus, layout, per-frame draw list | product flow |
| `UiCommandList` | 可執行的 UI drawing snapshot | navigation history |
| `DisplayBackend` | input/window/present | UI business semantics |

一個特別重要的 type distinction:

```text
ScreenId::ItemDetail

不等於

RouteEntry {
  instance = 42,
  screen = ItemDetail,
  args = ItemDetailArgs{ itemId = 123 }
}
```

同一種 screen 可以在 history 中出現多次, 參數與 instance identity 都不同.

---

## 5. Naive 版本為什麼一開始完全合理

```cpp
class ShopPanel {
public:
    void OnItemClicked(ItemId id) {
        Hide();
        itemDetailPanel->Show(id);
    }
};

class ItemDetailPanel {
public:
    void OnBackClicked() {
        Hide();
        shopPanel->Show();
    }
};
```

### Dry Run A: 只有一條路徑

```text
Initial:
  MainMenu = visible
  Shop     = hidden
  Detail   = hidden

Click Shop:
  MainMenu.Hide()
  Shop.Show()

Click Sword:
  Shop.Hide()
  Detail.Show(Sword)

Click Back:
  Detail.Hide()
  Shop.Show()
```

它沒有 bug. 因此不要因為看過 pattern, 就在兩個 screen 的 prototype 先造十個 manager.

真正的 architecture pressure 是 reuse:

```text
MainMenu -> Shop      -> ItemDetail
MainMenu -> Inventory -> ItemDetail
Mail                 -> ItemDetail
Crafting             -> ItemDetail
```

如果 `ItemDetail` 寫死 `Back -> Shop`, 從 Inventory 進去就錯. 最自然的補丁是:

```cpp
enum class DetailSource {
    Shop,
    Inventory,
    Mail,
    Crafting,
    // keeps growing...
};
```

這個補丁的問題不只是 `switch` 很長. 更深層的問題是:

```text
ItemDetail 的 semantic responsibility:
  顯示某個 Item.

它被迫知道的 global topology:
  Shop, Inventory, Mail, Crafting, Auction...
```

`previous destination` 不是 ItemDetail 的 property. 它是 navigation history 的 property.

---

## 6. 從失敗推導 Navigation Stack

### 6.1 Call stack 類比有用, 但不是完全同構

直覺上可以把 UI stack 想成 call stack:

```text
Call stack: A calls B calls C, C returns to B.
UI stack:   A pushes B pushes C, C pops back to B.
```

這個類比幫助理解「C 不需要知道 B」. 但兩者不能完全等同:

```text
function call stack:
  return address, local variables, stack frame 由 language/runtime 定義.

UI navigation stack:
  route arguments, saved view state, panel lifetime, animation,
  deep link, multiple back stacks, modal policy 都由 application 定義.
```

因此它是 mental model, 不是 implementation proof.

### 6.2 抽出 Navigator 解掉的是 information ownership

```text
Before:
  ItemDetail knows Shop and Inventory.

After:
  ItemDetail emits Back intent.
  Navigator owns previous destination.
```

這主要是 separation of concerns 與 dependency removal. 只有當 high-level policy 與 low-level detail 透過 abstraction 重新反轉依賴時, 才能更精確地談 Dependency Inversion Principle. 不能只因為用了 stack 就自動叫 DIP.

### 6.3 最小 state

```cpp
struct RouteEntry {
    RouteInstanceId instanceId;
    ScreenId screen;
    ScreenArgs args;
};

struct NavigationState {
    std::vector<RouteEntry> screens;
};
```

Stack 應優先保存 route state, 而不是只保存 `Panel*`:

```text
[ MainMenu,
  Shop(category = Weapon),
  ItemDetail(itemId = 123) ]
```

原因是 route 描述「使用者在哪裡」, Panel pointer 只描述「某個 view instance 在哪個 address」.

### 6.4 Push / Pop / Replace

```text
Push(B)
  [A] -> [A, B]

Pop()
  [A, B] -> [A]

Replace(C)
  [A, B] -> [A, C]
```

它們表達的是 UX history semantics:

| Operation | 意義 | 例子 |
|---|---|---|
| `Push` | 新 destination 可透過 Back 回來 | Shop -> ItemDetail |
| `Pop` | 回到 previous destination | ItemDetail -> Shop |
| `Replace` | current destination 不應再出現在 Back history | Login -> Lobby |
| `PopTo` | 一次退出多層 flow | Checkout -> Shop |
| `Reset` | 建立新的 root history | Logout -> Login |

`OpenPanel()` 只說「顯示」, 沒有完整表達 history semantics.

### 6.5 Stack invariant

至少要先定義:

```text
1. `screens.back()` 是 current screen.
2. root screen 是否允許被 pop 必須明確.
3. 每個 RouteEntry 有唯一 instance identity.
4. mutation 後 stack 不能落入沒有可顯示 root 的非法狀態.
5. transition 期間是否允許再次 navigation 必須明確.
```

Android back stack 文件特別提醒: 如果把最後一個 destination pop 掉, current destination 可能變成空. 自製系統也必須明確處理這個 boundary.

### 6.6 Stack 不是所有 navigation 的唯一模型

Stack 適合 nested drill-down 與 LIFO Back:

```text
Main -> Shop -> Detail -> Back
```

其他 flow 可能需要不同 state model:

| Flow | 較自然的模型 |
|---|---|
| Bottom tabs | 每個 tab 一個 back stack, 外加 selected tab |
| Fixed checkout wizard | explicit state machine / guarded transitions |
| Desktop multi-window | window set + z-order, 不只是單 stack |
| Breadcrumb hierarchy | tree path + current node |
| Deep link | 從 route graph 重建一條合法 history |
| Free-form editor panels | dock tree / workspace layout |

`Navigation Graph` 可以定義哪些 transition 合法, `Navigation Stack` 記錄使用者實際走過的 history. Graph 與 stack 不是互斥替代品.

---

## 7. `Close()` 不等於 `Pop()`

這兩個 operation 分屬不同 layer:

```text
Navigation Layer
  Push / Pop / Replace
  modifies history

Panel Lifecycle
  Open / Enter / Suspend / Resume / Exit / Close
  modifies view instance state
```

假設 history 是:

```text
[MainMenu, Shop, ItemDetail]
```

只執行:

```cpp
detailPanel.Close();
```

可能造成:

```text
navigation current = ItemDetail
visible panel      = Shop or nothing
```

資料與畫面不一致.

比較合理的 coordination 是:

```cpp
void Navigator::Pop() {
    // concept only
    exitCurrentRoute();
    history.pop_back();
    resumeCurrentRoute();
}
```

但 `Pop()` 內是否真的 `Destroy()` Panel, 仍然是另一個 instance-lifetime policy.

更精確的 vocabulary 可以是:

```text
DismissModal()
  modifies modal presentation state.

CloseView()
  runs view lifecycle and hides/removes an instance.

DestroyView()
  releases the instance and owned resources.
```

避免所有事情都叫 `Close()`.

---

## 8. 一個 Screen Stack 為什麼不夠

案例:

```text
Shop -> ItemDetail -> Delete Confirm
```

`Delete Confirm` 與 `ItemDetail` 的關係不是「新 page 取代舊 page」. 使用者仍然需要看見背景 Detail, 但不能操作它.

```text
+--------------------------------+
| ItemDetail                     |
|                                |
|      +------------------+      |
|      | Delete item?     |      |
|      | [Yes]      [No]  |      |
|      +------------------+      |
+--------------------------------+
```

如果把所有東西都塞進同一個 stack, 會把不同語義混在一起:

```text
Screen navigation:
  改變使用者所在 destination.

Modal presentation:
  暫時阻斷下層互動, 完成一個局部決策.
```

一個比較可用的 layer model:

```text
Top
  Toast Layer          non-blocking, short-lived notification
  System Overlay       loading, tutorial mask, reconnecting
  Modal Layer          confirm, error, reward, choice
  Screen Layer         main, shop, inventory, detail
  HUD Layer            gameplay information
  World                scene/gameplay
Bottom
```

這不是所有專案唯一正確的分類. 重點是每個 layer 必須回答:

```text
是否保留底下內容可見?
是否阻斷 pointer / keyboard / gamepad input?
是否影響 Back handling?
是否允許多個 instance?
是否參與 history?
是否能被 higher-priority request interrupt?
```

### 8.1 各類型的 default semantics

| Type | 底下可見 | 阻斷 input | 通常進 history | 常見資料結構 |
|---|---:|---:|---:|---|
| Screen | 視設計 | 是 | 是 | stack / graph |
| Modal | 是 | 是 | 局部 history | stack or single slot |
| Overlay | 是 | 視用途 | 通常否 | explicit state |
| Toast | 是 | 否 | 否 | queue |
| Tooltip | 是 | 否 | 否 | derived transient state |

不要因為東西浮在上面就全部叫 popup.

### 8.2 Modal 如何把結果交回 caller

Confirm modal 不應直接知道是 Shop, Inventory 還是 Editor 呼叫它. 它只產生 typed result:

```cpp
enum class ConfirmResult {
    Confirmed,
    Cancelled,
};
```

常見交接方式:

```text
callback scoped to caller lifetime
Future / Promise<ConfirmResult>
UiAction: DeleteConfirmed(itemId)
modal result event with owner token
```

不論選哪一種, 都要定義 caller 已離開時的 cancellation semantics. `Future` 讓 code 看似線性, 但不會自動解決 route 被 pop, request timeout 或 modal 被 higher-priority system dialog 取代的問題.

---

## 9. Stack, Queue, Priority Scheduler 解的是不同問題

### 9.1 Stack

回答:

> 關掉 current 後, 應該回到哪一層?

```text
Main -> Shop -> Detail
```

### 9.2 Queue

回答:

> 多個 request 都想顯示, 誰先?

```text
Daily Reward
Achievement
Event Notice
```

### 9.3 Priority scheduler

回答:

> 哪些 request 可插隊, 中斷, 合併, 過期或取消?

```text
Network disconnected: high priority, maybe preemptive
Purchase confirm:     user-blocking, exclusive
Daily reward:         normal priority
Achievement:          low priority, non-blocking
```

只寫一個 `std::priority_queue` 還不夠. Production policy 常需要:

```text
priority
FIFO within same priority
dedupe key
exclusive group
interruptible / non-interruptible
preempt and resume / preempt and cancel
time-to-live
cooldown
maximum visible count
starvation prevention
source / owner token
```

例如只按 priority 永遠排序, 低 priority reward 可能永遠拿不到機會. 因此 scheduler 需要 fairness 或 aging policy, 或乾脆把 critical error 與 reward 分到不同 presentation channel.

### 9.4 Request 不等於 visible instance

```cpp
struct PopupRequest {
    PopupRequestId id;
    PopupKind kind;
    PopupPriority priority;
    DedupeKey dedupeKey;
    PopupPayload payload;
    bool interruptible;
};
```

Request 進 queue 時還沒有 Panel instance. Scheduler 選中它後, 才產生 active modal/view. 把兩者分開, 才能安全取消未顯示 request, 做 dedupe, 或保存 scheduling trace.

---

## 10. Input Routing 才是 Modal 真正的硬邊界

Modal 不只是畫一個半透明黑框. 它最重要的語義是 input capture.

典型 dispatch order:

```text
OS / DisplayBackend input
  -> normalize to InputState / UI event
  -> highest blocking layer
  -> topmost modal
  -> current screen
  -> HUD
  -> game world
```

如果 topmost modal 已經 consume pointer event, event 不應繼續穿透到 `ItemDetail` 的 Delete button 或 world click.

需要分開考慮:

```text
pointer hover
pointer press/release
pointer capture during drag
keyboard focus
text input / IME
gamepad navigation focus
Back / Escape
accessibility focus
```

### Back / Escape chain

一個可解釋的 default policy:

```text
1. 如果有 non-dismissible system overlay, ignore or show reason.
2. 如果有 modal, ask modal to handle Back.
3. 如果有 temporary overlay, close it.
4. 否則 pop current screen.
5. 如果已在 root, hand back to application/OS quit policy.
```

### Pause 不等於 Modal

顯示 modal 時是否暫停 gameplay 是 product policy:

```text
single-player pause menu:
  may pause simulation.

online multiplayer dialog:
  cannot pause authoritative world.

inventory overlay:
  may block local control but world keeps updating.
```

不要讓 `PresentModal()` 偷偷寫死 `timeScale = 0`.

---

## 11. Navigation, Lifecycle, Instance Management 是三個正交 concern

```text
Navigation
  Push, Pop, Replace, route history

Lifecycle
  Create, Load, Enter, Active, Suspend, Resume, Exit, Close

Instance Management
  instantiate, cache, pool, destroy, resource ownership
```

它們會合作, 但不是同一件事.

### 11.1 Lifecycle state machine

```text
Unloaded
   |
   v
Loading --cancel/fail--> Closed
   |
   v
Entering
   |
   v
Active <----> Suspended
   |
   v
Exiting
   |
   v
Closed
   |
   +-- cache --> Inactive instance
   +-- destroy -> Destroyed
```

每個 transition 應明確定義:

```text
can receive input?
can render?
can start async work?
who cancels subscriptions?
what state survives?
```

### 11.2 Destroy, Cache, Pool

| Policy | 適合 | 優點 | 代價 |
|---|---|---|---|
| Destroy | 低頻, memory-heavy screen | memory 低, state 乾淨 | reopen/load 慢 |
| Cache | 常開啟的 fixed screen | reopen 快, 保留 local view state | memory 高, stale state risk |
| Pool | 大量同型短生命 instance | 降低 allocate/instantiate cost | reset contract 複雜 |

`Inventory Item Cell x 1000` 很適合 pool. `Settings Screen` 常適合 cache. 不要因為 pool 聽起來高效, 就把每個 full screen 都丟進 pool.

### 11.3 Suspend 時保存什麼

`Shop -> Detail -> Back` 常希望保留:

```text
scroll position
selected category
filter text
focused item
temporary animation progress, maybe not
```

這些可以由:

```text
Panel instance cache
Route-local saved state
ScreenStateStore keyed by RouteInstanceId
```

保存. 選擇哪一個取決於 memory, restore cost 與 determinism. 不要把「保存 view state」和「保留整個 Panel instance」視為同一件事.

### 11.4 `Open / Refresh / Close` 應該如何理解

在 retained Panel framework 中, 這三個 method 可以作為最小 lifecycle API:

```text
Open(args)
  bind initial presentation state, subscribe, start enter transition.

Refresh(viewModel)
  update visible controls from new presentation data.

Close()
  stop input/animation, unsubscribe, cancel owned work, hide or release.
```

但 `Refresh()` 不是 navigation operation, 也不一定是所有 framework 都需要的 public method. 在 declarative / Immediate-mode UI 中, state 改變後下一次 declaration/render 自然產生新 UI, 不一定手動呼叫 `panel.Refresh()`.

所以真正的 invariant 是:

```text
visible UI must be a current projection of presentation state.
```

而不是「每個 Panel class 一定要有名為 Refresh 的 method」.

---

## 12. UI Data 與 Business Logic 的邊界

最危險的版本:

```cpp
void ShopPanel::OnBuyClicked(ItemId id) {
    Player& player = GameManager::Instance().player();

    if (player.gold >= GetPrice(id)) {
        player.gold -= GetPrice(id);
        Inventory::Instance().Add(id);
        goldLabel.SetText(...);
        inventoryPanel.Refresh(...);
    }
}
```

它混合了:

```text
input handling
price rule
wallet mutation
inventory mutation
presentation formatting
cross-panel refresh
```

一個較乾淨的資料流:

```text
User Event
  -> View emits intent
  -> Application Service / Use Case validates and executes
  -> Domain state changes
  -> Presentation state / ViewModel is derived
  -> View renders UI-ready data
```

例如:

```cpp
void ShopView::OnBuyClicked(ItemId id) {
    actions.push(BuyItemRequested{id});
}

BuyResult ShopService::Buy(ItemId id) {
    // inventory, wallet, pricing, transaction rule
}
```

### 12.1 ViewModel 的工作

Domain data:

```cpp
struct Item {
    ItemId id;
    Money price;
    Timestamp expireAt;
    Rarity rarity;
};
```

UI-ready data:

```cpp
struct ItemViewModel {
    std::string title;
    std::string priceText;
    std::string expireText;
    IconId icon;
    FrameStyle frameStyle;
    bool canBuy;
};
```

ViewModel 做的是 domain representation 到 presentation representation 的 mapping. 它不應偷偷成為第二份 authoritative inventory.

### 12.2 不要把 God UIManager 變成 God Service

錯誤演化:

```text
UIManager knows everything
  -> move everything into GameService
  -> GameService knows everything
```

Service 應按 use case/domain responsibility 切分:

```text
ShopService
InventoryService
RewardService
AuthenticationService
```

或更小的 use case object:

```text
BuyItem
ClaimReward
EquipItem
```

是否需要 MVVM, Presenter, Store 或 Service class, 應由複雜度壓力決定. 小工具允許 code-behind; 先保留可測試邊界比套完整 pattern 更重要.

---

## 13. 把 UI 看成 State Transition System

一個很有力量的形式化方式:

```text
View = Render(UIState)

UIState(next) = Reduce(UIState(current), Event)
```

概念 state:

```cpp
struct UiState {
    NavigationState navigation;
    ModalState modal;
    OverlayState overlay;
    ToastState toasts;
    ShopPresentationState shop;
};
```

Action:

```cpp
using UiAction = std::variant<
    PushScreen,
    PopScreen,
    ReplaceScreen,
    PresentModal,
    DismissModal,
    EnqueuePopup,
    BuyItemRequested,
    BuyItemCompleted
>;
```

Reducer-like transition:

```cpp
void Reduce(UiState& state, const UiAction& action);
```

這種做法的價值不是追求 functional purity. 它讓 navigation mutation 有單一入口, 可以 trace, test, replay, 並避免 callback 中途重入 manager.

### 13.1 Reentrancy 問題

危險案例:

```text
Modal.OnClose()
  -> emits OpenReward()
  -> manager mutates modal container
  -> current Close() 還在 iterator/callback 中
  -> use-after-free or invalid iterator
```

一個常見解法是 command/action queue:

```text
input callbacks emit UiAction
  -> current dispatch finishes
  -> UI system processes actions at a stable phase
  -> compute next state
  -> run lifecycle diff
```

這和 renderer 的 deferred command thinking 有相似處, 但 navigation action 不是 draw command. 兩者的 execution phase 與 ownership 不能混在同一個 queue.

---

## 14. Async 才是 Production UI 最常爆炸的地方

### 14.1 典型 race

```text
Frame 10:
  Open ItemDetail(123)
  start LoadItem(123)

Frame 11:
  user presses Back
  ItemDetail is popped and destroyed

Frame 30:
  LoadItem callback returns
  callback writes detailPanel->title
```

結果可能是 use-after-free, stale UI update, 或錯把 item 123 的結果寫到已重用的 item 456 Panel.

### 14.2 至少需要兩種 identity

```text
RouteInstanceId
  identifies this specific navigation entry.

RequestId / CancellationToken
  identifies this async operation.
```

Callback 完成時檢查:

```text
route still exists?
route instance still matches?
request is still current?
operation was cancelled?
result belongs to current arguments?
```

### 14.3 Async presentation state

不要只用 `bool loading`:

```cpp
using LoadState = std::variant<
    Idle,
    Loading,
    Loaded<ItemDetailViewModel>,
    Failed<UiError>
>;
```

它能避免這些非法組合:

```text
loading = true
errorVisible = true
dataValid = true
```

### 14.4 重複 submit

Purchase Confirm 連按兩次不是 UI 小問題. 可能變成兩次 payment/use case. 需要:

```text
disable or debounce submit
idempotency key at use-case/network boundary
Submitting state
explicit retry policy
```

UI 防連點只能改善 interaction, 不能取代 domain/network idempotency.

---

## 15. Full Dry Run: Shop -> Detail -> Delete Confirm -> Network Error

現在把前面的概念放在同一個案例.

### Step 0: Main root

```text
Screen Stack: [MainMenu]
Modal Stack:  []
Popup Queue:  []
Input Owner:  MainMenu
```

### Step 1: Push Shop

Action:

```text
PushScreen(Shop(category = Weapon))
```

State:

```text
Screen Stack:
  [MainMenu,
   Shop(category = Weapon)]

Lifecycle:
  MainMenu -> Suspended
  Shop     -> Entering -> Active

Input Owner:
  Shop
```

### Step 2: Push ItemDetail

Action:

```text
PushScreen(ItemDetail(itemId = Sword123))
```

State:

```text
Screen Stack:
  [MainMenu,
   Shop(category = Weapon),
   ItemDetail(itemId = Sword123)]

Shop:
  Suspended, scroll/category state preserved

ItemDetail:
  Active, starts LoadItem(Sword123)
```

### Step 3: Present Delete Confirm

Action:

```text
PresentModal(DeleteConfirm(itemId = Sword123))
```

State:

```text
Screen Stack:
  [MainMenu, Shop, ItemDetail]

Modal Stack:
  [DeleteConfirm(Sword123)]

Visible:
  ItemDetail + dim layer + DeleteConfirm

Input Owner:
  DeleteConfirm
```

ItemDetail 仍可 render, 但不應接收 pointer/gamepad action.

### Step 4: User confirms, network request fails

Mechanical solution 可能是:

```text
Modal Stack:
  [DeleteConfirm, NetworkError]
```

Dismiss `NetworkError` 後會回到 `DeleteConfirm`. 這可能讓使用者再次按 Delete, 造成重複 request.

Product semantics 可能更適合:

```text
Option A:
  ReplaceModal(DeleteConfirm -> DeleteError)

Option B:
  DeleteConfirm state: Confirming -> Failed(error)

Option C:
  Dismiss Confirm, enqueue a global NetworkError
```

三者不是純技術選擇:

| Policy | 適合情況 | Back/Dismiss 後 |
|---|---|---|
| Stack error above confirm | error 是短暫說明, 可安全回原 decision | 回 Confirm |
| Replace confirm with error | 原 action 已結束, 不該自動重試 | 回 Detail |
| Confirm owns Failed state | retry 是同一個 use case 的一部分 | 留在同一 Modal |
| Global error channel | connection issue 影響整個 app | 由 system overlay policy 決定 |

這個例子說明: `Modal Stack` 只提供 mechanism, 不能替 product flow 做 decision.

### Step 5: Dismiss error and Pop Detail

假設選 `ReplaceModal(DeleteError)`, 使用者 dismiss 後:

```text
Modal Stack: []
Input Owner: ItemDetail
```

再按 Back:

```text
PopScreen()

Before:
  [MainMenu, Shop, ItemDetail]

After:
  [MainMenu, Shop]

Lifecycle:
  ItemDetail -> Exiting -> Closed
  Shop       -> Resume -> Active
```

ItemDetail 不知道 previous 是 Shop. `Navigator` 只依 history 恢復 current route.

### Step 6: 同時收到多個 popup request

```text
Queue:
  NetworkDisconnected priority=100 exclusive=system
  DailyReward         priority=30  dedupe=daily-2026-08-21
  Achievement         priority=10  dedupe=achievement-42
```

Scheduler 可能決定:

```text
1. 顯示 NetworkDisconnected system overlay.
2. 暫停 normal modal scheduling.
3. connection restored 後顯示 DailyReward.
4. Achievement 走 non-blocking toast channel, 不與 modal 競爭.
```

這比把所有 request 丟到同一個 priority queue 更符合語義.

---

## 16. 幾種 Architecture 方案比較

### Option A: Panel 直接 reference 彼此

```text
適合:
  2-3 個固定畫面的 prototype.

優點:
  最少 code, flow 直接.

失敗點:
  reuse destination, deep link, multiple caller, testability.
```

### Option B: 單一 UIManager + PanelId

```text
適合:
  小型 game menu, flow 尚簡單.

優點:
  集中 create/show/hide, 比互相 reference 好追.

失敗點:
  很容易混入 navigation, cache, business, resource loading.
```

### Option C: Navigator + Layer State + Panel Registry

```text
適合:
  有 screen history, modal, overlay, reusable destination.

元件:
  Navigator
  Screen Stack
  Modal State/Stack
  Popup Scheduler
  Panel Registry / Factory
  lifecycle coordinator

代價:
  transition 與 ownership contract 必須寫清楚.
```

### Option D: Store / Reducer / State-driven UI

```text
適合:
  複雜 async flow, replay/debug/test 很重要.

優點:
  state transition 可 trace, unit test, deterministic.

代價:
  action/reducer boilerplate, effect boundary 需要設計.
```

### Option E: MVVM + Data Binding

```text
適合:
  data-heavy form, editor, retained UI framework.

優點:
  view/data decoupling, binding, designer/tool support.

代價:
  hidden update flow, binding debug, ViewModel inflation.
```

### Option F: Immediate-mode UI + explicit app state

```text
適合:
  debug panel, renderer tools, rapidly changing inspector.

優點:
  application data is easy to keep as source of truth,
  widget declaration and draw list are direct.

代價:
  complex navigation, accessibility, animation, text input,
  large product UI still need explicit subsystems.
```

沒有一個 option 在所有 scale 都最好. 推薦 default 是「先使用能清楚表達目前 pressure 的最小方案, 但把 state ownership 寫明」.

---

## 17. 一個可測試的 C++ Skeleton

下面是 architecture sketch, 不是要求 Pixel-Renderer 現在實作的 API.

```cpp
enum class ScreenId {
    MainMenu,
    Shop,
    Inventory,
    ItemDetail,
};

enum class ModalId {
    DeleteConfirm,
    DeleteError,
    NetworkError,
};

using ScreenArgs = std::variant<
    NoArgs,
    ShopArgs,
    ItemDetailArgs
>;

using ModalArgs = std::variant<
    DeleteConfirmArgs,
    ErrorArgs
>;

struct RouteEntry {
    RouteInstanceId instanceId;
    ScreenId id;
    ScreenArgs args;
};

struct ModalEntry {
    ModalInstanceId instanceId;
    ModalId id;
    ModalArgs args;
    ModalPolicy policy;
};

struct UiNavigationState {
    std::vector<RouteEntry> screens;
    std::vector<ModalEntry> modals;
    std::deque<PopupRequest> pendingPopups;
};
```

Navigation command:

```cpp
using NavigationAction = std::variant<
    PushScreen,
    PopScreen,
    ReplaceScreen,
    PopToScreen,
    PresentModal,
    ReplaceModal,
    DismissModal,
    EnqueuePopup,
    CancelPopup
>;
```

Processing:

```text
Input / domain effect completion
  -> emits NavigationAction or UiAction
  -> reduce state at stable phase
  -> validate invariants
  -> compute lifecycle diff
  -> create/cache/resume/suspend views
  -> route input to highest active layer
  -> declare widgets / build draw list
  -> render UI commands
```

不要用 `std::any` 作為第一選擇. `std::variant` 或 typed route constructor 能讓錯誤較早被 compiler 發現. 如果 route 數量很大, 可以再研究 type erasure, serialization 與 registry.

---

## 18. Testing 與 Observability

UI architecture 如果只能靠眼睛點, 很難處理 race 與 history bug.

### 18.1 Navigation unit tests

```text
initial root is valid
Push adds one route
Pop returns previous route
Replace does not preserve replaced route
PopTo removes expected range
root cannot become accidentally empty
same ScreenId can have distinct RouteInstanceId
```

### 18.2 Layer / input tests

```text
top modal receives Back before screen
pointer does not leak through modal barrier
toast does not block screen input
non-dismissible overlay rejects Back
focus returns to previous owner after dismiss
```

### 18.3 Scheduler tests

```text
same dedupe key is merged or rejected
FIFO holds within same priority
critical request preempts only interruptible modal
expired request is not shown
low-priority request does not starve forever
cancelled owner removes its pending requests
```

### 18.4 Async race tests

```text
route popped before request completion
same Panel reused for a different item
double submit
out-of-order responses
cancel during Entering/Exiting transition
```

### 18.5 Trace

每個 transition 可以輸出 compact trace:

```text
[UI] action=PushScreen(ItemDetail:123)
     screens=[Main, Shop, Detail#42]
     modals=[]
     focus=Detail#42

[UI] action=PresentModal(DeleteConfirm)
     screens=[Main, Shop, Detail#42]
     modals=[DeleteConfirm#7]
     focus=DeleteConfirm#7
```

這和 Pixel-Renderer 的 pipeline trace 思想一致: 不只看最後畫面, 還要看中間 state transition.

### 18.6 Rendering verification

如果 UI frontend 最後產生 deterministic draw list, 可以分層測試:

```text
Input fixture
  -> UiAction / navigation state
  -> widget declaration
  -> UiCommandList
  -> UI software renderer
  -> Framebuffer
  -> command dump / golden image
```

Navigation correctness 與 pixel correctness 應分開測. Screen stack 對了, 不代表 clip/alpha/text 對了; golden image 對了, 也不代表 Back history 正確.

---

## 19. 與 Pixel-Renderer 的正確接法

Pixel-Renderer 既有 stable direction 聚焦在可信 raster pipeline, owned `Framebuffer`, `DisplayBackend`, renderer frontend/backend separation, 以及未來的 Immediate-mode debug UI. 這份 note 討論的是 UI stack 更上層的 application/navigation 問題.

完整 vertical stack 可以畫成:

```text
Domain / Renderer State
  -> Application Service / UiAction
  -> Navigation + Modal + Overlay State
  -> Screen / Panel declaration
  -> UiContext
  -> UiCommandList
  -> UI Software Renderer
  -> RenderDevice
  -> owned Framebuffer
  -> DisplayBackend
  -> OS Window
```

其中 Pixel-Renderer 現有文件已較深入處理的是下半部:

```text
UiContext
UiCommandList
clip rect
text / font atlas
alpha blend
software backend
owned Framebuffer
DisplayBackend
```

本篇補的是上半部:

```text
application state
navigation history
modal/window policy
popup scheduling
presentation boundary
async lifetime
input ownership
```

### 19.1 現階段不要直接做 full UIManager

`docs/PROJECT_MAP.md` 與 `docs/ARCHITECTURE.md` 的 current priority 仍是 trusted screen-space raster core. 因此這份 note 不應導出:

```text
現在立刻在 main 建立完整 production UI framework.
```

比較合理的學習順序:

```text
1. 先完成 raster correctness 與 verification.
2. owned Framebuffer / DisplayBackend boundary 穩定後,
   做最小 Immediate-mode debug UI.
3. 需要研究 navigation 時, 先做 headless state experiment,
   不必先接 renderer 或 Win32.
4. 只有當 actual tool/product flow 出現 modal/scheduler pressure,
   才把對應 subsystem 接入正式 architecture.
```

### 19.2 一個可隔離的 future experiment

如果之後要親手驗證本文, 最小實驗可以只有 console tests:

```text
Screen Stack
Modal Stack
UiAction reducer
trace output
no window
no widget framework
no renderer
```

先證明 state transition. 再把 current state 接到 Immediate-mode UI declaration. 最後才讓 `UiCommandList` 經 software backend 寫入 framebuffer.

這能保持問題分層:

```text
navigation bug
widget interaction bug
rendering bug
```

不會在同一幀混成一團.

---

## 20. 常見錯誤清單

### Error 1: Child screen 知道 previous screen

```text
Detail.Back() -> OpenShop()
```

修正方向: `Detail` 只表達 Back intent, history 由 `Navigator` 擁有.

### Error 2: `PanelId` enum 被誤認為 architecture

把所有 panel 放進 enum 只解決 naming/lookup, 沒有解決 history, layer, lifetime, input, async.

### Error 3: `UIManager` 混入 business rule

```text
UIManager::OpenShop()
  checks wallet, event, network, inventory...
```

Navigation 應接收已決定的 intent/result, 不應成為全遊戲規則中心.

### Error 4: 所有浮層都進同一個 Modal Stack

Toast, tooltip, loading, confirm, fatal error 的 input 與 scheduling semantics 不同.

### Error 5: Priority 永遠越大越先

會造成 starvation, 非 interruptible flow 被粗暴打斷, 或 reward 永遠排不到.

### Error 6: `Close()` 同時代表 hide, pop, destroy

API 名稱隱藏三種 state mutation, 很容易產生 inconsistency.

### Error 7: Cache Panel 就等於保存 navigation state

Panel instance, route state, domain data 是不同 owner/lifetime.

### Error 8: Event Bus 解耦一切

全域 string event bus 可能把 compile-time dependency 變成 runtime hidden dependency. Event source, owner, lifetime, unsubscribe policy 仍需明確.

### Error 9: Async callback capture raw Panel pointer

route 被 pop 或 instance 被 reuse 後, callback 可能寫錯 object. 使用 request/route identity 與 cancellation.

### Error 10: Immediate-mode UI 被誤解成沒有 state

Immediate-mode 描述 API/data ownership 風格, 不禁止 library 保存 internal state, cache 或 draw list.

### Error 11: UI navigation action 與 render command 混成同一 queue

它們都可以 deferred, 但 execution phase, failure semantics 與 data lifetime 不同.

### Error 12: Pattern 先於 pressure

兩個固定 panel 不需要 production scheduler. 先找 failure case, 再引入能解該 failure 的最小 abstraction.

---

## 21. Decision Checklist

設計一個 UI flow 前, 依序回答:

```text
1. 這是 Screen, Modal, Overlay, Toast, 還是 Tooltip?
2. 它是否改變 navigation history?
3. Back / Escape 的語義是什麼?
4. 底下 layer 是否可見, 可更新, 可接收 input?
5. state source of truth 在哪?
6. route arguments 是否 typed?
7. instance 要 destroy, cache, 還是 pool?
8. 哪些 local view state 必須恢復?
9. 是否啟動 async operation? 誰取消?
10. 多個 request 同時出現時如何排序, dedupe, interrupt?
11. transition 是否可 trace 與 unit test?
12. widget/draw/pixel verification 要在哪一層做?
```

如果這十二題沒有答案, 加更多 manager class 通常只會把不確定性包起來.

---

## 22. 後續學習路線

### Part 1: Navigation Core

```text
Ch01 naive Panel references
Ch02 Screen Stack and RouteEntry
Ch03 Push / Pop / Replace / PopTo
I-01 headless navigation reducer + trace tests
```

### Part 2: Window Semantics

```text
Ch04 Screen vs Modal vs Overlay vs Toast
Ch05 input routing, focus, Back policy
Ch06 Popup Queue, priority, dedupe, interrupt, fairness
I-02 Shop -> Detail -> Confirm -> Error dry run
```

### Part 3: State and Lifetime

```text
Ch07 lifecycle vs instance management
Ch08 async loading, cancellation, stale result
Ch09 ViewModel, Service, reducer/store tradeoffs
I-03 deterministic race-condition tests
```

### Part 4: Pixel-Renderer Integration

```text
Ch10 application/navigation state -> Immediate-mode declaration
Ch11 UiContext -> UiCommandList
Ch12 software UI backend -> owned Framebuffer
I-04 navigation trace + command dump + golden image
```

這個順序保留原對話最有價值的教學方法:

```text
concrete case
  -> naive model
  -> failure point
  -> deeper mechanism
  -> typed state
  -> code boundary
  -> verification
```

---

## 23. 本篇帶走的三件事

1. `previous screen` 屬於 navigation history, 不屬於 child Panel. `Push / Pop / Replace` 表達的是 UX history semantics, 不是單純 Show/Hide.
2. Screen Stack, Modal state, Popup Queue, input routing, lifecycle, instance management 與 business separation 是不同 concern. 可以由同一 subsystem 協調, 但不能在概念上混為一件事.
3. 對 Pixel-Renderer 而言, navigation 位於 `UiContext` 與 `UiCommandList` 之上. 現在最有價值的是保存這張完整 map, 不提前把 production UI framework 插進 raster baseline.

---

## 24. Project Cross-Links

- [`docs/ARCHITECTURE.md`](../ARCHITECTURE.md): current short/mid/long architecture 與 Debug UI 位置.
- [`docs/PROJECT_MAP.md`](../PROJECT_MAP.md): Rendering Systems Lab 的 UI / Tooling branch.
- [`Embedded OLED UI x Pixel-Renderer 教學紀錄`](2026-05-14-embedded_oled_ui_invalidation_teaching_notes_v2.md): retained/immediate continuum, invalidation, dirty region, compositor layers.
- [`Owned Framebuffer, UI 與 ImGui Software Backend`](2026-05-15-owned-framebuffer-ui-imgui.md): UI frontend, software backend, owned `Framebuffer`, `DisplayBackend` boundary.
- [`I-14 Immediate-mode UI Integration Boundary`](../tutorial-cpp/impl/i14_immediate_mode_ui_integration_boundary.html): `UiContext`, widget state, draw list, font atlas, clip, alpha blend.

---

## 25. References

Primary / official references used to check terminology and boundaries:

- [Trygve Reenskaug, Models-Views-Controllers, 1979](https://zenodo.org/records/3676092)
- [Android Developers, Navigation and the back stack](https://developer.android.com/guide/navigation/backstack)
- [Microsoft Learn, Windows data binding and MVVM](https://learn.microsoft.com/en-us/windows/apps/develop/data-binding/data-binding-and-mvvm)
- [The Elm Architecture](https://guide.elm-lang.org/architecture/)
- [React, State: A Component's Memory](https://react.dev/learn/state-a-components-memory)
- [Unity Manual, UI Toolkit](https://docs.unity3d.com/2023.2/Documentation/Manual/UIElements.html)
- [Dear ImGui, About the IMGUI paradigm](https://github.com/ocornut/imgui/wiki/About-the-IMGUI-paradigm)
- [Dear ImGui repository, How it works](https://github.com/ocornut/imgui)
