# Session Handoff (2026-08-25)

状态快照供新 session 独立冷启动。**先读本文件，再读 docs/00_plan.md（M4 验收），最后 docs/04_m3_log.md/03_m2_log.md/02_m1_log.md/01_bringup_log.md（证据）。**

## 当前事件顺序（新 session 第一件事）

1. 读「关键事实」章节（不要猜，只信这里 + 文档）
2. 复述 M4 验收条件，再改文件（AGENTS.md：先复述再动手）
3. 用固定命令验证基线构建（环境见下）

## 已完成

- M0/M1/M2 全部通过（`48772f3`…`7d5437c`）。
- **M3 已通过（2026-08-25，未提交——新 session 第一件事是审查并提交 M3 工作区）**：LVGL v9.3.0 SW direct 双缓冲接 GFXMMU virtual 0/1；display port 契约=flush 只在本帧最后一块提交 swap（`lv_display_flush_is_last`，对齐 stock `lv_st_ltdc.c`）+ `flush_wait_cb` 自旋 `g_fb_swap_pending==0`（不调 `lv_display_flush_ready`）；LVGL heap 256 KiB→LVGLH、FreeRTOS heap 128 KiB app-allocated→RTOSHEAP 已落地 linker；runtime stats 64-bit provider + LVGL idle% trace hooks；项目 profiler backend（24 slot + 64 ring，.perf_trace@SRAM2）。证据见 `docs/04_m3_log.md`。

## 下一步 = M4（NeoChrom + DMA2D、Cache 一致性与串行路由）

验收（计划 §7 M4，先读原文档再动手）：预编译 Nema 库 ABI 审计、项目 HAL-owned 同步 DMA2D draw unit（unit ID=5/score=20/50 ms poll timeout）、DCACHE2 保持关闭并清读 SRAMCACHED、串行路由断言、四 profile 构建（bare 固定命令 + 显式 preset）、CRC/容差门控、错误注入路径、30 min→**10 min**（新门限）长稳。

## 关键事实（不可猜）

### 硬件/探针
- 板：STM32U5A9J-DK，ST-LINK SN `003C00333532510E31333430`（FW V3J17M11）
- 探针只读：CubeProgrammer CLI（HOTPLUG 读内存；UR 复位/烧录）。GDB Server 不用
- HOTPLUG 侵入：读数要快
- **连续验收时长门限：10 min（用户决定 2026-08-25，长期有效，不再跑 30 min）**

### 显示栈（全部已验证）
- M3 后管线：LVGL task(defaultTask, 16 KiB 栈) → refr → swdraw 线程（v9.3 OS 模式自动建，8 KiB/HIGH）渲染到 back virtual → 最后一块 flush 提交 FB_Submit → VBlank reload → 回调换 ownership 并 re-arm RR（勿删 re-arm）
- GFXMMU/LTDC/DSI/LUT/双缓冲状态机 = M2 冻结态，未改
- `Board_LCD_VerifyMapping` 每次启动 3/3（可作复位证据的一部分）
- 帧率名义 79.125 Hz（实测 78.5，±1 % 门）

### M3 新增内存布局锚点（本构建 nm 实测，重建后需重新 nm）
- `.lvgl_heap` 0x20110000 256K（`work_mem_int.0`，注意 GCC 段名后缀，linker 用 `*(.bss.work_mem_int*)`）
- `.freertos_heap` 0x20150000 128K（`ucHeap`，app-allocated）
- `.perf_trace` 0x200C8000 8928 B（g_prof_slots，24 slot × 372 B：name[32]/calls/cycles/min/max@+32/+36/+40/+44/ring[64]@+48/ring_phase[64]@+304/ring_idx@+368）
- `g_m3_stats` 0x20171B68（1 Hz 快照：uptime/phase/frames/submit/done/refr/sync/wait/est/line_events/idle%/lv_max/rtos_min/hwm/errors/front）
- fb 序列号块 0x20171DE8 起（dsi/ltdc/gfxmmu 错误计数 + submit/done/pending/errors/front/back）
- **符号地址随构建漂移：每次上板前重新 `arm-none-eabi-nm`**

### 构建环境（必须用，别用裸 PowerShell）
- 工具链 bundle 路径与 M1 相同；模板脚本 `C:\Users\86151\AppData\Local\Temp\opencode\m3_build.cmd`（含 PATH/CMAKE_GENERATOR/CMAKE_TOOLCHAIN_FILE 注入 + 全新 build）
- hex：`arm-none-eabi-objcopy -O ihex build/u5a9_lvgl.elf build/u5a9_lvgl.hex`
- 烧录：`STM32_Programmer_CLI -c port=SWD mode=UR sn=<SN> -w build\u5a9_lvgl.hex -v -rst`
- 复位脚本：`reset20c.ps1`（20 轮 UR 复位 + 14 s 后 HOTPLUG 读两块；解析器 parse_reset20c.ps1）

### 纪律 / 陷阱
- **AGENTS.md** 全部约束适用；先复述验收再动手
- CubeMX 再生成会冲：Src/dsihost.c、Src/ltdc.c、Src/gfxmmu.c 生成区手改需恢复；FreeRTOSConfig.h 的 M3 改动全在 USER CODE Defines（再生安全）；linker/CMake/lv_conf.h 为项目所有
- LVGL vendor 树零修改已验证（git clean）；M4 也不得改
- 场景教训（勿回退）：swap 必须只在最后一块 flush 提交；动画用 lv_anim 时间驱动；phase A 底色 1 Hz 步进
- 未提交的临时脚本/探针都在 `C:\Users\86151\AppData\Local\Temp\opencode\`

## 建议的第一步（M4）

1. 审查并提交 M3 工作区（git status：6 个修改文件 + platform/os、platform/perf、platform/display/lv_port_display.*、docs/04_m3_log.md、docs/00_plan.md、docs/HANDOFF.md）
2. 按 §9.6 先复述 M4 验收条件
3. Nema 预编译库 ABI 审计（vendor 树 libs/nema_gfx，锁 hash、M33/Thumb/hard-float）
