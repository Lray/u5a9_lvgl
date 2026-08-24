# M2 Record

Date: 2026-08-24  
Target: STM32U5A9J-DK / STM32U5A9NJH6Q / Cortex-M33  
Baseline: M1 passed (`18b03f1`), LVGL relocated (`a0781ad`), CubeMX GFXMMU config (`8c8cfe3`).  
Scope this round: **M2 step 1 — BSP ARGB8888 LUT + single GFXMMU buffer reproduction** (plan §4.4/§7 M2). Formal SRAM-partition linker lands; DIAG linker retired.

## Decisions recorded (plan §9.6)

- **RGB565 LUT imported ahead of ARGB8888 reproduction** (user decision): `Inc/gfxmmu_lut.h` (CubeMX LUT import, `GFXMMU_FB_SIZE 370,256`) is kept untouched as the M2-A phase LUT; this round still executes the ARGB8888 reproduction with the BSP table `gfxmmu_lut_config_argb8888`.
- **BSP `stm32u5x9j_discovery_lcd.c` is not compiled** (no hidden `PhysFrameBuffer` object exists in this build); the reproduction is driven by the project-owned `board_lcd.c` plus CubeMX-generated `Src/gfxmmu.c`, with the BSP ARGB LUT injected from the locked BSP header. The plan's "hidden PhysFrameBuffer must be reclaimed" gate is satisfied vacuously and the deviation recorded here.
- RGB565 LUT validated host-side: 480 rows, EN=1 all, FVB<=LVB<192, LO continuity 0 mismatches, physical continuity 0 mismatches, footprint 370,256 B (plan §4.2 conservative-cover criterion).

## Formal linker (`linker/STM32U5A9NJHXQ_FLASH.ld`)

SRAM1/2/3/5 modeled as independent windows; every fixed-address section uses its own MEMORY region (ORIGIN-pinned, no location-counter back-jumps):

| Region | Address | Size | Content |
|---|---|---:|---:|
| SRAM1 | 0x20000000 | 768K | `.fb0_phys` 720 KiB (ARGB reproduction; RGB565 tightens to 384 KiB later) |
| DMASTG | 0x200C0000 | 32K | `.dma_nocache` (empty placeholder) |
| PERTRC | 0x200C8000 | 16K | `.perf_trace` (empty placeholder) |
| NEMAGFX | 0x200D0000 | 256K | `.nemagfx_pool` (empty placeholder) |
| LVGLH | 0x20110000 | 256K | `.lvgl_heap` (empty placeholder) |
| RTOSHEAP | 0x20150000 | 128K | `.freertos_heap` (empty placeholder) |
| RUNCTX | 0x20170000 | 176K | `.data/.tdata/.tbss/.bss/_user_heap_stack`, MSP top 0x2019C000 |
| GUARD | 0x2019C000 | 16K | `.sram3_guard` (empty, MPU region 7 no-access later) |
| SRAM5 | 0x201A0000 | 832K | `.fb1_phys` 384 KiB (empty until M2-A) |
| SRAM4 | 0x28000000 | 16K | reserved, unused |

Fixed-command build: **0 errors, 0 warnings**. Linker `ASSERT`s pin every region address/size upper bound; runtime fit checked against RUNCTX end.

## Memory audit (map)

- `.fb0_phys` 0x20000000, size 0xB4000 (737,280 B = 720 KiB), align 16, NOLOAD.
- RUNCTX usage 148,720 B (82.5 %); FLASH 263,128 B.
- `gfxmmu_lut_config_argb8888` linked from BSP header into FLASH rodata.

## GFXMMU configuration (CubeMX-generated `Src/gfxmmu.c` + one edit)

- 192 blocks/line; `DefaultValue` edited 0 → **0xFFFFFFFF** (BSP value); Buf0=0x20000000, Buf1=0x201A0000; cache/prefetch off; interrupts on (buffer0 overflow + AHB master error).
- `Board_LCD_BringUp()` injects the **BSP ARGB8888 LUT** (`HAL_GFXMMU_ConfigLut` 0..479 + `HAL_GFXMMU_DisableLutLines` 480..544), then runs the M1 panel sequence unchanged.
- LTDC layer: `FBStartAdress = 0x24000000` (GFXMMU virtual buffer0), `ImageWidth = 768` (3072-B stride), `PixelFormat ARGB8888`, window `[1,481)`, `ImageHeight 480` (M1-B1 geometry kept). DSI profile = M1-B2 frozen (RGB888).

## Board result — ARGB8888 GFXMMU reproduction PASSED (probe-driven)

| Check | Result |
|---|---|
| Mapping verify (`g_board_lcd_map_check`) | **3/3 pass**: (240,240) physical readback = written 0xFF00FF00; outside-circle (row 0, x=200) `virt_to_phys` = 0 and virtual readback = **0xFFFFFFFF** (default) with write discarded |
| Map point physical address | 0x20059AE0 (inside `.fb0_phys`) |
| LTDC readback | CFBAR=0x24000000, CFBLR pitch 0x0C00=3072, line length 0x783=480×4+3, CFBLNR=480 |
| Frame rate | line-events 4,027 in ~50.9 s; period 2,038,045 cyc (78.5 Hz), within ±1 % |
| Errors | DSI=1 (one-time boot ACK baseline, no growth), LTDC=0 |

Visual: six diagnostic patterns, checkerboard, color bars and gray ramp all drawn through the GFXMMU virtual address (user-visible confirmation pending).

## M2 step 2 — double-buffer VBlank swap PASSED (probe-driven)

`platform/display/framebuffer.c` implements plan §2.2 ownership rules: two 720-KiB physical buffers (`.fb0_phys`@SRAM1 0x20000000, `.fb1_phys`@SRAM5 0x201A0000), virtual 0x24000000/0x24400000, deliberate reversed initial registration (LTDC front=B1 at `FB_Init`, first draw target=B0), single pending reload, only `HAL_LTDC_ReloadEventCallback` may change ownership.

| Check | Result |
|---|---|
| swap_submit_seq vs reload_done_seq | 113 vs 113, equal after settle (0 ≤ submit−done ≤ 1 honored, stop-then-settle = 0) |
| swap_pending | 0 after settle |
| swap_errors | 0 (no double-submit, no HAL failure) |
| Ownership | front/back alternate exactly 0x24000000↔0x24400000; each submit → exactly one reload event |
| DWT timestamps | submit/done pairs recorded, non-zero, increasing |
| CFBAR (snapshot) | 0x24400000 initial front; alternates per swap (firmware snapshot readback; direct APB probe read of CFBAR not supported by HOTPLUG — noted) |
| Display flow | 100 red/blue full-screen swaps at 2/s (500 ms cadence), then double-buffered soak (color bars / gray ramp with one VBlank swap each) |
| Frame rate | line-events 10,308 in ~130 s; period 2,039,549 cyc (78.44 Hz), ±1 % |
| Errors | DSI=1 (boot-time ACK baseline), LTDC=0 |

Diagnostic addition: `Board_LCD_VerifyMapping` (mapping self-test, 3/3 pass) runs after patterns; `g_fb_submit_ts/g_fb_done_ts` rings readable via probe.

### Debugging note

First double-buffer build hard-faulted at boot: `fill_rect` had been switched to the dynamic back-buffer pointer, but the panel-bring-up clear-screen call runs before `FB_Init` — pointer still 0 → write to 0x0 → HardFault (symptoms: line-events and register snapshot all zero). Fixed by `fill_rect_at(base,...)` + keep clear-screen on the constant buffer0 base + `FB_Init()` moved before `Board_LCD_BringUp()`.

## Open items

- M2 step 2 (double-buffer VBlank swap): see above — done; visual confirmation of red/blue + soak by user pending.
- M2-A: switch LTDC to RGB565 (`ImageWidth` 1536, stride 3072), use the imported RGB565 LUT, footprint validator final numbers (366,992/370,256/370,304 criteria vs 370,256 actual), then tighten `.fb0_phys` to 384 KiB.
- M2-B (optional, non-blocking): DSI RGB565 cold-boot experiment per plan.