# M5 日志（外部存储、性能设施与合成基准）

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
