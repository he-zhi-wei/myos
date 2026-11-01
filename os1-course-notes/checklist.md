# checklist.md — os1 教学讲义验收清单

> 对应 `spec.md` 第 8 节"验收标准"。每项均已实测并勾选。

## 交付物与结构
- [x] 交付物存在：`os1-course-notes/os1-course-notes.html`（334672 B），单文件、自包含（仅外链 `_shared/` 字体与 JS、`assets/charts.js`）
- [x] 14 个 `<section class="chapter">`，id 依次为 ch1–ch13 + appendix
- [x] 侧栏 14 个 `href="#..."` 与 section id 全匹配，无死锚点（脚本比对：missing = ∅）
- [x] 每章都有"学习目标"元信息条与"自测"块（13 个 `<div class="quiz">` + 39 个 `<details>` 折叠答案）
- [x] 97 张表全部在 `.table-wrap` 内（tables 97 / wraps 97）
- [x] 16 张图全部有 `figcaption`（14 个 `<figure class="figure">` + 2 个 ECharts 图题）

## 篇幅
- [x] 正文汉字总数 ≥ 35000（实测 57512）
- [x] 第 9 / 10 / 11 章各 ≥ 3500 字（实测 4318 / 3952 / 4176）
- [x] 第 1 / 2 / 3 章达到蓝图目标（实测 2752 / 3885 / 3696，目标 2200 / 2800 / 3000）
- [x] 附录 ≥ 2000 字（实测 2073）

## 视觉
- [x] 6 张 Mermaid 图全部渲染为 `<svg>`，0 个 `.mermaid-error`，页面无 "Syntax error" 文本
- [x] 8 张内联 SVG（ch3/4×2/6/8/9/11/12）正常显示
- [x] 2 个 ECharts（柱状图 11/8/4/4、环形图 5 类合计 17）有图形、数据非空
- [x] 浏览器控制台无页面自身报错：唯一一条 `SyntaxError` 来自 IDE 预览注入的 `@vite/client`（该请求为 `net::ERR_ABORTED`），与交付物无关——本页 4 个 script 标签全部成功执行（否则图表与 Mermaid 不可能渲染）

## 内容准确性
- [x] 抽查 8 个常量与源码逐字一致：`0x1BADB002`、`0x03`、`0x1B`、`0x8E`/`0xEE`、`11931`、`0x200000`/`0x201000`/`0x202000`、`8192`、`SYS_MAX 17`
- [x] 已标注"注释/实现不一致"：`fs.h` 4KB vs 8192、`task.h` `task_sleep` vs `task_sleep_ms`、`init.c` 的 `cd` 注释、版本串 0.6.0 vs 0.7.0
- [x] 已注明无调用者的函数：`paging_switch_dir`/`paging_clone_dir`/`elf_load`
- [x] 每个"坑"都有可观察证据（日志行、测试名、输出）
- [x] 产物数字已与当前构建同步：`kernel.bin` 47904 B、`test_kernel.bin` 52108 B、`init.elf` 7068 B、入口 `0x4001c3`

## 响应式
- [x] 375px：正文无横向溢出——无固定像素宽度布局（唯一 `width:52px` 是标题装饰线），表格在 `.table-wrap` 内自滚、代码块 `pre` 自滚，并加了 `overflow-wrap:anywhere` 兜底长标识符
- [x] ≤1100px 侧栏隐藏；≤768px 单列

## 不破坏构建
- [x] `make check` 27/27 通过（并顺带修复了 `src/shell.c` 被注释插入破坏的两处：重复的 `cmd_history` 残块、`uint32_t total` 截断、误插的 `shell.h` 内容）
- [x] `make ring3` PASS（`exec: program exited`）

## 收尾
- [x] 无残留占位符（`grep -c "@@"` = 0）
- [x] `tasks.md` 复选框 171 项全部同步
