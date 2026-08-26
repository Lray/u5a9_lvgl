# M5 Benchmark Results (2026-08-26)

Board: STM32U5A9J-DK (ST-LINK 003C00333532510E31333430, 1.79 V)
Build: M5 stage 6, fixed-command acceptance build (zero warnings, 497 targets)
Toolchain: GNU ARM 14.3.1+st.2, -O0 -g3 (Debug), cortex-m33 fpv5-sp-d16 hard-float
Display: LVGL 9.3.0 direct 480x480 stride 3072, RGB565, GFXMMU, scanout ~78.5 Hz
Route: root SW-only; offscreen layer -> Nema(transform)/DMA2D(clean fill); serial
Resources: program-generated (plan §7 M5 fallback); PSRAM non-cacheable safe profile
Method: 10 s warm-up + 60 s sample x3 per scene, CSV @1 Hz via ST-LINK VCP 921600

CSV columns: seq,up_ms,lines,lvgl_frames,sw_sub,fb_err,dsi_err,ltdc_err,gfxmmu_err,
dma_disp,dma_err,n_allocs,n_frees,outst_hwm,fails,idle_pct,lv_used_max

## Summary

| scene       | FPS  | swaps/s | scan Hz | idle% | lvmax B |
|-------------|------|---------|---------|-------|---------|
| full_repaint| 29.5 | 29.4    | 78.1    | 31.7  | 7408    |
| transform   | 32.8 | 29.5    | 78.4    | 32.1  | 11348   |
| alpha       | 2.96 | 2.9     | 77.6    | 0.0   | 8256    |
| text        | 63.2 | 29.4    | 78.2    | 31.0  | 8264    |
| mixed       | 68.6 | 28.3    | 78.6    | 16.0  | 12184   |

All scenes: errors fb/dsi/ltdc/gfxmmu/dma2d = 0, Nema allocs=2/hwm=2/fails=0.
Scanout 78.1-78.6 Hz across all runs (nominal 79.1, within +-1%).
3 iterations per scene; per-iteration CSVs in this directory.

## Long soak (mixed, 10 min user gate)

mixed_soak1.csv: dt=609 s, 620 lines @1 Hz (no drops), fps 65.4, scan 78.59 Hz,
errors all zero, Nema fails=0, no reset.

## Monitor on/off perturbation (full_repaint, 62 s each)

- on  (default build):   fps 29.53, idle 31.7%, lvmax 7408
- off (-DLV_USE_PROFILER=0 -DLV_USE_SYSMON=0 -DLV_USE_PERF_MONITOR=0):
                         fps 29.45, idle 32.9%, lvmax 6288
=> instrumentation cost ~1% CPU, <1% FPS; 1,120 B LVGL heap saved when off.

## Notes

- transform_iter3 truncated at 33 s due to one host-side VCP interruption (kept, marked).
- alpha is bandwidth-saturated (idle 0%) as designed for the blend pressure scenario.
- Untested (recorded per plan): XRGB8888, PSRAM cacheable, controlled concurrency,
  formal NOR resource manifest (program-generated resources per plan fallback).