# M5 日志（外部存储、性能设施与合成基准）

## 阶段 6：M5 收口（2026-08-26）——正式基准、长稳、扰动量化、未测试项声明

### NOR CRC 链路自检（上板验证 #1 的 NOR 部分）
- `platform/memory/ospi_nor.{c,h}`：标准 CRC-32（nibble 表，**主机 Python/zlib 独立对照验证实现正确**）+ 启动自检读 NOR 首 64 KiB 计算 CRC + 非零字数统计
- 板上结果（`g_mem_probe` 扩展）：`nor_crc32=0x1C1C6A9A`（两轮稳定）、**`nor_nonzero_words=0x39B2`（16,384 字中 14,770 非零 ≈90%）→ 板上 NOR 出厂含数据**（此前仅验证前 16 B 为零）；CRC 对真实内容正确
- 说明：基准资源采用程序生成图案（计划 §7 M5 回退方案允许"资源先改用内部Flash/生成图案"）；正式资源 manifest 管线（offset/size/CRC）留待真实资产阶段

### LV_SYSMON_GET_IDLE → FreeRTOS provider（计划验收项）
- `freertos_runtime_stats.c` 新增 `perf_idle_percent()`：`ulTaskGetIdleRunTimeCounter / perf_runtime_counter64`；`lv_conf.h` `LV_SYSMON_GET_IDLE` 指向它；m3_snapshot 直读（CSV idle 字段语义=整机 FreeRTOS idle）

### 正式基准运行（10 s warm-up + 60 s sample ×3，归档 docs/perf_results/）

| 场景 | FPS (x̄) | swaps/s | scan Hz | idle% | lvmax B | 错误 |
|---|---|---|---|---|---|---|
| full_repaint | 29.5 | 29.4 | 78.1 | 31.7 | 7408 | 0 |
| transform | 32.8 | 29.5 | 78.4 | 32.1 | 11348 | 0 |
| alpha | 2.96 | 2.9 | 77.6 | 0.0 | 8256 | 0 |
| text | 63.2 | 29.4 | 78.2 | 31.0 | 8264 | 0 |
| mixed | 68.6 | 28.3 | 78.6 | 16.0 | 12184 | 0 |

三次采样逐项几乎一致（确定性场景，可重复性极好）；transform_iter3 因主机串口中断仅 33 s（标注）。Nema 统计全程 allocs=2/hwm=2/fails=0。

### 长稳（10-min 用户门限）
`mixed_soak1.csv`：dt=609 s、620 行（1 Hz 全命中）、fps 65.4、scan 78.59 Hz、**四类错误全零、Nema 零失败**、无复位。

### monitor on/off 扰动量化（full_repaint，62 s 各一）
- off 构建：独立 `build-off` + `CMAKE_C_FLAGS_DEBUG` 注入 `-DLV_USE_PROFILER=0 -DLV_USE_SYSMON=0 -DLV_USE_PERF_MONITOR=0`（工具链文件改累积语义支持）；compile_commands 证实宏注入
- 结果：**FPS 29.45 vs 29.53（<1%）**、idle 32.9% vs 31.7%（≈1 pp）、lvmax 6288 vs 7408（省 1,120 B 观测对象，证明 off 生效）
- 结论：观测设施扰动 ≈1% CPU、<1% FPS——低扰动设计成立

### 未测试项声明（计划文本允许"没有实现就明确记为未测试"）
1. **XRGB8888 单变量**：未实现。需 ARGB8888 LUT + FB 720 KiB 布局 + lv_conf 32bpp 全套切换；RGB565 正式基线保持（计划回退方向）
2. **PSRAM cacheable 单变量**：未实现。需 DCACHE1 使能 + 维护闭环（cache.c 未建）+ MPU attr 切换；safe 基线 PSRAM non-cache 已冻结
3. **受控并发**：未测试。共享 arbiter/dependency/fence 未实现；正式串行路由保持
4. **NOR 正式资源 manifest**：未实现（见上，回退方案允许生成图案）

### 构建/验收状态
- 固定命令默认构建零警告（497 目标，FLASH 12.49%）；五场景构建零警告（502 目标）
- 默认固件已恢复 mixed；build-bench/build-off 已清理

## 阶段 5：合成基准场景框架（2026-08-26）

### 实现
- `benchmarks/bench_runner.{c,h}`：`Bench_Scene_Setup/Step` 分发，`BENCH_SCENE` CMake 选项
  （mixed|full_repaint|transform|alpha|text，默认 mixed=既有 demo 行为原样保留）
- 场景文件（plan §5.3 矩阵）：
  - `bench_full_repaint.c`：每帧强制 100% dirty 全屏色块
  - `bench_transform.c`：80×80 程序生成彩虹格图，旋转 30°/1.5× + 平移动画（Nema 层路径，自 M4 验证场景抽取）
  - `bench_alpha_layers.c`：4 层全屏 30% alpha 叠加，alpha 周期扰动
  - `bench_text_scroll.c`：montserrat_14 固定字符串横纵滚动（局部 dirty）
  - `bench_mixed.c`：原 M3/M4 demo 原样迁移（rect+alpha+label+transform img+25 s 相位 100% dirty 交替）
- `app_freertos.c` 瘦身为任务胶水：init/自检 → `Bench_Scene_Setup` → 循环 `Bench_Scene_Step`+`lv_timer_handler`+1 s 快照
- 统计复用现有 CSV 遥测；warmup/sample 分段由主机按 CSV `up_ms` 时间窗处理（10 s/60 s×3 语义）

### 上板证据（m5_bench_build.cmd 参数化构建，每场景 UR 烧录 + COM7 抓取 12 s）

| 场景 | FPS(lvgl_frames Δ) | idle% | 特征 |
|---|---|---|---|
| mixed（回归） | ~30（相位相关） | 4-56 | 与抽取前完全一致（帧/swap/错误全同） |
| full_repaint | ~29.7 | ~57 | 100% dirty 全屏 SW 重绘 |
| transform | ~33 | ~70 | Nema 层路径单图变换 |
| alpha | ~3.3 | 0 | 4 层全屏 alpha 读改写带宽压力 |
| text | ~68 | ~35-45 | 局部 dirty 滚动文字 |

全部场景：scanout 78.5 Hz 恒定、swap submit/done 同步、四类错误零、Nema/DMA2D 统计正常。
五场景构建均零警告（502 目标）。默认固件已恢复 mixed。

## 阶段 4：PSRAM 64 MiB 全覆盖诊断（2026-08-26）

### 实现
`Hspi_Psram_FullDiag()`（`platform/memory/hspi_psram.c`，`PSRAM_FULL_DIAG=ON` 编译门控，默认 OFF，
CMake `option(PSRAM_FULL_DIAG)`，对应计划 §7 M5 上板验证 #1）：
- 地址模式全 64 MiB 写+读回（`0xA5965A3C^i`）
- 4 种数据模式（`0x00/FF/A5/5A`）各全量一遍
- 256 KiB 窗口 walking-bit 1s/0s 各 32 位模式
- DWT 计时（`diag_ms`），逐子测试 `diag_ok` 位掩码 + 首个失败地址

### 上板证据（build-diag 独立目录，固定命令同构模板 m5_diag_build.cmd）

```
g_mem_probe: 4D454D31 00000008 000000FF 0000E444 00000000
             00000001 00000001 00000001 00000001 001111A4
             00000001 0000007F 00000000 000036C3 00000001
```
- `diag_run=1`、**`diag_ok=0x7F`（7/7 子测试全过）**、`diag_fail_addr=0`
- **`diag_ms=0x36C3 = 14,019 ms`**（全量 64 MiB 多模式遍历）
- 诊断后系统照常：CSV 1 Hz 出流、scanout 78.5 Hz、DMA2D 零错误、Nema 统计一致
- 验收后已恢复默认固件（diag 默认关闭，不影响常规启动）

## 阶段 3：VCP CSV 遥测 + Nema allocator 统计包装（2026-08-26）

### 实现

| 文件 | 内容 |
|---|---|
| `Drivers/…/stm32u5xx_hal_uart{,_ex}.{c,h}` | 官方 FW_U5_V1.8.0 包同源复制（HAL V1.6.2）；hal_conf 启用 `HAL_UART_MODULE_ENABLED` |
| `Src/usart1.{c,h}` | 方式 A：内核时钟显式 SYSCLK、PA9/PA10 AF7、921600-8N1 TX-only、MSP 自含（regen 安全）、无 IRQ（轮询发送） |
| `platform/perf/perf_uart.{c,h}` | `perfUart` 线程（BelowNormal/2 KiB）：1 Hz 快照 `g_m3_stats`+DMA2D+Nema 统计→单行 CSV→`HAL_UART_Transmit` 100 ms 超时；drop 计数 |
| `Inc/app_stats.h` | `m3_snapshot_t` 从 app_freertos.c 提升共享（写读两侧同头文件防漂移） |
| `platform/perf/perf_nema_alloc.{c,h}` + `-Wl,--wrap=tsi_malloc_pool/tsi_free` | 固定表统计。事实修订：vendor glue 经 `tsi_malloc(size)` **宏**转发到 `tsi_malloc_pool(0,…)`，wrap 点与计划一致 |

CSV 字段序：`seq,up_ms,lines,lvgl_frames,sw_sub,fb_err,dsi_err,ltdc_err,gfxmmu_err,dma_disp,dma_err,n_allocs,n_frees,outst_hwm,fails,idle_pm,lv_used_max`

### 上板证据（COM7 ST-LINK VCP 抓取 12 s）

```
39,39812,3196,2209,1104,0,1,0,0,9103,0,2,0,2,0,48,12184
...
51,51895,4139,3318,1455,0,1,0,0,12276,0,2,0,2,0,5,12184
```
- 12 s 恰 13 行（1 Hz 精确、零丢行）
- lines 增速 78.5 Hz 标称；`dsi_err=1` 为 M1 已记录的启动期一次性 ACK（恒定不增长）
- **map 证明 wrap 生效**：`__wrap_tsi_malloc_pool@0x0801109c`、`__wrap_tsi_free@0x0801110c`；
  运行时 allocs=2/frees=0/outst_hwm=2（Nema 仅初始化期分配 context+ring，符合预期），fails=0
- DMA2D dispatch 推进、error=0；显示管线无扰动

### 统计口径说明（诚实边界）

`tsi_free` 无 size 参数，字节级 current 无法由 wrapper 精确维护；表输出
allocs/frees/outst(计数)/outst_hwm/fails + bytes_cum。字节级高水位需 allocator 内部信息（开放项）。

## 阶段 2：MPU 落地 + PSRAM 资源 arena（2026-08-26）

### 权威参考来源（回应"仓库无 MPU 参考"）

工作区/Riverdi/本板官方示例均无 MPU 配置先例（U5x9J-DK 示例全部裸跑默认映射）。采用同官方包
`STM32Cube_FW_U5_V1.8.0\Projects\STM32U575I-EV` 的权威模式：
- `Examples\BSP\Src\main.c` / `DMA2D_BlendingWithAlphaInversion`：Disable→MAIR→region(Base/Limit-1)→
  `HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT)`；
- `Examples\DCACHE\DCACHE_Maintenance`：缓存属性组合写法 `MPU_NOT_CACHEABLE | OUTER(policy…)`
  （**外 nibble 承载缓存策略**，内 nibble 固定 NC——照抄，不自创）。
计划 §4.6 region 表为冻结规格；`platform/memory/mpu.c` 仅做表→HAL API 的机械映射。

### 与计划的偏差（纪律 #6 记录）

§4.6 region7 要求 "no-access guard"，但 Armv8-M PMSA AP 编码仅 RW/RO × priv/all 四种，
**no-access 硬件不可表达**。修订：region7 以 `PRIV_RW+XN+NC` 钉住属性（防止未来 DCACHE1 缓存该窗口）；
硬防护仍由既有 linker ASSERT（`.sram3_guard` 区禁入）与 MSPLIM/任务栈检查承担。

### 实现

| 文件 | 内容 |
|---|---|
| `platform/memory/mpu.{c,h}` | DREGION==8 检查；MAIR0=attr0(NC 0x44)/attr1(内NC+外WB-R-A 0xE4)；8 regions：FB0/FB1/GFXMMU virtual/Nema pool/DMA staging=ALL_RW XN inner-shareable NC、OSPI NOR=ALL_RO XN(attr1)、PSRAM safe=ALL_RW XN NC、guard=PRIV_RW XN；`HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT)`；逐 region RBAR/RLAR readback |
| `Inc/mem_probe.h` + `g_mem_probe @0x201752EC`(map 核实) | 启动探针：magic/dregion/rb 掩码/MAIR/arena 结果/done |
| `platform/memory/hspi_psram.{c,h}` | PSRAM arena @0xA0000000（32 MiB 预算，§4.5）：32-B 对齐分配、rounded-size、头 magic/canary + 尾 canary、bump 语义 + high-water；上电自检：数据 pattern、对齐断言、跨分配 canary 完整性 |

main.c：`MX_MPU_Config()` 入 USER CODE SysInit（先于 ICACHE/GFXMMU/XSPI），`Hspi_Psram_ArenaInit()` 入 USER CODE 2 尾。

### 上板证据（UR 烧录 -v → 复位 → HOTPLUG 干净读）

```
g_mem_probe: 4D454D31 00000008 000000FF 0000E444 00000000
             00000001 00000001 00000001 00000001 001111A4 00000001
```
- dregion=8 ✅（计划硬闸门）；mpu_rb_ok=0xFF ✅（8/8 region base/limit/AP/XN/SH/AttrIndx 全匹配）
- MAIR0=0xE444 ✅（attr0=NC、attr1=WB-R-A）；arena init/data/align/canary 全 1 ✅；hwm≈1.07 MiB 合理
- `g_xspi_probe` 全绿不变（存储自检在 MPU 下复测通过）
- 显示管线活性：10 s 内 line events +8,276（79.1 Hz 标称）、swap submit/done 差恒 1（不变式保持）、错误计数全零

### 构建

固定命令全新 build：491 目标 **零错误零警告**，FLASH 12.35%。
正式 10-min 门限随基准设施阶段统一执行（本轮为基础设施 bring-up）。

## 阶段 1：存储接入（方式 A，2026-08-26）

### 前置阻塞解除：XSPI HAL 来源

HANDOFF 记录 `stm32u5xx_hal_xspi.{c,h}` 不在工程/STM32CubeU5 克隆/Riverdi 三处。本轮在
**`C:\Users\86151\STM32Cube\Repository\STM32Cube_FW_U5_V1.8.0\`**（CubeMX 官方固件包）找到，同包还有其硬依赖
`stm32u5xx_ll_dlyb.{c,h}`。来源一致性证据：

- 该包 HAL 版本 = **V1.6.2**（`stm32u5xx_hal.c` 版本宏），与工作区 HAL 完全一致；
- 工作区 `stm32u5xx_hal_gfxmmu.c/h`、`stm32u5xx_hal_def.h` 与该包逐字节相同（SHA-256）；
- 工作区 CMSIS Device `stm32u5a9xx.h` 与该包逐字节相同，且早已含 XSPI/HSPI 全套寄存器定义
  （HSPI1_IRQn=131、HSPI1 基址 0xA0000000）——M4 的 `gpu2d.c` 内容亦与该包一致（仅行尾差异）。

结论：工作区 HAL 本就源自该官方包，xspi/ll_dlyb 为同一来源补齐，无版本混用。

### 方式 A 实现（不碰 .ioc）

BSP 两驱动（`stm32u5x9j_discovery_hspi.c/_ospi.c`）自持句柄/MSP/DMA/DLYB，`MX_HSPI_RAM_Init`/
`MX_OSPI_NOR_Init` 为 `__weak`；项目侧最小接入：

| 变更 | 内容 |
|---|---|
| `Inc/stm32u5xx_hal_conf.h` | 启用 `HAL_XSPI_MODULE_ENABLED`；新增 `USE_HAL_XSPI_REGISTER_CALLBACKS=0U` |
| BSP conf 落地 | `stm32u5x9j_discovery_conf.h`（最小集）、`aps512xx_conf.h`、`mx25um51245g_conf.h`（模板复制，include 改法与官方 BSP 示例一致） |
| `Src/hspi1.c/h` | 内核时钟显式 `RCC_HSPICLKSOURCE_SYSCLK`(160 MHz)→`BSP_HSPI_RAM_Init`(FIXED latency 7/7、linear burst 64B、x16 = 官方 BSP 示例 mode4)→mmap→双窗口(0/+8 MiB×64 字)p pattern 自检 |
| `Src/octospi1.c/h` | 内核时钟 SYSCLK→`BSP_OSPI_NOR_Init`(OPI+DTR)→mmap→读 0x90000000 首两字 |
| `Inc/xspi_probe.h` + `g_xspi_probe @0x201752EC`(map 核实,0x28 B) | 探针结构，HOTPLUG 读回验收 |
| CMake | xspi+dlyb 入 STM32_Drivers（GPU2D 先例位置）；4 BSP TU + hspi1/octospi1 入顶层 |
| `Src/main.c` USER CODE 2 | `MX_HSPI1_Init(); MX_OCTOSPI1_Init();` |

内核时钟事实：`.ioc` 仅含信息性 `HSPI1Freq_Value/OCTOSPIMFreq_Value=160000000`；复位默认选择即 SYSCLK，
现以 `HAL_RCCEx_PeriphCLKConfig` 显式固定并可通过 readback 审计。

### 上板证据（build id 见 git 提交；ST-LINK SN 003C…3430，1.79 V）

最终探针读回（UR 烧录 -v 通过 → 复位 → HOTPLUG `-r32 0x201752EC 0x28`）：

```
58535031 00000000 00000000 00000000
FFFFFFFF 00000000 00000000 00000000
00000000 00000001
```

- PSRAM（APS512XX）：init=0 / mmap=0 / **pattern 自检通过**（cmp=0，misoff=0xFFFFFFFF）
- NOR（MX25UM51245G）：init=0 / **mmap 首试成功** / word0=word1=0x00000000 / done=1

### 发现与处置（重要，勿回归）

1. **组件驱动 `MX25UM51245G_ReadID` 在 OPI-DTR 下接收短读**：板上实测返回 `C2 80 00`
   （第三字节丢失），且 ReadID 返回 OK 后 XSPI SR 残留 `FTF|BUSY|FLEVEL=1`——传输未真正结束。
   后续任何 CFG Command 等 BUSY 超时（ErrorCode=TIMEOUT），导致 `EnableMemoryMappedMode`
   返回 -5（BSP_ERROR_COMPONENT_FAILURE）。Abort 后重试即可成功（已实测）。
   **处置**：启动路径不含 ReadID（Init→mmap 直通，两轮冷启动均首试成功）；器件身份以
   UM2967/BSP 冻结事实为准。后续若需 ID/SFDP，须走间接窗口并处理该短读问题（开放项）。
2. **NOR 数据路径可信度交叉验证**（诊断轮，已从代码移除）：同一地址三路读取——mmap CPU 读 /
   SPI-STR 间接读（DOPI→SPI 切换）/ OPI-DTR 间接读——16 字节逐位一致（全零）。
   证明 OPI-DTR+mmap 读取正确；NOR 起始区真实内容为 0x00 填充（非空片 FF）。
3. HOTPLUG 活读偶发单字伪影（M1 已记录侵入性）：关键判定一律复位后干净读。

### 构建/验收状态

- 固定命令（Bundle 终端 cmd.exe /d）：489 目标 **零错误零警告**，FLASH 12.30%
- 连续运行门限（10 min）：本阶段为存储 bring-up，未跑显示长稳（M4 基线未动，LVGL 场景照常运行）；
  正式 M5 长稳随基准设施阶段一并执行
