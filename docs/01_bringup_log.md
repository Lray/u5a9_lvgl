# M0 Bring-up Record

Date: 2026-08-23  
Target: STM32U5A9J-DK / STM32U5A9NJH6Q / Cortex-M33  
Scope: version and hardware gates, reproducible build skeleton, FreeRTOS/FPU/TIM2 smoke only; no display or graphics accelerator is initialized.

## Result

M0 is complete. The locked bundle build, artifact audit, option-byte readback, pre-scheduler HAL timebase check, and uninterrupted 10-minute FreeRTOS/FPU burn-in all passed. The board run uses the project's original STM32U5 HAL `V1.6.2`, not a STM32CubeU5-aligned HAL. The firmware used for the board run is `build/u5a9_lvgl.hex`, build ID `m0-48772f387863-dirty-graphics-safe`.

## Locked sources

| Item | Locked identity |
|---|---|
| Target repository baseline | `48772f387863` |
| LVGL | `v9.3.0`, `c033a98afddd65aaafeebea625382a94020fe4a7` |
| Nema no-OS backport | upstream-equivalent to `ff620cafc41737ed55de390abeb1fa79cb024f63`; local patch SHA-256 `209994e3ba9effd63e71a49f6fbc020c30b16a675cba77b7b0bba9aa9c692dd9` |
| FreeRTOS | Kernel `V10.4.6`; Riverdi reference source tree `11d12a36d71adeff49f4bd869a442bc8a0534a79`; local 35-file tree digest `c99c5f87ca7076f3b3297d84841b99f4a68e24e2bb31c2ac2f7037e9a1526197` |
| STM32CubeU5 | `88763fb11dfa79178e5410f37e73dbca7d8db39e` / package `FW.U5.1.9.0` |
| STM32U5 HAL | 工程原始 HAL `V1.6.2`；不与 STM32CubeU5 主仓版本对齐 |
| STM32U5 CMSIS device | `624374fa1e21ca195d6f2102ac0caaa50d0ea4c8` |
| STM32U5x9J-DK BSP | `v1.3.1`, `a5723a4ff404cb659b3c095d59527a0b0c2062f0` |
| APS512XX component | `80c1489164d737dabff0287a95f39ca1cf31b254` |
| MX25UM51245G component | `2fdc96a372222d6a3e47fb84263e176636a68972` |
| Sitronix component | `8fbda4431bd823a4c2573c79273921aefbf4ce50` |

`cmake/check_source_locks.cmake` recomputes the source-tree digests at configure time. HAL is locked to the project's original V1.6.2 baseline; the unused CMSIS `Source/Templates/gcc/linker` set is excluded; BSP components retain only driver sources, configuration templates, and licenses. The Nema patch is stored but is not applied or compiled in M0; configure runs `git apply --check` against the locked LVGL worktree.

Board facts are frozen as STM32U5A9J-DK, BSP board ID `MB1829`, APS512XX 512-Mbit HSPI PSRAM, MX25UM51245G OSPI NOR, and Sitronix touch. The attached ST-LINK electronically identifies the board as `STM32U5A9J-DK`; PCB assembly revision is not exposed by the probe and therefore is not inferred.

## Locked host tools

| Tool | Version / absolute source | SHA-256 |
|---|---|---|
| CMake | `C:\Users\86151\AppData\Local\stm32cube\bundles\cmake\4.3.1+st.1\bin\cmake.exe`, 4.3.1 | `f05482595d42888f2befe209d8aa4848560c8a05356411043241a15e7d3f86a7` |
| Ninja | `C:\Users\86151\AppData\Local\stm32cube\bundles\ninja\1.13.2+st.1\bin\ninja.exe`, 1.13.2 | `e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8` |
| GNU Arm GCC | `C:\Users\86151\AppData\Local\stm32cube\bundles\gnu-tools-for-stm32\14.3.1+st.2\bin\arm-none-eabi-gcc.exe`, 14.3.1 | `c8fcafea64559054bbfa87917182598892f81b41706b003c5a93fa7542355908` |
| GNU Arm GDB | STM32 bundle `gnu-gdb-for-stm32/14.3.1+st.2`, 15.2.90.20241229-git | `ce0c9ebacd39c21edb4c3ded32d3f005eb7e93964394b2c04a6bc2fc24d83afc` |
| STM32CubeProgrammer | STM32 bundle `programmer/2.23.0`, 2.23.0 | `269db404ed3f2cdd7cfcccc257ab0a907f2364c3d44e39edab6b007cb5f668ed` |
| ST-LINK GDB Server | STM32 bundle `stlink-gdbserver/7.14.0+st.2`, 7.14.0 | `aea17a5e497ac37a424d76e36bdfe0a1b46a6ca59a0114f381080d2a231d12ae` |
| STM32CubeMX | `E:\STM32CubeMX\STM32CubeMX.exe`, file version `>6.15.0-RC4` | `67b12576eed1c9544668b529458d87e6a9db600cf53492da1534afa8cf1ca0b8` |

The top-level CMake checks the exact CMake, Ninja and GCC locations, versions and hashes. It also requires `CUBE_BUNDLE_PATH`, Ninja, and the locked toolchain file. Two negative gates were exercised:

- The ordinary host environment resolved Nordic CMake 3.21.0 / Ninja 1.10.2 and no Arm GCC; configure was rejected.
- Windows PowerShell 5.1 rejected the literal `&&` command at parse time; the acceptance command was therefore run unchanged through `cmd.exe /d` in the activated bundle environment.

## Build and artifact gates

Acceptance command, executed twice in the same locked shell:

```bat
cmake -S . -B build && cmake --build build
```

The clean build completed 34 steps with zero errors and zero warnings. The second build reported `ninja: no work to do`. Post-link audits passed:

- ELF is 32-bit Arm EABI5, Cortex-M33, hard-float, FPv5 single precision; entry is `Reset_Handler`.
- Compile/link flags include `-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb`.
- `SVC_Handler`, `PendSV_Handler`, and `SysTick_Handler` each have one strong definition from the FreeRTOS CM33 non-secure port. `TIM2_IRQHandler` has one strong definition from the HAL timebase. `Core/Src/stm32u5xx_it.c` owns none of the three RTOS exceptions.
- No Riverdi display, DSI, LTDC, DMA2D, GPU2D, GFXMMU, cache, LVGL, or Nema source is linked into M0.
- FreeRTOS mode is compile-time locked to FPU=1, FreeRTOS MPU wrapper=0, TrustZone=0, secure-only=0, 64-bit runtime counter, application heap, and both runtime-stat hooks.

Artifact sizes:

| Artifact/region | Size |
|---|---:|
| ELF `.text` | 22,224 B |
| ELF `.data` | 184 B |
| ELF `.bss` | 156,096 B |
| Flash image usage | 22,412 B |
| SRAM3 application usage | 8,824 B |
| `.freertos_heap` at `0x20150000` | 131,072 B |
| `.sram3_guard` at `0x2019C000` | 16,384 B |

The linker models SRAM1, SRAM2, SRAM3, SRAM5, and SRAM4 as separate windows. It reserves the future framebuffer, Nema, LVGL, DMA, trace, and guard sections and asserts their size/address constraints in the link itself.

## Board and option bytes

Probe information:

| Field | Value |
|---|---|
| ST-LINK serial | `003C00333532510E31333430` |
| ST-LINK firmware | `V3J17M11` |
| Target voltage | 1.79-1.80 V |
| Device ID / revision | `0x481` / `0x3001` (Programmer: Rev X) |
| Flash | 4 MiB |
| CPU / clock | Cortex-M33 / 160,000,000 Hz |
| MPU data regions | 8 |

The original option bytes already matched the required safe state, so they were not needlessly reprogrammed. Pre- and post-reset values are identical:

| Field | HAL/Programmer decode | Raw evidence |
|---|---|---|
| TZEN | disabled | `FLASH_OPTR.TZEN = 0` |
| SRAM2 ECC | disabled | `FLASH_OPTR.SRAM2_ECC = 1`, mask `0x01000000` |
| SRAM3 ECC | disabled | `FLASH_OPTR.SRAM3_ECC = 1`, mask `0x00800000` |
| Full `FLASH_OPTR` | pass | `0x7FEFF8AA` |
| HAL `USERConfig` | pass | `0x7FEFF800` |

This confirms the HAL's counter-intuitive encoding: a set SRAM2/SRAM3 ECC option bit means ECC disabled. CubeProgrammer and the boot-time read-only diagnostic agree. Because no option value changed, a hardware reset and fresh boot were used; no speculative option-byte write was performed.

The verified image (`21.89 KiB`) was downloaded through the board ST-LINK, read-back verified, and reset before the uninterrupted burn-in.

## RTOS and FPU board test

Pre-scheduler:

- `HAL_Delay(100)` advanced TIM2/HAL by 101 ticks.
- DWT measured approximately 16.16 million cycles at a 160 MHz system clock, inside the ±2% gate.

Post-scheduler, uninterrupted 10-minute run (the acceptance checks below were evaluated before the debug probe connected):

| Check | Final value | Result |
|---|---:|---|
| Scheduler / idle hook | started / seen | pass |
| Heartbeat half-periods | at least 1,200 before the 10-minute gate (1,254 at final read) | pass |
| 1-second tick windows | at least 600 before the 10-minute gate (627 at final read) | pass |
| HAL/FreeRTOS non-monotonic or >1-tick drift windows | 0 when the gate was evaluated | pass |
| Maximum HAL-vs-RTOS 1-second drift | no more than 1 tick when the gate was evaluated | pass |
| High-priority FPU S16-S31 iterations/errors | 627,022 / 0 | pass |
| Low-priority FPU S16-S31 iterations/errors | 313,511 / 0 | pass |
| HardFault / stack overflow / malloc failure / assertion | 0 / 0 / 0 / 0 | pass |
| Runtime-stat samples/non-monotonic errors | 627 / 0 | pass |
| Runtime task count | 4 (three application tasks + idle) | pass |
| Firmware 10-minute aggregate flag | 1 | pass |

Both FPU tasks write distinct bit patterns to S16-S31, explicitly yield, delay across further context switches, read the registers back, and compare all 16 words. Disassembly was inspected to ensure the write/yield/delay/read sequence is present. Runtime statistics use an unsigned-wrap DWT extender and a `uint64_t` FreeRTOS counter. The final counter was 100,320,147,963 cycles (627.00092 s at 160 MHz); the sum of the four task counters was 100,320,008,471 cycles, leaving a 139,492-cycle/0.872-ms in-flight sampling gap.

### Debug-probe measurement note

STM32CubeProgrammer `HOTPLUG` memory reads are invasive on this target: while connected, the external NVIC enable state is suspended and TIM2 stops servicing updates even though the timer continues counting. A deliberately probed run therefore recorded false HAL-tick drift after the first snapshot. A clean 30-second A/B run proved the distinction: before the first connection, `uwTick=45140` and FreeRTOS `xTickCount=45149`; only the debugger connection then exposed the suspended NVIC state. That run was discarded.

The accepted 10-minute test was reflashed/reset and left completely unobserved for the full interval. The firmware set its internal 10-minute pass flag before one final snapshot was taken. That snapshot confirms `ten_min_pass=1`, all FPU/fault/runtime counters clean, and the expected 160 MHz/MPU/option-byte values. It also contains one non-monotonic tick window and a 102-tick drift record, created after the flag was set by the invasive `HOTPLUG` attachment; this is not an acceptance failure. The acceptance predicate had already verified zero such windows and a maximum drift of one tick before latching the pass flag.

After the accepted snapshot, ST-LINK GDB Server 7.14.0 attached to the running image and GNU Arm GDB placed a hardware breakpoint on `vApplicationIdleHook`. The target continued, hit the breakpoint once at `0x08004fb8`, reported the expected PC/xPSR, and detached cleanly. This closes the separate symbol/attach/breakpoint debug gate without contaminating the timed run.

## M0 boundary

M0 deliberately does not initialize the display, external memories, touch controller, GFXMMU, LTDC, DSI, DMA2D, GPU2D/NeoChrom, ICACHE, or DCACHE2. M1 must start from this passing baseline and add only the DSI/LTDC single-buffer display path described in `docs/00_plan.md`.
