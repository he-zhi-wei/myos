# Spec — os1 操作系统教学讲义（HTML，详细版）

## 1. 目标与范围

| 项 | 内容 |
|---|---|
| 交付物 | 一份自包含 HTML 教学讲义，逐层讲解 `/home/hello/os_project/os1` 这个 32 位 x86 教学内核 |
| 读者 | 操作系统课程学生、自学者、准备做课程设计的本科生 |
| 前置知识 | C 语言（指针、结构体、位运算）；了解寄存器与栈；不要求读过 Intel 手册，也不要求会写汇编 |
| 语言 | 简体中文（代码、寄存器、函数名、常量保持英文） |
| 教学法 | 每章固定六段：**学习目标 → 问题背景 → 原理 → 本项目实现（文件/函数/常量/代码）→ 坑与常见误解 → 自测与实验** |
| 深度目标 | 正文不少于 **3.5 万汉字**；13 章平均 2500 字以上，核心章（9/10/11）各 3500 字以上；每章至少 2 段源码摘录、1 张图、3 道自测题、1 个可执行观察步骤 |
| 范围外 | 不修改 `src/`、`include/`、`test/`、`Makefile`、链接脚本；不生成 AI 插画；不做多页拆分 |

## 2. 输出文件（唯一权威清单）

```
/home/hello/os_project/os1/os1-course-notes/
├── os1-course-notes.html        # 交付物（重写脚手架）
├── assets/charts.js             # 全部 ECharts 初始化（IIFE，外部文件）
└── _shared/
    ├── fonts/                   # 已就绪：BricolageGrotesque-{Regular,Bold}.ttf, GeistMono-{Regular,Bold}.ttf
    └── js/                      # 已就绪：echarts.min.js, mermaid.min.js
```

已就绪：脚手架、字体、JS 库。待写：HTML 正文、`charts.js`、渲染验证。

## 3. 设计系统（暖纸学术风）

```css
:root{
  --bg:#FAF8F3; --bg2:#F2EEE4; --ink:#1B1F27; --muted:#5A6070;
  --rule:#DED8C9; --accent:#1E6E5A; --accent2:#B5651D;
  --font:'Bricolage Grotesque','Noto Sans SC',system-ui,sans-serif;
  --font-body:'Noto Sans SC','PingFang SC','Microsoft YaHei',system-ui,sans-serif;
  --font-mono:'Geist Mono','SFMono-Regular',Consolas,monospace;
  --max:960px;
}
```

| 元素 | 规则 |
|---|---|
| 布局 | 左 240px sticky 目录（可独立滚动）+ 右 960px 正文；≤1100px 隐藏侧栏 |
| 章头 | 大号序号（Geist Mono，accent）+ 章标题 + 一行导语 + 元信息条（学习目标/前置/阅读时长） |
| h2 | 2px accent 下划线短线；h3 accent 色 1.15rem；h4 加粗同 ink |
| 正文 | 16px / 1.85，段距 1.1rem，最大行宽 68 字符（中文约 34 字） |
| 代码块 | `--bg2` 底 + 左 3px accent + 1px rule；Geist Mono 13.5px；`overflow-x:auto`；顶部显示文件名（`<span class="file">`） |
| 提示框 | `.note`（原理补充，accent 左边框）、`.warn`（坑/不一致，accent2）、`.lab`（实验卡，虚线上边框）、`.quote`（手册原文/结论引用） |
| 表格 | 仅行线 + 表头下框线；首列 mono；`.table-wrap` 包裹（`overflow:auto` + `max-height:600px`） |
| 强调 | 统一 `<strong>`（Method A），不使用彩色 mark |
| 自测 | 每章末 `.quiz` 卡片：3 道题 + 折叠答案（`<details>` 原生折叠，无 JS） |
| 打印 | 隐藏侧栏与 details 展开控制，章前分页 |

## 4. 讲义整体骨架

```
封面（标题/副标题/版本/读者/日期）
导读（怎么读、三条学习路线、命令总表、全项目地图）
第 1 章  全景：这个内核到底做了什么
第 2 章  从加电到 kernel_main
第 3 章  分段、特权级与 TSS
第 4 章  中断、异常与 PIC
第 5 章  物理内存管理（PMM）
第 6 章  分页与地址空间
第 7 章  时钟、键盘与输出设备
第 8 章  RamFS 与文件描述符
第 9 章  ELF32 加载与 Ring 3 运行
第 10 章 系统调用
第 11 章 任务与调度器
第 12 章 测试、构建与工程化
第 13 章 已知限制与分级实验
附录 A 命令速查  B 系统调用表  C 常量表  D 术语表  E 源码索引  F 调试技巧
```

## 5. 逐章内容蓝图（写作时必须覆盖的小节）

> 每节给出：**要讲清的问题 → 必须出现的代码/常量 → 配图/表格 → 观察方法**。

### 第 1 章 全景（目标 2200 字）

1.1 一句话定位：47872 字节的 `kernel.bin` 能做什么（VGA shell、RamFS、ELF、Ring 3、syscall、单元测试），不能做什么（无真 fork、无磁盘、无网络、8MB 分页窗口）。
1.2 为什么选 32 位 x86 + Multiboot：GRUB 直接进保护模式、无实模式切换代码、适合一学期课程。
1.3 目录与职责表：`src/`(23 个 .c/.asm) / `include/`(18 个头) / `test/`(6 个) / `linker.ld` / `user.ld` / `Makefile`。
1.4 构建产物表：`kernel.bin 47872B`、`build/test_kernel.bin 52044B`、`init.elf 7068B`（入口 `0x4001c3`，VMA `0x400000`）。
1.5 模块依赖图（V1）。
1.6 三条学习路线：只读不跑 / 边跑边读 / 改代码做实验。
1.7 自测 ×3。

### 第 2 章 启动（目标 2800 字）

2.1 加电链路：BIOS → 引导设备 → GRUB → Multiboot 规范 → 32 位保护模式、`EAX=magic`、`EBX=mb_info`。
2.2 Multiboot 头逐字段：`0x1BADB002`、`flags=0x03`（bit0 页对齐、bit1 提供内存信息）、校验和 `-(magic+flags)` 的补码本质；为什么必须在前 8KB。
2.3 `boot.asm` 逐行精读：`section .multiboot` / `align 4` / `.text` / `mov esp, stack_top` / `push ebx` / `push eax`（cdecl 从右到左，所以 ebx 先压）。
2.4 `.bss` 里 16KB 栈：`stack_bottom: resb 16384`、`stack_top:` 的符号含义；为什么栈必须自己建。
2.5 `linker.ld` 的作用：`.multiboot` 段优先、`_kernel_end` 被 C 引用、加载地址与 VMA。
2.6 `kernel_main` 九步初始化逐条解释与顺序理由：`vga_clear` → 打印 → `gdt_init` → `tss_init` → `fs_init` → `idt_init` → `irq_init` → `memory_init` → 写 `init` 到 RamFS → `timer_init(100)` → 注册 IRQ0 → `sti` → `keyboard_init` → IRQ1 → `shell_init`。
2.7 `embed.asm` 的 `incbin`：为什么用户程序被"编译进内核"，`user_bin_start/end` 如何变成 RamFS 文件。
2.8 观察：串口应出现 `Hello, OS World!`、`RAM: …MB, …MB free`、`Paging enabled (8MB identity-mapped).`、`Interrupts enabled.`。
2.9 常见误解（`-kernel` 不是 GRUB 启动、magic 未校验、栈溢出无人管）。
2.10 自测 ×3。

### 第 3 章 分段、特权级与 TSS（目标 3000 字）

3.1 保护模式为什么还需要段：基址/界限/权限三段式，平坦模型只是"极限值"用法。
3.2 GDT 描述符 8 字节逐字段图（base 拆三段、limit 拆两段、granularity 高 4 位、access 8 位）。
3.3 6 项表：NULL / CODE0 `0x9A` / DATA0 `0x92` / CODE3 `0xFA` / DATA3 `0xF2` / TSS `0x89`，逐位解读 access（P/DPL/S/Type）。
3.4 选择子算术：`index<<3 | TI | RPL`，逐个算出 `0x08/0x10/0x1B/0x23/0x28`；`0x1B=0b11011` 拆位。
3.5 `lgdt` + `ljmp $0x08,$label` 为什么必须远跳转（CS 不能 `mov`）；`gdt_flush` 代码逐行。
3.6 TSS：40 字节结构表、`ss0=0x10`、`esp0` 初值 0、`ltr 0x28`、`tss_set_esp0` 的调用时机（`elf_exec`、`task_yield`）。
3.7 Ring3→Ring0 硬件动作序列：取 TSS → 新 SS:ESP → 压旧 SS/ESP/EFLAGS/CS/EIP → 清 IF（中断门）→ 跳到 handler；配 V4 图。
3.8 本项目 TSS 的简化：无 `iomap_base`（真实 TSS 结构 104 字节），因此不能做 I/O 权限位图；只靠内核不做 `in/out` 之外的控制。
3.9 自测 ×3。

### 第 4 章 中断、异常与 PIC（目标 3600 字）

4.1 三类事件区分：异常（同步，CPU 内部）、IRQ（异步，PIC）、软中断（`int n`）。
4.2 IDT 门描述符 8 字节字段表；`0x8E` = P|DPL0|0x0E，`0xEE` = P|DPL3|0x0E；为什么 syscall 门必须 DPL=3。
4.3 向量分配表：0–31 异常、32–47 IRQ、0x80 syscall、其余未用。
4.4 stub 宏：`ISR_NOERR` 补 `push 0`，`ISR_ERR` 不补；带错误码向量清单 8/10/11/12/13/14/17/30；`IRQ` 宏的两个参数。
4.5 `isr_common` 压栈顺序逐步表：`cli` → `pusha` → `push ds` → 装 `0x10` → `push esp` → `call` → `add esp,4` → 恢复 → `popa` → `add esp,8` → `iret`。
4.6 `registers_t` 与栈帧一一对应（V4 双栏 SVG）；`packed` 的必要性。
4.7 C 侧 `isr_handler`：`int_no==0x80` 转 syscall；异常名表；只有 8/18 停机，其余返回（教学取舍及其风险）。
4.8 PIC 8259A：为什么 IRQ0–7 默认撞上异常向量 8–15；ICW1–ICW4 四字节表；主 0x20/0x21、从 0xA0/0xA1。
4.9 `irq_handler`：先 EOI 后回调的取舍（回调里再阻塞的风险）；`irq >= 40` 需要从片 EOI；`irq_unmask` 位运算。
4.10 中断门清 IF 与 `sti` 的位置；为什么 `kernel_main` 在 timer 注册后才开中断。
4.11 自测 ×3。

### 第 5 章 PMM（目标 3200 字）

5.1 物理内存管理要回答的三个问题：有多少页、哪些空闲、怎么分配/回收。
5.2 Multiboot mmap：`size` 字段含自身 → 步进 `entry->size + 4`；64 位 `base_addr/length` 被截断成 `uint32_t` 的隐患。
5.3 `total_ram` 取所有 type=1 区间的最高尾址；无 mmap 时回退 `mem_upper`。
5.4 位图计算：`num_pages = total_ram/4096`、`words=(num_pages+31)/32`、位图放 `page_align_up(_kernel_end)`、`bitmap_sz`。
5.5 `pmm_init` 三步：清位图 → 标记低 1MB → `next_free=256`。
5.6 `pmm_alloc_page` 线性扫描 + 回绕代码块；`next_free=i+1`；最坏复杂度与碎片表现。
5.7 `pmm_free_page` 的幂等、`next_free` 回退优化。
5.8 `pmm_mark_region`：`len` 向上取整到页、越界保护、`free_pages` 增减只在状态变化时发生。
5.9 预留策略：内核+位图到 `max(used_end,0x200000)`；为什么是 2MB（页目录固定地址）。
5.10 与分页的衔接：identity 只 8MB，但 PMM 管 127MB，这一不一致如何在学习实验里暴露。
5.11 观察：`make check` 的 8 个 PMM 用例语义逐条解释。
5.12 自测 ×3。

### 第 6 章 分页（目标 3600 字）

6.1 分页解决什么：虚拟地址、隔离、按需映射；与分段并存时的先段后页顺序。
6.2 32 位两级翻译：`PDI=virt[31:22]`、`PTI=virt[21:12]`、`offset[11:0]`；CR3 存页目录物理基址；配 V6。
6.3 PDE/PTE 位域表（P/RW/USER/PWT/PCD/A/D/PS/G 等，只讲用到的 3 位）。
6.4 固定布局：`PAGE_DIR=0x200000`、`PAGE_TAB0=0x201000`、`PAGE_TAB1=0x202000`；两张表各覆盖 4MB，共 8MB identity。
6.5 `paging_init` 三步：填两张表 → 清 PDE[2..1023] → 填 PDE[0]/[1] → `pmm_mark_used(0x200000,0x3000)` → 写 CR3 + 置 CR0.PG。
6.6 `paging_map_page` 逐行：PDE 缺页表则分配并清零；`PAGE_DIR[pde] |= USER` 的"上推"；取表基址 `&0xFFFFF000`；写 PTE。
6.7 "USER 上推整个 4MB"的后果：一个用户页会让同 4MB 内所有页对 Ring3 可见（若 PTE 也是 USER），配 `.warn` + 实验。
6.8 TLB 与刷新：为什么要重写 CR3；`elf_exec` 里的 `flush_tlb`。
6.9 已定义但无调用者：`paging_switch_dir`、`paging_clone_dir`、`elf_load` — 讲清"代码存在 ≠ 功能接通"。
6.10 ELF 装载如何复用 identity：先映射 `0x400000` 虚拟页到新物理页，退出时恢复 identity。
6.11 观察与实验：读 CR3、在用户态越界访问触发 page fault（可按现有异常打印观察）。
6.12 自测 ×3。

### 第 7 章 时钟、键盘与输出（目标 3200 字）

7.1 PIT：输入频率 1.19318MHz、通道 0 接 IRQ0、模式 2（rate generator）。
7.2 模式字 `0x36` 位分解；`divisor=1193180/100=11931`；实际频率与误差；tick=10ms。
7.3 `timer_handler` 只 `ticks++` 的理由（ISR 里不做重活）；`volatile` 的必要性。
7.4 `timer_uptime_sec = ticks/freq`；`sys_sleep` 如何用 tick 换算毫秒。
7.5 键盘控制器：数据口 0x60、状态口 0x64、set 1 扫描码（按下/释放 bit7）。
7.6 `keyboard_handler` 状态机：`shift/ctrl/e0_prefix` 三个变量；为什么 `0xE0` 判断必须在释放过滤之前。
7.7 方向键映射为 `0x82/0x83`（非 ASCII），给 shell 历史导航用。
7.8 输入归属：`shell_owns_input() = !shell_edit_mode && !shell_input_suspended`，三分支表（内核 shell / edit / Ring3）。
7.9 环形缓冲 `kb_buf[256]`：head/tail/count、满则丢弃、中断与主循环的并发假设。
7.10 `kb_getc` 的 `sti; hlt; cli`：先开中断再睡，醒来关中断，为什么不会丢唤醒。
7.11 VGA：`0xB8000`、80×25、每格 2 字节（字符+属性 0x07）、`scroll` 搬 24 行、CRTC 0x3D4/0x3D5 光标。
7.12 串口 COM2 `0x2F8`：LSR(+5) bit5 轮询；为什么选 COM2（COM1 被别的东西占用/便于双串口分流）。
7.13 观察：`make serial`、`make kbdtest`。
7.14 自测 ×3。

### 第 8 章 RamFS（目标 3200 字）

8.1 设计目标：无磁盘、无目录、够 shell 与 ELF 用即可；接口向 POSIX 靠。
8.2 `inode_t` 字段表；一个文件 = 3 页（inode 页 + `data` + `data2`）；`next` 头插单链表。
8.3 `capacity = PAGE_SIZE_*2 = 8192`，与 `fs.h` 注释"4KB"不一致（`.warn`，以代码为准）。
8.4 `fd_table[16]`：0/1/2 常开；`alloc_fd` 从 3 开始；`fs_close` 拒绝关闭 0–2。
8.5 `fs_open(name, create)`：线性查找、不截断已有文件、返回最小可用 fd。
8.6 `fs_read`：fd0 走 `kb_getc`（一次 1 字节）；文件读的两页寻址公式 `(off<PAGE)?data:data2` + `off & 0xFFF`；EOF 返回 0。
8.7 `fs_write`：fd1/2 同时写 VGA+串口；文件写按页切分、容量满后返回部分写入；`size` 只增不减。
8.8 `fs_seek`：SET/CUR/END 三种 base，负偏移夹到 0，超尾夹到 size。
8.9 `fs_list` 与 shell `ls/cat/write/edit/touch`。
8.10 实验：写入 9000 字节观察只落 8192；写满后 seek 到 0 再读。
8.11 自测 ×3。

### 第 9 章 ELF32 与 Ring 3（目标 4200 字）

9.1 ELF32 文件结构：ELF header / program header table / segments；本项目只用 `ET_EXEC` 静态可执行。
9.2 校验清单表：magic 4 字节、`EI_CLASS=1`、`EI_DATA=1`、`e_type=ET_EXEC`、`e_machine=3`、`e_phnum<=8`。
9.3 `elf_exec` 前置条件：`user_mode` 禁止嵌套；`fs_open(name,0)`；读头/读程序头。
9.4 `load_segment` 逐行：按页循环 `seg_off < p_memsz`；`vpg` 对齐、`page_off`；只拷 `filesz` 内数据，其余清零。
9.5 `MAX_ELF_PAGES=64` 的取舍（256KB 用户镜像上限）；`ldr_vaddr/ldr_phys` 记录用于退出清理。
9.6 用户栈页与内核栈页：两个 `pmm_alloc_page`；`tss_set_esp0(kstack+4096)`。
9.7 `iret` 五连压栈：`ss/esp/eflags/cs/eip` 的顺序必须与 CPU 期望一致；`eflags=0x202`。
9.8 `kctx_save` 汇编逐行：保存 esp/ebp/ebx/esi/edi/eip/cs/eflags；`xorl %eax,%eax; ret` 返回 0。
9.9 `kctx_restore` 汇编逐行：换栈 → 恢复寄存器 → 压 eflags/cs/eip → `eax=1` → `iret`。
9.10 两阶段完整时间线（V8 时序图）：`elf_exec` 首次返回 0 → 进 Ring3 → 程序 `exit` → `int 0x80` → `sys_exit` → `kctx_restore` → 回到 `elf_exec` 返回 1 → 清理 → 返回 0。
9.11 `user_pages_release` 清理清单：恢复 identity 映射、释放镜像页/用户栈/内核栈、`tss_set_esp0(0)`、刷 TLB。
9.12 失败路径：任何一步失败都要 `fs_close` + `user_pages_release`；哪些路径可能漏释放。
9.13 为什么 `fork` 不能工作在用户态（与第 11 章呼应）。
9.14 观察 `make ring3`：`exec init` → `hello` → `exit` 的 sendkey 序列与串口判定。
9.15 自测 ×3。

### 第 10 章 系统调用（目标 4000 字）

10.1 完整路径：Ring3 `int $0x80` → 门 DPL=3 → `syscall_stub` → `isr_common` → `isr_handler` → `syscall_handler` → 改 `regs->eax` → `iret`。
10.2 寄存器约定：`eax` 号、`ebx/ecx/edx` 参数、返回值写回 `eax`；与 Linux i386 的相似与不同。
10.3 17 项表（0–16）+ 分类表（进程 5 / IO 4 / 时间 3 / 文件 3 / 其他 2）。
10.4 `sys_write`：直接 `fs_write`，fd1/2 双输出。
10.5 `sys_read`：stdin 的回显循环、退格、`max-1` 留结尾、`buf[i]='\0'`。
10.6 `sys_exit`：`elf_exit_to_kernel()`；无用户上下文时 `cli;hlt` 死循环。
10.7 `sys_exec`：调用 `elf_exec`，用户态下必失败（`user_mode` 检查）。
10.8 `sys_sleep`：`(ms*freq+999)/1000` 向上取整、`sti;hlt;cli` 忙等。
10.9 返回值坑：`sys_yield` 不写 eax（返回值是上次的垃圾）、`sys_uname` 不补 `\0`、`sys_getc` 用 0 表示"无输入"与字符 `'\0'` 不可分。
10.10 安全缺口：完全信任 `ebx/ecx` 用户指针；一个 `SYS_WRITE` 指向内核地址即可读内核内存（`.warn` + 实验设计）。
10.11 扩展一个新 syscall 的 6 步（编号、表项、handler、用户侧封装、文档、测试）。
10.12 自测 ×3。

### 第 11 章 任务与调度（目标 4000 字）

11.1 现状声明：调度器存在，但 Ring 3 仍由 `elf_exec` 同步执行，两者未打通。
11.2 `task_t` 字段表（esp/stack/user_esp/user_stack/kernel_esp0/pgdir/is_user/pid/name/state/wake_at）。
11.3 状态机：READY/RUNNING/SLEEPING/DONE 与 `schedule` 的转移。
11.4 `_task_switch` 栈帧布局（V10）；`pusha`+`push ds` 后 `40(%esp)`/`44(%esp)` 的偏移推算。
11.5 `_task_switch` 结尾 `pop ds; popa; add $4; add $8; iret` 的含义（丢返回地址、err_code、int_no）。
11.6 `schedule()` 轮转：唤醒睡眠任务、从 `cur_idx+1` 起找 READY、旧 RUNNING 降级。
11.7 `task_yield`：首次 `cur_idx<0` 的引导、保存 ESP、切 TSS.esp0、调用 `_task_switch`。
11.8 `task_sleep_ms` 与 `task.h` 声明的 `task_sleep` 不一致（`.warn`）。
11.9 `task_fork` 三个缺陷逐条定位：`task_count--` 留空洞、`eax_off` 偏移推算、`pgdir` 克隆了但从不写 CR3。
11.10 `task_exit` 的 `task_count--` 会让后续 `tasks[i]` 访问越界/漏任务。
11.11 正确路线（P1 实验）：per-task 页目录 + 用户栈复制 + `_task_switch` 前 `paging_switch_dir` + `TSS.esp0`。
11.12 自测 ×3。

### 第 12 章 测试与构建（目标 3400 字）

12.1 三个构建目录的必要性：`-DTEST` 不能污染发布对象。
12.2 Makefile 分段讲解：变量、5 条模式规则、`|` order-only 依赖、`-MMD -MP`、`-include *.d`。
12.3 依赖追踪验证：`touch include/pmm.h` 只应重编 `pmm.o` 与 `memory.o`。
12.4 测试协议：为什么用 `[PASS]/[FAIL] name (line N)` + `RESULT n/N` + `TESTS_OK` 标记，而不是靠 QEMU 退出码。
12.5 27 个用例逐组解释（ISR 11 / PMM 8 / PIT 4 / CMD 4），每个用例"验证了什么不变量"。
12.6 `make check` 判定代码块：`grep '^TESTS_OK'`、超时 10s、失败时打印整个串口日志。
12.7 反例：`TEST(name,1)` 式假测试为什么能"永远绿"，以及如何用注入失败验证框架（退出码 2）。
12.8 `make ring3`：QEMU monitor `sendkey` 序列、sleep 时序、`grep 'exec: program exited'`。
12.9 Makefile 三个真实的坑：赋值行尾随空格、`-timeout` 前缀、旧对象散落根目录。
12.10 实验：新增一个测试组并在其中故意失败一次，观察 `make check` 变红。
12.11 自测 ×3。

### 第 13 章 限制与分级实验（目标 3000 字）

13.1 限制清单表（≥16 行，每条含现象/根因/影响/修复难度）：仅 8MB identity、USER 上推、无用户指针校验、用户态 fork 不可用、调度未接 Ring3、`task_fork` 缺陷、`task_exit` 计数、RamFS 无 unlink/truncate、文件 8192B 上限、串口未初始化、VGA 无滚回、`sys_yield` 返回值、`sys_uname` 无终止符、TSS 无 iomap、`init.c` 未用 `rc`、`init.c` UTF-8 `—` 显示 `ΓÇô`、`fs.h` 注释 4KB 不符、`task.h` 签名不符、`init.c` 注释提 `cd` 但无实现、CLAUDE.md 与实际 Makefile 参数不一致。
13.2 P0 实验（不改架构即可完成）：修 `init.c` 乱码；把 `ls/cat/write` 做成 Ring3 内建；`sys_uname` 补 `\0`；`sys_yield` 写 `eax`。
13.3 P1 实验（中等改动）：真 fork + `paging_switch_dir` + per-task CR3；用户指针校验（地址范围检查）；RamFS 加 `unlink`/`truncate`；`kmalloc` 小对象分配器。
13.4 P2 实验（课程设计级）：把 `elf_exec` 改成任务化调度（多进程 + 时间片）；实现 COW fork；加简单 VFS 抽象。
13.5 每个实验卡固定字段：目标 / 涉及文件与函数 / 步骤 / 验收命令 / 预期输出 / 思考题。
13.6 课程安排建议：3 周版与 6 周版各一张表。
13.7 自测 ×3。

### 附录（目标 2500 字）

A 命令速查：`make` 全部目标 + QEMU 常用参数 + `grep` 日志技巧。
B 17 个系统调用表（号/名/参数/返回/备注）。
C 常量表：GDT 选择子、IDT 门、页表标志、PIT divisor、地址（0x200000/0x201000/0x202000/0xB8000/0x2F8）。
D 术语表（≥30 条，中英对照 + 一句话）。
E 源码索引表（47 个文件 → 职责 → 关键函数/常量）。
F 调试技巧：串口日志分流、`-d int,cpu_reset`、GDB stub、常见"静默失败"排查顺序。

## 6. 视觉清单（14 个，位置与类型固定）

| # | 位置 | 类型 | 工具 | 内容 |
|---|---|---|---|---|
| V1 | 1.5 | 模块依赖 | Mermaid flowchart | boot/interrupt → kernel → 各子系统 |
| V2 | 2.6 | 初始化流程 | Mermaid flowchart | Multiboot → boot.asm → 9 步初始化 → shell |
| V3 | 3.2–3.4 | 内联 SVG | SVG | GDT 描述符字段 + 选择子位域 |
| V4 | 4.5–4.6 | 内联 SVG | SVG | `registers_t` 栈帧两栏（同特权/跨特权） |
| V5 | 6.4 | 内联 SVG | SVG | 0–8MB+ 虚拟地址布局 |
| V6 | 6.2 | 页表遍历 | Mermaid flowchart | CR3 → PDE → PTE → 物理页 |
| V7 | 5.11 | 柱状图 | ECharts | 四组测试用例数量 11/8/4/4 |
| V8 | 9.10 | 时序图 | Mermaid sequenceDiagram | shell→elf_exec→Ring3→sys_exit→清理 |
| V9 | 10.3 | 分类饼图 | ECharts | 17 个 syscall 五类分布 |
| V10 | 11.4 | 内联 SVG | SVG | `_task_switch` 前后栈布局 |
| V11 | 4.8 | 流程图 | Mermaid flowchart | PIC 重映射 ICW1–ICW4 与向量偏移 |
| V12 | 8.5–8.7 | 内联 SVG | SVG | `fs_read/write` 的 4096 边界分页寻址 |
| V13 | 7.8 | 数据流 | Mermaid flowchart | 扫描码 → 归属判断 → shell/环形缓冲 |
| V14 | 9.4 | 内联 SVG | SVG | ELF `p_offset` → 页内偏移 → VMA 映射 |

图表规则：ECharts `renderer:'svg'`、`animation:false`、`tooltip.appendToBody:true`、颜色全取自 CSS 变量、每图 `resize`；全部初始化在 `assets/charts.js` IIFE。Mermaid `theme:'neutral'`、`securityLevel:'loose'`、`startOnLoad:true`；`<pre class="mermaid">` 内只放图定义，节点文本不超过 28 个汉字。

## 7. 写作硬约束

1. 每个结论都能回源：文件名、函数名、常量与源码完全一致；代码片段可省略无关行，但不得改写语义。
2. 主动标注"注释/文档与实现不一致"：`fs.h` 4KB vs 8192、`task.h` `task_sleep` vs `task_sleep_ms`、`init.c` 注释 `cd` 无实现、CLAUDE.md 的 5s/`serial.log` vs Makefile 10s/`build/test.log`。
3. 不虚构能力：`paging_switch_dir`/`paging_clone_dir`/`elf_load` 注明无调用者；用户态 `fork` 注明恒 -1；调度器注明未接 Ring3。
4. 每个"坑"必须给出可观察证据（日志行、异常输出、测试用例名），不能只断言。
5. 每章至少 1 个"动手"：命令 + 预期输出 + 若不符的排查方向。
6. 避免大段贴源码：单段代码 ≤ 25 行，超出则拆成"结构 + 关键行"两段并做表格化说明。
7. 中英混排规范：中文与英文/数字间加空格；寄存器大写（EAX）、函数名保持源码大小写。
8. 术语首次出现给英文原文与一句话解释，附录 D 汇总。

## 8. 验收标准（可执行）

1. 本地 `python3 -m http.server 8765` 打开页面：侧栏 14 个锚点全可达；4 个 Mermaid 图渲染成功（无 syntax error）；2 个 ECharts 有图形；8 个内联 SVG 正常；控制台 0 报错；无 404。
2. 结构：`<h2>` 14 个；每章含"学习目标"与"自测"；`.table-wrap` 覆盖全部表格；每个图有 `figcaption`。
3. 篇幅：正文中文 ≥ 35000 字；第 9/10/11 章各 ≥ 3500 字；附录 ≥ 2000 字。
4. 抽查 8 处与源码逐字一致：`0x1BADB002`、`0x03`、`0x1B`、`0x8E/0xEE`、`11931`、`0x200000/0x201000/0x202000`、`8192`、`SYS_MAX 17`。
5. 移动端 375px：无横向溢出（代码块与表格自身可滚），侧栏隐藏。
6. 讲义不改动构建：`make check` 仍 27/27、`make ring3` 仍 PASS。

## 9. 已定决策

- 单页 HTML + 侧栏；不拆多页。
- 中文正文 + 英文标识符；单语。
- 不用 AI 插画，全部用示意图/流程图/图表。
- 强调统一 `<strong>`。
- 只嵌讲解所需片段，不嵌整文件。
- 自测答案用原生 `<details>`，不引入额外 JS。
