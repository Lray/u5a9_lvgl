# M5 日志（外部存储、性能设施与合成基准）

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
