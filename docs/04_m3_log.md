# M3 Record

Date: 2026-08-25  
Target: STM32U5A9J-DK / STM32U5A9NJH6Q / Cortex-M33 @160 MHz  
Baseline: M2 wrap-up (`7d5437c`), workspace clean except `docs/HANDOFF.md`.  
Scope this round: **M3 — FreeRTOS + LVGL v9.3.0 software direct-render double buffering on GFXMMU virtual buffers** (plan §7 M3).

## Restated acceptance (executed against)

Compile: fixed command 0 err/0 warn; vendor tree v9.3.0 unmodified; `LV_USE_NEMA_GFX=0`, `LV_USE_DRAW_DMA2D=0`, `LV_USE_ST_LTDC=0`; SW draw unit only + project stride-aware display port; static asserts 480×480 / stride 3072 / buf_size 1,474,560 / DIRECT; map: LVGL heap 256 KiB @LVGLH, FreeRTOS heap 128 KiB @RTOSHEAP, two physical FBs, no third full-frame buffer / BSP hidden PhysFrameBuffer.  
Board: composite scene with forced 100 %-dirty vs small-dirty phases (sync-copy est trend); ≥10 min continuous run (gate reduced from 30 min — see decisions); 20 resets with reverse-registration, `0<=submit-done<=1` and zero error growth.

## Decisions & deviations (plan §9.6)

1. **Continuous-run gate = 10 min (user decision 2026-08-25, binding for later milestones).** Replaces the plan's 30-min wording; actual coverage this round 11.08 min plus ~16 min of additional unconstrained running during interactive verification.
2. **Swap submitted only on the frame's last flush** (`lv_display_flush_is_last()`), matching the locked v9.3 stock ST LTDC driver semantics (`ThirdParty/LVGL/src/drivers/display/st_ltdc/lv_st_ltdc.c`). First integration submitted per inv-area flush; when a VBlank landed between area renders the panel exposed partially updated frames (user-visible flicker on translucent square). Fixed and re-verified.
3. **Scene motion via `lv_anim` (time-based)** instead of per-frame position stepping (frame-rate jitter made speed visibly uneven). Phase-A background color steps at 1 Hz (per-frame alternation was a full-screen strobe at ~15 Hz). Red/blue squares placed in disjoint vertical bands after an anim migration regression (Y left at default → overlap).
4. **perf_clock folded into `platform/os/freertos_runtime_stats.c`** (DWT 32→64-bit provider for the port macros). A separate perf_clock wrapper would have been a thin layer over the same accumulator.
5. **Profiler**: `LV_USE_PROFILER=1`, `LV_USE_PROFILER_BUILTIN=0`, `LV_PROFILER_INCLUDE="lv_profiler_backend.h"`; project backend (`platform/perf/perf_profiler.c`) accumulates LIFO begin/end pairs keyed by tag into a 24-slot table (calls/cycles/min/max + 64-entry ring with phase tag) placed in SRAM2 `.perf_trace`. Only `LV_PROFILER_REFR` enabled (minimal set).
6. **app_queues.* / platform/os/lvgl_task.* not created**: M3 has a single producer/consumer task (`defaultTask`); empty queue shells are forbidden (AGENTS.md). Deferred until a second task actually exists.
7. **FreeRTOS heap 128 KiB formalized now** (M0 收口/M3 前待办): `configAPPLICATION_ALLOCATED_HEAP=1`, `ucHeap` in `.freertos_heap`; `configGENERATE_RUN_TIME_STATS=1` + `configRUN_TIME_COUNTER_TYPE=uint64_t` + port macros → provider; LVGL idle% hooks (`traceTASK_SWITCHED_IN/OUT` → `lv_freertos_task_switch_*`) per official LVGL FreeRTOS doc.
8. One unexplained uptime regression between two probes (~18:14–18:16, before the soak t0) was most consistent with a manual NRST; not reproduced. All 21 subsequent boots clean.

## Implementation (file manifest)

- NEW `platform/display/lv_port_display.{c,h}` — display port: `lv_display_create(480,480)`; `lv_display_set_buffers_with_stride(B0v, B1v, 1474560, 3072, DIRECT)`; `flush_cb` = est-bookkeeping + `FB_Submit()` on last flush only; `flush_wait_cb` = spin on `g_fb_swap_pending==0` (stock-driver contract: no `lv_display_flush_ready`). Static asserts pin geometry/bases.
- NEW `platform/perf/perf_profiler.{c,h}`, `lv_profiler_backend.h` — LVGL custom profiler backend + aggregation (.perf_trace).
- NEW `platform/os/freertos_runtime_stats.{c,h}` — DWT→64-bit runtime counter.
- NEW `platform/perf/perf_gpio.{c,h}` — optional module, `PERF_GPIO_EN` off by default, compiles both ways, never wired (plan §5.2).
- MOD `Src/app_freertos.c` — ucHeap definition (.freertos_heap); 16 KiB LVGL task stack; scene (bg + moving rect + translucent alpha square + scrolling label, anim-driven); phase switch every 25 s with profiler phase tag; 1 Hz snapshot struct `g_m3_stats` (frames/submits/dones/refr/sync/wait/est/line-events/idle%/heaps/stack-HWM/errors/front).
- MOD `Src/main.c` — removed blocking M2 demo (patterns/100-swap loop/SoakLoop); keeps `FB_Init` + `Board_LCD_BringUp` + `Board_LCD_VerifyMapping`.
- MOD `config/lv_conf.h` — `LV_MEM_SIZE` 256 KiB; `LV_DRAW_BUF_ALIGN/STRIDE_ALIGN` 16; SYSMON+PERF_MONITOR on; PROFILER on (builtin off, backend include, REFR-only).
- MOD `Inc/FreeRTOSConfig.h` — USER CODE: app-allocated 128 KiB heap, uint64 runtime stats + port macros, trace-switch hooks.
- MOD `linker/STM32U5A9NJHXQ_FLASH.ld` — `.lvgl_heap` captures `*(.bss.work_mem_int*)` (GCC suffix) and `.freertos_heap` captures `ucHeap`; both output sections moved ahead of the generic RUNCTX `.bss` rule (first-match order); ORIGIN-pinned ASSERTs unchanged.
- MOD `CMakeLists.txt` — new sources/includes; `PERF_GPIO_EN` option (OFF).
- `ThirdParty/LVGL` untouched (git clean throughout).

## Compile acceptance

Fixed command (`cmd.exe /d` + Bundle PATH/CMAKE_GENERATOR/CMAKE_TOOLCHAIN_FILE, fresh `build`): **0 errors, 0 warnings**. Regions: FLASH 453,484 B; SRAM1 384 KB (fb0); SRAM5 384 KB (fb1); PERTRC 8,928 B (profiler slots); LVGLH 256 KB (work_mem_int.0); RTOSHEAP 128 KB (ucHeap); RUNCTX 18,584 B. Map: `.lvgl_heap`@0x20110000 0x40000, `.freertos_heap`@0x20150000 0x20000, `ucHeap`@0x20150000, no BSP `PhysFrameBuffer`. nm: SVC/PendSV/SysTick/TIM2_IRQHandler single strong definitions (port/timebase); `lv_version.h` = 9.3.0.

## Display port contract (as implemented)

Per refresh: `refr_sync_areas` waits (`flush_wait_cb` spins until reload done) → sync-copy of stale areas (front→back, CPU) → SW render of inv areas into `buf_act` → per-area flush; only the LAST flush submits `FB_Submit()` (SetAddress_NoReload + VBlank reload via framebuffer.c single-pending machine); `buf_act` toggles on last flush; next refresh's sync-wait drains the pending swap before copying/rendering. Reverse registration from `FB_Init` (front=B1v, LVGL buf1=B0v) reused unchanged.

## Board evidence

### Defect-fix iterations (interactive, user-observed)

| Iter | Symptom | Root cause | Fix |
|---|---|---|---|
| 1 | Translucent square flicker | per-area swap submission exposed partially rendered frames at interleaved VBlank | submit on `lv_display_flush_is_last` only |
| 2 | Full-screen strobe in phase A | scene alternated bg color every frame (~15 Hz) | 1 Hz color step |
| 3 | Uneven square speed; then red/blue overlap | per-frame ±px stepping tracked render jitter; Y lost in anim migration | `lv_anim` time-based X; fixed Y bands |

User confirmed all three resolved on-screen.

### Continuous run (10-min gate, t0=18:24:29, final read 18:35:30, uptime 664.7 s — no reset)

| Check | Result |
|---|---|
| frames / submits / dones | 43,815 flush_cb / 19,643 submits (snapshot) ; live final `submit==done==19,661`, `pending=0` |
| `0<=submit-done<=1` | held throughout (transient ≤1 while animating; settled equal) |
| reload == submitted frames | equal counts, zero backlog |
| line-event rate | 52,181 / 664.7 s = **78.50 Hz** (nominal 79.125, −0.79 %, within ±1 %) |
| errors | FB=0, DSI=1 (boot ACK baseline, no growth), LTDC=0, GFXMMU=0 |
| memory | LVGL heap peak 8,676 B / 256 KiB; RTOS heap min-free 105,808 / 131,072; task stack HWM 13.2 KiB / 16 KiB |
| est_copy_bytes | 795.5 MB cumulative ≈ 1.2 MB/s during partial-dirty phases; exactly 0 during full-repaint windows |

### Per-phase stats (profiler rings, 64 samples, cycles→ms @160 MHz)

| Scope | Phase 0 (100 % dirty) | Phase 1 (partial dirty) |
|---|---|---|
| est copy bytes/frame | **0** | > 0 continuously (~33 KB/frame equivalent) |
| `refr_sync_areas` | mean 1.454 ms, P95 7.624 ms (includes VBlank wait 0.627/4.578 ms) | mean 0.852 ms, P95 1.303 ms (wait ≈ 0.021 ms → residual ≈ copy+diff work) |
| whole refresh `lv_display_refr_timer` | mean 28.51 ms, P95 35.67 ms | mean 18.38 ms, P95 25.93 ms |

Full-repaint shows no sync-copy (est = 0, diff strips everything); partial-dirty shows sustained CPU sync-copy (est grows, sync residual present). Exact `lv_draw_buf_copy` byte counts stay non-exposed in v9.3 — est marked as estimate per plan §1.2; no zero-copy claim made.

### Reset test — 20/20 PASS (reset20c.ps1, read @~13.1 s each)

All rounds: deterministic startup (uptime 13.07–13.12 s, frames 385–386), `d≤1` (16 settled at 0/pending 0; 4 in-flight d=1), swap_errors 0, DSI=1 baseline constant, LTDC/GFXMMU 0, `Board_LCD_VerifyMapping` 3/3 every boot, front ∈ {0x24000000, 0x24400000} alternating (reverse registration intact; first frame stable per user observation across all boots).

## Open items

- M4 next: NeoChrom + DMA2D draw units, DCACHE2 kept off, serial routing (plan §7 M4).
- Runtime-stats per-task CSV reporting stays with M5 perf facility (provider already live).
- The refr-callback cadence under PERF_MONITOR runs faster than `LV_DEF_REFR_PERIOD` when idle (LVGL-internal); harmless (renders gated by invalidation), noted for M5 monitor-perturbation A/B.
