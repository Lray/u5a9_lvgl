# LVGL 本轮性能数据验收报告

日期：2026-09-07  
原始数据：[`数据.md`](C:/Users/86151/Desktop/LVGL/u5a9_lvgl/数据.md)  
数据 SHA-256：`77F1FB24B02E911026AF9F2522495C58CA44CD70BFBBD8A86A29163695982E82`

## 1. 验收结论

**通过，限定用途为：在本硬件、本 LVGL 配置和官方 benchmark 负载下，对 NEMA GFX 与 DMA2D 四种组合进行相对性能比较。**

本轮数据可以回答：启用某个绘制后端后，各场景的平均 CPU 占用、平均帧率、平均渲染时间和平均 flush 时间如何变化，以及变化是否大于三次重复之间的波动。

本轮数据不能回答：真实手表业务的最终帧率、最差帧或卡顿、功耗、GPU/DMA2D 独立占用时间、端到端输入响应时间。原因是当前报告只有平均值，七个代理场景也没有真实业务权重。

## 2. 配置身份验收

四个构建目录均通过 `LV_CONF_PATH` 引用独立配置头，所有配置均明确设置 `LV_USE_PROFILER 0`：

| 组别 | variant 配置 | ELF 符号核验 | 结果 |
|---|---|---|---|
| NEMA off / DMA2D off | 两者均覆盖为 0 | 无 `lv_draw_nema_gfx_init`，无 `lv_draw_dma2d_init` | 通过 |
| NEMA off / DMA2D on | NEMA 覆盖为 0 | 仅有 `lv_draw_dma2d_init` | 通过 |
| NEMA on / DMA2D off | DMA2D 覆盖为 0 | 仅有 `lv_draw_nema_gfx_init` | 通过 |
| NEMA on / DMA2D on | 继承基础配置的两个开启项 | 两个初始化符号均存在 | 通过 |

共同基础配置为 RGB565（`LV_COLOR_DEPTH 16`）、刷新周期 12 ms、绘制缓冲区 16 字节对齐、绘制线程栈 8 KiB。四个 variant 分别见：

- [`off/off`](C:/Users/86151/Desktop/LVGL/u5a9_lvgl/config/variants/lv_conf_profiler_off_nema_off_dma2d_off.h)
- [`off/on`](C:/Users/86151/Desktop/LVGL/u5a9_lvgl/config/variants/lv_conf_profiler_off_nema_off_dma2d_on.h)
- [`on/off`](C:/Users/86151/Desktop/LVGL/u5a9_lvgl/config/variants/lv_conf_profiler_off_nema_on_dma2d_off.h)
- [`on/on`](C:/Users/86151/Desktop/LVGL/u5a9_lvgl/config/variants/lv_conf_profiler_off_nema_on_dma2d_on.h)

用于本轮归档的 ELF 信息：

| 组别 | 大小（byte） | SHA-256 |
|---|---:|---|
| off/off | 1,294,448 | `C0F50E28AB7B271831E1C6ED2A543D020B87E6CC69EA343DB37D47E41380E748` |
| off/on | 1,299,944 | `9F1D3296B11D882D1C20701BAFC100B7F3823A0B92BF68E96CA5979F4CD00A88` |
| on/off | 1,334,684 | `0206BE23A1C79B3F9F93AC936A3867BC164E0720B97F2EEFE515783A403C57AD` |
| on/on | 1,336,008 | `55303B1E91F19DCE939DFC83BA7A9ABEE672448BACC82F032B2FF29CEEA3D768` |

## 3. 原始数据完整性与一致性

| 检查项 | 结果 |
|---|---|
| 组别 | 4/4 完整 |
| 重复次数 | 每组 3 次，共 12 份报告 |
| 每份报告 | 16 个场景加 1 行 `All scenes avg.`，共 17 行 |
| 数据行总数 | 204 行 |
| 场景名称与顺序 | 12 份报告完全一致 |
| 字段格式 | CPU、FPS、total、render、flush 均可解析 |
| 数值边界 | CPU 在 0–100%，FPS 为正，时间非负 |
| 算术关系 | 204/204 行满足 `Avg. time = render time + flush time` |

`nema off dma2d on` 第二份报告前出现的 `Benchmark Summary (9.3.0)` 是官方版本标题，不构成额外测试，也不影响解析。

## 4. 三次重复的稳定性

全部场景、全部组别中，三次重复的最大跨度为：

| 指标 | 最大跨度 |
|---|---:|
| Avg. CPU | 2 个百分点 |
| Avg. FPS | 3 FPS |
| Avg. time | 2 ms |
| render time | 1 ms |
| flush time | 2 ms |

官方 `All scenes avg.` 的三次重复更稳定：

| 配置 | CPU 三次范围 | FPS 三次范围 | Avg. time 三次范围 |
|---|---:|---:|---:|
| off/off | 60–61% | 58 | 17 ms |
| off/on | 52% | 62 | 14 ms |
| on/off | 44% | 69 | 9 ms |
| on/on | 42% | 70–71 | 8–9 ms |

这些波动明显小于旋转 ARGB、overlay、透明容器和滚动场景中的主要技术差异，因此主要结论具有重复性。1–2 FPS 或 1 ms 的小差异仍视为量化噪声或刷新上限效应，不作为技术取舍依据。

## 5. 四组总体结果

下表是三次官方 `All scenes avg.` 行的均值。该行覆盖全部 16 个场景，并非只覆盖选定的七个手表代理场景。

| 配置 | Avg. CPU | Avg. FPS | Avg. time | render | flush |
|---|---:|---:|---:|---:|---:|
| NEMA off / DMA2D off | 60.67% | 58.00 | 17.00 ms | 14.00 ms | 3.00 ms |
| NEMA off / DMA2D on | 52.00% | 62.00 | 14.00 ms | 11.00 ms | 3.00 ms |
| NEMA on / DMA2D off | 44.00% | 69.00 | 9.00 ms | 6.00 ms | 3.00 ms |
| NEMA on / DMA2D on | 42.00% | 70.33 | 8.67 ms | 6.00 ms | 2.67 ms |

相对 off/off：DMA2D 单独开启降低 8.67 个 CPU 百分点、增加 4 FPS、减少 3 ms；NEMA 单独开启降低 16.67 个 CPU 百分点、增加 11 FPS、减少 8 ms；双开降低 18.67 个 CPU 百分点、增加 12.33 FPS、减少约 8.33 ms。

## 6. 七个手表代理场景

单元格格式为 `CPU% / FPS / Avg. time(ms)`，数值为三次报告的算术均值。

| 场景 | off/off | off/on | on/off | on/on |
|---|---:|---:|---:|---:|
| Multiple ARGB images | 64.7 / 72.0 / 8.7 | 21.3 / 81.3 / 4.0 | 23.0 / 81.3 / 2.3 | 21.3 / 81.3 / 3.0 |
| Rotated ARGB images | 62.7 / 48.0 / 21.0 | 61.7 / 48.3 / 22.0 | 18.0 / 82.0 / 1.0 | 19.0 / 82.0 / 1.0 |
| Multiple labels | 90.0 / 67.0 / 12.0 | 95.3 / 74.7 / 10.0 | 79.0 / 78.0 / 9.7 | 79.0 / 78.0 / 10.0 |
| Multiple arcs | 27.3 / 79.3 / 6.3 | 27.0 / 80.0 / 6.0 | 26.7 / 80.7 / 6.0 | 26.3 / 80.3 / 5.7 |
| Containers with overlay | 82.3 / 19.0 / 46.3 | 56.0 / 39.0 / 23.0 | 81.0 / 62.0 / 13.0 | 72.3 / 76.7 / 10.0 |
| Containers with opa | 45.3 / 75.3 / 13.0 | 36.7 / 79.0 / 10.3 | 24.0 / 82.0 / 3.3 | 24.0 / 82.3 / 4.0 |
| Containers with scrolling | 78.7 / 28.7 / 31.3 | 76.3 / 39.0 / 22.0 | 59.0 / 39.0 / 22.0 | 68.3 / 50.3 / 16.3 |

七场景结论：

- NEMA 对旋转 ARGB、透明容器、标签和 overlay 的收益显著，是当前代理负载中的主要加速能力。
- DMA2D 对普通 ARGB、overlay 和滚动有价值；在 NEMA 已开启时，它仍把 overlay 从约 62 FPS 提升到 76.7 FPS，把 scrolling 从 39 FPS 提升到 50.3 FPS。
- 普通 ARGB 中两种后端存在明显重叠，双开没有获得进一步 FPS 收益。
- Multiple arcs 已接近 12 ms 刷新周期形成的帧率上限，四组差异不足以支持取舍。

基于这七个代理场景，保留 NEMA 的证据充分；DMA2D 的平均增益较小，但对 overlay 和 scrolling 有明确的互补收益。若技术底座需要覆盖这两类交互，保留 DMA2D 的可配置能力是合理的。

全量 16 场景中还需要登记一项七场景之外的回归：`Screen sized text` 在 NEMA 单开时三次均为 31 FPS、30 ms，双开时三次均为 28 FPS、32 ms。双开虽然把 CPU 从平均 83.3% 降到 77%，但吞吐下降约 9.7%，耗时增加 2 ms；这不是重复波动。如果后续真实手表界面包含大面积动态文本，该项必须进入业务负载权重，不能用总体平均值覆盖。`Widgets demo` 也没有因双开改善，均为 21 FPS，平均时间从 28 ms 增至 30 ms。

## 7. 指标解释与使用边界

- `Avg. CPU` 回答当前场景下 CPU 忙碌比例和剩余余量。不同配置 FPS 不同时，CPU 百分比不能脱离 FPS 单独比较。
- `Avg. FPS` 回答平均刷新吞吐。当前刷新周期为 12 ms，理论上限约 83.3 FPS；79–82 FPS 已进入上限区间。
- `render time` 回答 LVGL 在官方统计口径下完成绘制阶段的平均耗时，可定位绘制后端收益。
- `flush time` 回答提交显示缓冲区的平均耗时。当前 `LV_ST_LTDC_USE_DMA2D_FLUSH=0`，因此 flush time 不能解释成 DMA2D 的独立运行时间。
- `Avg. time` 是 `render time + flush time`，不包含完整的等待、调度和输入响应时间。
- `All scenes avg.` 是各有效场景平均值的等权平均，并使用整数除法；它适合做全套 benchmark 概览，不是业务加权得分。实现见 [`lv_demo_benchmark.c`](C:/Users/86151/Desktop/LVGL/u5a9_lvgl/ThirdParty/LVGL/demos/benchmark/lv_demo_benchmark.c:582) 和 [`summary_create`](C:/Users/86151/Desktop/LVGL/u5a9_lvgl/ThirdParty/LVGL/demos/benchmark/lv_demo_benchmark.c:758)。

## 8. 已知限制与归档要求

原始数据文件没有逐次记录复位方式、异常日志和任务栈最低余量。12 次均完整到达报告页且结果高度稳定，支持本轮数据级验收；如果其中任何一次实际触发了 `vApplicationStackOverflowHook`、HardFault、画面错误或持续的未对齐警告，则该次必须作废并单独补测。

后续实验不需要增加板端 CSV 代码。只需在串口原文之外记录组别、重复编号、ELF SHA-256、是否冷/热启动及异常状态，即可补齐追溯链。

验收依据包括 LVGL 9.3.0 当前工程中的 benchmark 统计实现，以及 [NIST/SEMATECH Measurement Process Characterization](https://www.nist.gov/publications/nistsematech-engineering-statistics-handbook-chapter-2-measurement-process) 对重复性、稳定性和测量不确定性的原则。DMA2D flush 的口径参考 [LVGL 9.3 STM32 LTDC driver documentation](https://docs.lvgl.io/9.3/details/integration/driver/display/st_ltdc.html)。
