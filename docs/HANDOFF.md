# Session Handoff (2026-08-25, session 2 — M4 layer path done)

状态快照供新 session 独立冷启动。**先读本文件，再读 docs/00_plan.md（§7 M5 规划），最后 docs/05_m4_log.md/04_m3_log.md（证据）。**

## 当前事件顺序（新 session 第一件事）

1. 读「关键事实」
2. 复述 M5 验收条件，再改文件
3. 固定命令验证基线构建（环境见下）

## 已完成

- M0–M3 全部通过并提交（`48772f3`…`9b33e43`）。
- **M4 层路径阶段通过（`6e7a7f8` 为 step A/B；本轮另有 layer 路径成果未提交——新 session 先提交）**：Nema 集成（GPU2D HAL + pool 240,640B@0x200D0000 + SRAMCACHED=0）、项目 DMA2D draw unit（ID5/score20/R2M+OOR/50ms poll/abort+re-init）、屏蔽路由（根层 shield→SW，Nema 只服务连续层缓冲）、transform 层路径验证通过、10-min 门限验收通过。20-reset 被用户跳过（已记录）。证据 `docs/05_m4_log.md`。

## 下一步 = M5（外部存储、性能设施与合成基准）

先解决阻塞：**`stm32u5xx_hal_xspi.{c,h}` 不在任何工作区来源**（工程/STM32CubeU5/Riverdi 均无）——BSP `stm32u5x9j_discovery_hspi.c`/`_ospi.c`（APS512XX + MX25UM51245G）都依赖它。需从 ST 官方来源取得后再启动 M5 存储阶段（计划 §7 M5 / §4.5 / §4.6）。

## 关键事实（不可猜）

### 硬件/探针
- 板：STM32U5A9J-DK，ST-LINK SN `003C00333532510E31333430`（FW V3J17M11）；探针只读 CubeProgrammer CLI（HOTPLUG 读内存/UR 复位烧录）；GDB 不用
- **连续验收门限：10 min（用户决定，长期有效）**
- 用户接受跳过 M4 的 20 次复位验收（3× smoke 复位均干净）

### M4 后路由事实（板上验证，勿回归）
- 根层（GFXMMU 3072-B stride）= **SW only**：DMA2D 写 0x24000000 虚拟窗=死写（R2M 单测），Nema 单元 960-B pitch 硬编码不可用根层
- 连续离屏层缓冲 = **Nema/GPU2D（transform）+ 项目 DMA2D（clean fill）**
- 屏蔽路由：项目 DMA2D（head-most）以 score=80 声明所有根层任务 → dispatch 移交 SW（preferred=NONE/score=100）
- 不可改 LVGL vendor 树（每轮 git clean 验证）

### 构建环境（必须用，别用裸 PowerShell）
- 工具链 bundle 同前；模板 `C:\Users\86151\AppData\Local\Temp\opencode\m3_build.cmd`（改坏了用 `m1_build.cmd` 作模板重建）
- hex：`arm-none-eabi-objcopy -O ihex build/u5a9_lvgl.elf build/u5a9_lvgl.hex`
- 烧录/复位/读：CubeProgrammer CLI（URL/UR），reset20d.ps1（20 轮）

### 符号地址（本构建实测；每次重建后重新 nm）
- `g_m3_stats @0x20174D90`、`g_u5_dma2d_stats @0x201752B0`、`g_perf_*`、fb 块 @0x20175020（dsi/ltdc/gfxmmu 错误 + submit/done/pending/errors/front/back）、`g_gpu2d_sramcached_readback @0x20174EC4`、`g_prof_slots @0x200C8000`
- profiler slot：name[32]/calls/cycles/min/max @ +32/+36/+40/+44；ring[64]@+48；ring_phase[64]@+304；ring_idx@+368；slot=372 B

### 纪律 / 陷阱
- **AGENTS.md** 全部约束；先复述再动手；不允许改 vendor
- `.ioc` 不含 HSPI1/OCTOSPI1 外设块（仅时钟预留 160 MHz）——M5 采用方式 A（手写 MX_*_Init + BSP 组件，不碰 .ioc），与 GPU2D 同法
- CubeMX 再生成会冲：Src/dsihost.c、ltdc.c、gfxmmu.c 手改区
- 临时脚本在 `C:\Users\86151\AppData\Local\Temp\opencode\`

## 建议的第一步（M5）

1. 审查并提交未提交的 M4 层路径工作区（git status：lv_conf.h/app_freertos.c/lv_draw_dma2d_u5.c|h、docs/00_plan.md、docs/05_m4_log.md）
2. 取得 `stm32u5xx_hal_xspi.{c,h}`（用户授权 ST 官方源）
3. 复述 M5 验收 → 手写 `Src/hspi1.c`/`octospi1.c` + BSP 组件接入 → 探测/自检（APS512XX ReadID、NOR CRC、mmap 资源）
