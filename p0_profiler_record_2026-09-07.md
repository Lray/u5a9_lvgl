# P0 LVGL 性能记录（2026-09-07）

## 当前固件

- 固件：`build/Release/u5a9_lvgl.elf`
- 状态：`LV_USE_PROFILER=1`、`LV_USE_PROFILER_BUILTIN=1`
- profiler 输出：`flush_cb=NULL`，不走阻塞 UART；通过 ST-LINK 读取官方 RAM 缓冲区
- draw backend：NEMA/GPU2D 与 DMA2D 均启用

## ST-LINK 快照

`lv_global`：`0x20102E6C`

- `lv_global.inited`：`1`
- `lv_global.disp_default`：`0x20103E28`
- `perf_sysmon_info.calculated`：`0x201041A8`（显示结构基址 + `0x380`）
- built-in profiler context：`0x20103DF4`
- item buffer：`0x20108004`
- item capacity：`0x2AA`（682）
- 当前索引快照：`0x154`
- profiler tick frequency：`1000 Hz`

最近读取到的官方 profiler 记录包含：

- `NEMA_GFX`（字符串地址 `0x08046AB2`）
- `IMAGE`、`lv_display_refr_timer` 等刷新/绘制 scope
- 本次读取窗口未见 `DMA2D`（其官方名字字符串为 `DMA2D`）

B 组（NEMA/GPU 关闭、DMA2D 开启）烧录后读取：

- `lv_global`：`0x20102E5C`
- `disp_default`：`0x20103E18`
- `perf_sysmon_info.calculated`：`0x20104198`
- `cpu_avg_total=8%`、`fps_avg_total=7`、`run_cnt=862`
- 该次读取的瞬时 `fps/cpu/render/flush` 均为 0，符合 sysmon 发布后清零行为

C 组（NEMA/GPU 开启、DMA2D 关闭）烧录后读取：

- `lv_global`：`0x20102E68`
- `disp_default`：`0x20103E24`
- `perf_sysmon_info.calculated`：`0x201041A4`
- `cpu_avg_total=10%`、`fps_avg_total=10`、`run_cnt=300`
- 该次读取的瞬时 `fps/cpu/render/flush` 均为 0

D 组（NEMA/GPU 与 DMA2D 均关闭）烧录后读取：

- `lv_global`：`0x20102E58`
- `disp_default`：`0x20103E14`
- `perf_sysmon_info.calculated`：`0x20104194`
- `cpu_avg_total=9%`、`fps_avg_total=9`、`run_cnt=1748`
- 该次读取的瞬时 `fps/cpu/render/flush` 均为 0

A 组（NEMA/GPU 与 DMA2D 均开启）重新烧录后读取：

- `lv_global`：`0x20102E6C`
- `disp_default`：`0x20103E28`
- `perf_sysmon_info.calculated`：`0x201041A8`
- `cpu_avg_total=10%`、`fps_avg_total=7`、`run_cnt=581`
- 该次读取的瞬时 `fps/cpu/render/flush` 均为 0

## 四组合当前快照汇总

| 组 | NEMA/GPU | DMA2D | CPU_avg_total | FPS_avg_total | run_cnt |
|---|---:|---:|---:|---:|---:|
| A | 开 | 开 | 10% | 7 | 581 |
| B | 关 | 开 | 8% | 7 | 862 |
| C | 开 | 关 | 10% | 10 | 300 |
| D | 关 | 关 | 9% | 9 | 1748 |

这些是各次重启后不同 `run_cnt` 的独立快照，不是严格 A/B 实验；它们只能说明当前硬件路径没有出现数量级差异。正式结论仍需固定场景、固定采样时长并取多次中位数。

一次此前捕获的完整刷新 scope 为约 `1 ms`（tick `188302` 到 `188303`），该帧没有出现 flush-wait scope。

## profiler-on / profiler-off 记录说明

此前的临时 ST-LINK 快照曾放在 `build\profiler_stlink_raw.bin`、`build\profiler_stlink_snapshot.bin` 和 `build\refresh_frame.bin`；后续 build 目录被清理，这些原始文件已不存在。因此旧数据不能声称为可复核的正式数据，本文件只保留已核对的地址、字段和读数摘要。

旧快照中的非正式读数：

- profiler-on（无 UART 阻塞）：累计 CPU 约 `23%`、累计 FPS 约 `59`、`run_cnt=80`；瞬时字段可能为 0。
- profiler-off：不同时间窗约 `CPU 10–12%`、`FPS 10–16`；未能保证同一场景、同一窗口，不能作为最终验收基线。

正式对比必须用相同场景、相同时间窗和相同固件配置，分别记录 profiler-off 粗基线与 profiler-on 诊断数据；profiler-on 只用于定位 scope/等待，不能把阻塞 UART 版本的结果当作性能基线。

## GPU/NEMA 与 DMA2D

两者存在能力重叠，但 LVGL draw dispatcher 为每个 draw task 选择一个 owner，不会默认把同一个 task 同时执行两遍。官方 profiler 中的 `NEMA_GFX`、`DMA2D` 和 `wait_for_finish_cb` scope 用来确认实际归属及等待成本。

下一轮使用四组合：

1. NEMA/GPU 开 + DMA2D 开（当前）
2. NEMA/GPU 关 + DMA2D 开
3. NEMA/GPU 开 + DMA2D 关
4. 两者都关（软件参考）

只有在相同场景下比较四组的 FPS、CPU、render、flush 和 backend/wait scope，才能判断重叠是否造成退化。
