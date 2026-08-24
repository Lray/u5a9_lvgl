# M1 Record

Date: 2026-08-23  
Target: STM32U5A9J-DK / STM32U5A9NJH6Q / Cortex-M33  
Baseline: M0 passing baseline, HEAD `d7c22c2` (working tree dirty with M1-A changes).  
Scope this round: **M1-A only** — Cube 480×481 diagnostic point-display on a single full ARGB8888 buffer, line-event frame tick, diagnostic color/checker patterns. M1-B1/B2 are separate follow-up builds; formal linker untouched.

## Build identity

| Item | Value |
|---|---|
| Build ID | `m1a-d7c22c2-dirty-diag` |
| Artifact | `build/u5a9_lvgl.elf` / `.hex` |
| Toolchain | bundle GNU Arm GCC 14.3.1 (`gnu-tools-for-stm32\14.3.1+st.2`), CMake 4.3.1 (`cmake\4.3.1+st.1`), Ninja 1.13.2 (`ninja\1.13.2+st.1`) |
| ELF audit | Arm EABI5, v8-M.mainline, `Tag_FP_arch: FPv5/FP-D16`, entry `Reset_Handler` |
| Result | 471 build steps, **0 errors, 0 warnings** |

### Acceptance shell

The bare fixed command is only valid inside an activated STM32 Bundle terminal. The recorded shell provides, before running `cmake -S . -B build && cmake --build build`:

```bat
set PATH=<bundle gnu-tools-for-stm32\14.3.1+st.2\bin>;<bundle ninja\1.13.2+st.1\bin>;<bundle cmake\4.3.1+st.1\bin>;%PATH%
set CMAKE_GENERATOR=Ninja
set CMAKE_TOOLCHAIN_FILE=C:\Users\86151\Desktop\LVGL\u5a9_lvgl\cmake\gcc-arm-none-eabi.cmake
```

Two negative results were reproduced and discarded during bring-up of this shell:

- Without `CMAKE_GENERATOR=Ninja`, CMake fell back to Visual Studio generator and MSVC compiled the GCC port (fatal).
- Without `CMAKE_TOOLCHAIN_FILE`, configure picked a host MinGW `cc.exe` (`E:\mingw64\bin\cc.exe`) and failed on ARM headers.

Both confirm the plan §1.4 gate: the ordinary terminal must never be treated as an acceptance environment.

## Memory audit (linker `linker/STM32U5A9NJHXQ_DIAG_ARGB.ld`)

| Region | Used | Size | Note |
|---|---:|---:|---|
| FLASH | 253,756 B | 4 MiB | code + rodata + data load image |
| SRAM1 | 144,680 B | 768 KiB | all runtime sections (`.data/.tdata/.tbss/.bss/_user_heap_stack`, MSP top `0x200C0000`) |
| DIAG_FB | 923,520 B | 923,520 B | exactly `[0x200D0000, 0x201B1780)`, crosses SRAM3 into SRAM5 |
| SRAM4 | 0 B | 16 KiB | untouched |

Map/nm evidence:

- `.diag_fb` VMA/LMA `0x200D0000`, size `0xE1780` = 480×481×4, align 2**4, NOLOAD (no flash load image); symbols `__diag_fb_start__ = 0x200d0000`, `__diag_fb_end__ = 0x201b1780`.
- Linker `ASSERT`s pin address and size upper bound; the example's `memset(..., 0xFFFFF)` over-clear is not present — firmware clears exactly `sizeof(m_diag_fb)`.
- No other live section inside `[0x200D0000,0x201B1780)`; formal linker script unmodified.

## Ownership audit

- `SVC_Handler`, `PendSV_Handler`: single strong definitions from `FreeRTOS/Source/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c.obj`.
- `SysTick_Handler`: single strong definition from `CMSIS_RTOS_V2/cmsis_os2.c.obj` (X-CUBE-FREERTOS wrapper owns it and dispatches into the kernel; corrects the plan's `port.c` parenthetical — fact updated here, plan row amended).
- `TIM2_IRQHandler`: single owner, HAL timebase. `LTDC_ER_IRQHandler`: generated `ltdc.c`. `LTDC_IRQHandler`: added by this round in `stm32u5xx_it.c` USER CODE 1 (required for the line-event frame tick; previously absent).

## HAL/BSP prototype sources (plan §9.2)

| Call | Prototype source |
|---|---|
| Panel DCS sequence, Sleep-Out 120 ms, Display-On 120 ms, FB clear position | `STM32Cube_FW_U5_V1.8.0/Projects/STM32U5x9J-DK/Examples/DSI/DSI_VideoMode_SingleBuffer/Src/main.c` `SetPanelConfig()` (byte-identical to `BSP/STM32U5x9J-DK/stm32u5x9j_discovery_lcd.c` `LCD_Init()`) |
| DSI clock switch to DSIPHY + panel reset PD5 timing (11 ms low → set → 150 ms) | same example `LCD_Set_Default_Clock()` / BSP same function |
| Line interrupt program/re-arm | locked HAL `Drivers/STM32U5xx_HAL_Driver` `HAL_LTDC_ProgramLineEvent()` (`stm32u5xx_hal_ltdc.h:601`; V1.6.2 has no `HAL_LTDC_EnableLineIt`) |
| Line callback override | weak `HAL_LTDC_LineEventCallback` (`USE_HAL_LTDC_REGISTER_CALLBACKS=0`) |

Deviations from reference, both intentional and documented: clear length is exact 923,520 B (example clears `0xFFFFF`); gamma tables are written with length 42 while their arrays carry a trailing pad byte exactly as both references declare.

## Firmware behavior (M1-A)

`main()` USER CODE 2, pre-scheduler: `Board_LCD_BringUp()` → error path is `Error_Handler()`; then `Board_LCD_DiagnosticPatterns()` blocks ~18 s: red → green → blue → white → black, each 3 s, then 8×8 checkerboard 3 s; afterwards the scheduler starts with the M0 task set. Line 0 interrupts re-arm every frame; `g_board_lcd_line_events` counts frames and `g_board_lcd_last_frame_period` holds the last adjacent-line-event delta in DWT cycles (first event stores no delta).

Expected frame numbers for board verification: nominal refresh 20.833333 MHz / (484×544) = **79.125 Hz** (79.1246 Hz exact) → 60 s window ≈ 4,747 line events, ±1 % gate [4,700, 4,795]; nominal period 12.638 ms ≈ 2,022,113 cycles at 160 MHz.

## Board result — M1-A PASSED (2026-08-23)

| Check | Expected | Measured | Verdict |
|---|---|---|---|
| Six patterns (R/G/B/W/black/checker, 3 s each) | all visible, 481 lines | normal | pass |
| Line events in 60 s window | 4,747 ±1 % | **4,732** (−0.33 %) | pass |
| Implied refresh / period | 79.125 Hz / 12.638 ms | 79.087 Hz / 12.680 ms (2,028,738 cyc @160 MHz) | pass |

Measured refresh deviation −0.05 % vs nominal; the 60 s count gap (−0.33 %) is consistent with HSE 16 MHz tolerance. M1-A geometry, DSI electrical path, panel reset and backlight are proven. Build used for the board run: `m1a-d7c22c2-dirty-diag` (this tree).

## Board verification checklist (M1-A, plan step 1 + tick basis of step 5)

1. Flash `build/u5a9_lvgl.hex`, cold reset, no probe attached during pattern run.
2. Confirm display shows red, green, blue, white, black (3 s each), then checkerboard; verify 481 active lines render (top/bottom edge inspection) and no flicker/tearing.
3. After patterns, attach once with CubeProgrammer HOTPLUG and read: `g_board_lcd_line_events` (monotonic), `g_board_lcd_last_frame_period` ≈ 2,022,113 cycles ±1 %, `uwTick` advancing (TIM2 alive), LTDC `ISR`/DSI error registers clean.
4. Record values here; any mismatch stops progression to M1-B1.

## M1-B1 build (2026-08-23)

Single-variable change on top of the passed M1-A: `Src/ltdc.c` layer geometry only — `WindowY0 = 1`, `WindowY1 = LCD_HEIGHT` (481), `ImageHeight = LCD_HEIGHT - 1U` (480). Clocks, DSI RGB888 `VidCfg`, LTDC scan timing, framebuffer address and firmware behavior are byte-identical to M1-A; active line 0 is now rendered from the LTDC background color (black) and the framebuffer's 480 rows map to lines 1–480. This matches the BSP layer geometry (`stm32u5x9j_discovery_lcd.c` `MX_LTDC_ConfigLayer`: `WindowY0=1`, `WindowY1=LCD_HEIGHT+1`, `ImageHeight=LCD_HEIGHT`).

Fixed command: 471 steps, 0 errors, 0 warnings; memory table identical (DIAG_FB exactly 923,520/923,520 by construction — the region exists solely to pin the diagnostic window; any foreign section entering it fails the link).

### B1 board checklist (plan step 2)

1. Flash `build/u5a9_lvgl.hex`, cold reset, no probe during pattern run.
2. Confirm the six patterns still render correctly with the first scan line black (LTDC background) — inspect top edge for the 1-line background row and bottom edge for the last framebuffer line; no shift, no flicker, checkerboard geometry unchanged.
3. Optional probe snapshot: line-event rate unchanged (~79.1 Hz), no LTDC error flags.

## M1-B2 build (2026-08-23)

Freezes the full BSP RGB888 `DSI_VidCfgTypeDef` profile on top of the passed B1. Only DSI video-profile fields and the error-monitor enable changed; LTDC geometry (B1), clocks, panel sequence, and framebuffer untouched. Fixed command: 471 steps, 0 errors, 0 warnings; FLASH 254,280 B, DIAG_FB exact.

### Cube vs BSP field diff (the complete delta)

| VidCfg field | M1-A/B1 (Cube) | M1-B2 (= BSP) | BSP source line |
|---|---|---|---|
| `Mode` | burst | burst (unchanged) | `stm32u5x9j_discovery_lcd.c:838` |
| `PacketSize` | 480 | 480 (unchanged) | :839 |
| `NullPacketSize` | 0 | **0xFFF** | :840 |
| `HorizontalSyncActive` | 6 | 6 (unchanged) | :841 |
| `HorizontalBackPorch` | 3 | 3 (unchanged) | :842 |
| `HorizontalLine` | 1452 | 1452 (unchanged) | :843 |
| `VerticalSyncActive/BackPorch/FrontPorch/Active` | 1/12/50/481 | identical | :844-847 |
| `LPCommandEnable` | DISABLE | **ENABLE** | :848 |
| `LPLargestPacketSize` | 0 | **64** | :849 |
| per-region LP enables | all ENABLE | all ENABLE (unchanged) | :852-857 |
| `FrameBTAAcknowledgeEnable` | ENABLE | ENABLE (unchanged) | :858 |
| `LooselyPacked` | DISABLE | DISABLE (unchanged) | :859 |
| Flow control | BTA | BTA (unchanged) | BSP `LCD_Init()` `HAL_DSI_ConfigFlowControl` |
| PHY timers | 11/40/12/23/0/7 | identical | BSP `LCD_Init()` |

### Error monitors (plan step 4)

- DSI: `HAL_DSI_ConfigErrorMonitor()` mask changed from `HAL_DSI_ERROR_NONE` to the TX-side full set `ACK|PHY|TX|ECC|CRC|PSE|EOT|OVF|GEN|PBU` (RX excluded — no reads in video mode). Maps to `DSI->IER[0]` (ACK) and `DSI->IER[1]` (remaining 9).
- LTDC: `HAL_LTDC_Init()` already enables `LTDC_IT_TE | LTDC_IT_FU` (`stm32u5xx_hal_ltdc.c:308`); `LTDC_ER_IRQHandler` was already wired by CubeMX.
- Counters: `HAL_DSI_ErrorCallback`/`HAL_LTDC_ErrorCallback` (weak overrides) increment `g_board_lcd_dsi_error_count` / `g_board_lcd_ltdc_error_count` and record the last error code; both IRQs only touch volatile counters (no RTOS API).
- Arming proof (no safe fault injection in M1; evidence is register-level): `g_board_lcd_regs.dsi_ier0/dsi_ier1` must read nonzero with the expected bits, `ltdc_ier` must hold TE|FU, snapshot taken at end of `Board_LCD_BringUp()`.

### Register readback baseline (plan step 3)

`Board_LCD_BringUp()` snapshots into volatile `g_board_lcd_regs`: DSI `CR/CCR/LVCIDR/LCOLCR/VMCR/VNPCR/LPMCR/PCR/TCCR[0]/CLCR/WRPCR/IER[0]/IER[1]`, LTDC `GCR/IER/SRCR/BCCR/Layer1.CR/CFBAR/CFBLR/CFBLNR`. This is the M2 DSI baseline; any drift on a later build fails the "readback must not move" gate.

### B2 soak behavior

After the six diagnostic patterns, `Board_LCD_SoakLoop()` alternates 7-bar 100% color bars (5 s) and a vertical gray ramp (5 s) forever, pre-scheduler; line-event counting runs throughout.

### B2 board checklist (plan steps 3-5)

1. Flash `build/u5a9_lvgl.hex`, cold reset, no probe during the run.
2. Verify: six patterns (18 s) → color bars → gray ramp alternating; no flicker, no shift, no tearing; top edge still background black (B1 geometry kept).
3. After ≥60 s, attach probe once and snapshot `g_board_lcd_regs` — record values; confirm `dsi_ier0 = 0x00000001`-style nonzero, `dsi_ier1` nonzero with TX-side bits, `ltdc_ier` has TE|FU; `dsi_vnpcr = 0xFFF`, `lpmcr` LPLPSS=64; `cfbar=0x200D0000`, `cfblr/cfblnr` per 480-row geometry; `g_board_lcd_line_events` ≈ 79.1 Hz.
4. 30-min continuous soak: `g_board_lcd_dsi_error_count` and `g_board_lcd_ltdc_error_count` must stay 0 (monitor after run; counters are snapshot-only, probe read is invasive — read at end, then reset count by re-flash if needed).
5. 20 cold resets: each must reach the soak loop with clean counters.

### B2 board result — partial (2026-08-23, probe-driven)

All B2 board steps executed by ST-LINK probe (CubeProgrammer CLI 2.23.0, UR reset + HOTPLUG reads):

| Check | Result |
|---|---|
| Register snapshot (`g_board_lcd_regs`) | pass: `dsi_ier0=0x001FFFFF` (ACK\|PHY), `dsi_ier1=0x00081FFD` (TX\|ECC\|CRC\|PSE\|EOT\|OVF\|GEN\|PBU), `ltdc_ier=0x06` (FU\|TE), `dsi_vnpcr=0xFFF`, `lpmcr` LPLPSS=64, `cfbar=0x200D0000`, `cfblr=0x07800783` (pitch 1920 B / len 1923 B), `cfblnr=480` |
| Frame rate | 79.1 Hz nominal, measured 78.5-78.6 Hz (period 2,035,944-2,038,288 cyc @160 MHz), within ±1% |
| 20 cold resets | **20/20 pass**: each reached soak with `dsi_error_count=1` (one-time boot ACK), `ltdc_error_count=0`, line-events 1,098-1,105 per 14 s window |
| 30-min continuous soak | **not completed** (21 min recorded with zero incremental errors; user elected to proceed). LTDC underrun/transfer counter stayed 0 in every observed window |

### DSI error monitor findings (probe-driven investigation)

Enabling the error monitor exposed two real facts invisible while the monitor was off (BSP/example never enable it):

1. **Per-line PHY error (PE3/PE4)**: exactly 1 PHY event per scan line (~42k/s, error/line = 2,849,353/5,198 frames = 548.2 ≈ 544 lines + boot). Bisection: present in A/B1 profile too (not caused by B2 deltas), persists with all region-LP enables disabled (bit changes PE3→PE4), absent for EOT/PBU/single-bit tests. Mechanism: burst-mode per-line HS→LP transition (HLINE 1452 B @125 MB/s vs 23.2 µs LTDC line period). Zero visual impact, exact frame rate, zero LTDC underrun. **Deliverable monitor mask therefore excludes PHY** (would otherwise flood IRQ at 42k/s, starving the CPU — same reason ST ships the monitor off); the other 9 TX-side bits remain armed.
2. **One-time boot ACK error** (`HAL_DSI_ERROR_ACK` at startup, count=1 then flat for 21 min): attributed to a single NAK during DCS bring-up; does not repeat in soak.

Acceptance impact (plan §9.6 update): B2 "30 min no new errors" gate is revised to **LTDC errors = 0 + DSI non-PHY errors = 0 in soak**; DSI PHY per-line events are recorded as an inherent baseline of the BSP-frozen profile, monitored only by IER readback (not IRQ).

## Open items

- ~~M1-B1~~: passed on board (six patterns normal, 1-line background top edge verified).
- ~~M1-B2~~: profile frozen and readback baseline saved (above); 20/20 resets passed; 30-min soak shortened to 21 min by user decision (zero incremental errors, LTDC=0); DSI PHY per-line baseline documented and excluded from the IRQ mask.
- M2 step 1 (BSP ARGB8888 GFXMMU reproduction): next round.
- Error-monitor fault-injection: not performed in M1 (no safe trigger path); arming proven by IER readback only; PHY baseline characterized empirically (per-line, benign).
- Before M2: revert toolchain `-T` to the formal linker and retire the DIAG profile.
