# Session Handoff (2026-08-26, session 3 — M5 全部通过)

状态快照供新 session 独立冷启动。**先读本文件，再读 docs/00_plan.md（§7 M6 规划），最后 docs/06_m5_log.md（证据）。**

## 当前事件顺序（新 session 第一件事）

1. 读「关键事实」
2. 复述 M6 验收条件，再改文件
3. 固定命令验证基线构建（环境见下）

## 已完成

- M0–M4 全部通过并提交；**M5 全部阶段通过并提交**（`b50cb25`/`7d737bb`/`50b6faa`，另 stage5-6 待提交见下）：
  - 存储接入（方式 A 手写 hspi1.c/octospi1.c，不碰 .ioc）+ BSP 组件；**发现组件 MX25UM51245G_ReadID 在 OPI-DTR 下短读（第 3 字节丢失+BUSY 残留卡死后续命令），启动路径已移除**
  - MPU 8-region（PMSAv8 无 no-access → region7 属性钉扎+linker ASSERT）
  - PSRAM arena（32-B 对齐/canary）+ 64 MiB 全覆盖诊断（7/7 通过，14.0 s）
  - VCP CSV 遥测 @1 Hz（COM7 921600，零丢行）+ tsi_malloc wrap 统计（map+运行时双证）
  - NOR CRC32 链路（主机 zlib 对照验证；**板上 NOR 出厂含数据 ~90% 非零，CRC=0x1C1C6A9A**）
  - 五场景基准（10 s warm-up+60 s×3）+ 10-min 长稳 + monitor 扰动量化（≈1% CPU）

## M6 前置状态

- M5 未测试项（已记录）：XRGB8888、PSRAM cacheable、受控并发、正式 NOR 资源 manifest
- 触摸器件 Sitronix 经 I2C5（BSP 已备，未接入）

## 关键事实（不可猜）

### 硬件/探针
- 板：STM32U5A9J-DK，ST-LINK SN `003C00333532510E31333430`（FW V3J17M11）；探针只读 CubeProgrammer CLI（HOTPLUG 读内存/UR 复位烧录）；GDB 不用
- **连续验收门限：10 min（用户决定，长期有效）**
- **COM7 = ST-LINK VCP（921600 8N1）遥测 CSV 输出口；COM3/COM5 为其他串口勿用**
- HOTPLUG 活读有侵入性伪影（M1 记录）：关键判定一律复位后干净读

### 路由事实（板上验证，勿回归）
- 根层（GFXMMU 3072-B stride）= **SW only**（DMA2D 死写虚拟窗；Nema 960-B pitch 不可用）
- 连续离屏层缓冲 = Nema/GPU2D（transform）+ 项目 DMA2D（clean fill）
- 屏蔽路由：项目 DMA2D score=80 声明根层任务 → SW
- MPU：8 region 全过 readback；DCACHE1/DCACHE2 均未启用；PSRAM/NOR mmap 已上线
- NOR 出厂数据非零（~90%），正式资源规划须先擦除或避开
- 不可改 LVGL vendor 树（每轮 git clean 验证）

### 构建环境（必须用，别用裸 PowerShell）
- 工具链 bundle 同前；模板 `C:\Users\86151\AppData\Local\Temp\opencode\m3_build.cmd`（固定验收命令）
- 场景构建：`m5_bench_build.cmd <scene> [off]`（build-bench/build-off 独立目录）；monitor off 经 `CMAKE_C_FLAGS_DEBUG` 注入（工具链文件已改累积语义）
- 探针符号每次重建后查 map：`g_xspi_probe`/`g_mem_probe`/`g_m3_stats`/`g_fb_*`/`g_board_lcd_*`/`g_u5_dma2d_stats`/`g_nema_alloc_stats`

### 纪律 / 陷阱
- **AGENTS.md** 全部约束；先复述再动手；不允许改 vendor
- `.ioc` 不含 HSPI1/OCTOSPI1/USART1 外设块（方式 A 手写已建立）
- CubeMX 再生成会冲：dsihost.c/ltdc.c/gfxmmu.c 手改区
- 临时脚本在 `C:\Users\86151\AppData\Local\Temp\opencode\`（vcp_capture.ps1/bench_formal.ps1/analyze_bench.py 等可复用）

## 建议的第一步（M6）

1. 审查并提交 M5 阶段 5-6 工作区（benchmarks/、ospi_nor、perf_results、log/plan/handoff 更新、freertos_runtime_stats、lv_conf 覆盖、工具链累积）
2. 复述 M6 验收 → Sitronix 触摸接入（I2C5 方式 A 手写 MX_I2C5_Init + BSP_TS_* → lv_indev）
