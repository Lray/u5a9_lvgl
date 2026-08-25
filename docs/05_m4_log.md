# M4 Record — progress + hardware facts (in progress)

Date: 2026-08-25 (continued session)  
Target: STM32U5A9J-DK / STM32U5A9NJH6Q / Cortex-M33 @160 MHz  
Baseline: M3 (`9b33e43`). Scope: M4 step A (NeoChrom integration) + step B (project DMA2D draw unit) — routing research and hardware characterization.

## Key hardware fact (plan §9.6 revision — evidence, not inference)

**U5A9 GFXMMU translates the graphics masters (LTDC, GPU2D) accesses; DMA2D does NOT pass through GFXMMU.**
- Evidence: on-board R2M self-test: DMA2D fill into plain SRAM works (poll + readback OK); identical R2M fill into the GFXMMU virtual window (0x24000000) polls success but neither virtual nor the LUT-mapped physical readback shows the written pixel → the DMA2D-AHB write to the 0x24000000 window performs a no-op / non-translated access.
- RM0456 Rev.7 Table 1/Table 2 list DMA2D as a bus master and GFXMMU as a slave for U59x/5Ax, but the GFXMMU translation master set is LTDC/GPU2D (graphics masters) — matching the on-board result (RM §21.4.1 footnote "system masters accessing GFXMMU").
- Consequence: **the project DMA2D draw unit cannot service the display root layer** (GFXMMU 3072-B stride buffers). Its legitimate role = contiguous off-screen layer buffers (rotation/transform stage, plan §2.2 layer path).

## M4 step A — NeoChrom/NemaGFX integration (done, characterized)

- HAL GPU2D driver (`stm32u5xx_hal_gpu2d.c/h`, SHA256 `B34CF05…`/`5E045B9…`) sourced from the authoritative Riverdi reference (HAL v1.3-era, self-contained) → our Drivers; `HAL_GPU2D_MODULE_ENABLED` enabled; `Src/gpu2d.c` + `Inc/gpu2d.h` created (MX_GPU2D_Init; SRAMCACHED clear+readback → `g_gpu2d_sramcached_readback=0` ✓; GPU2D clock only — NO DCACHE2 clock; NVIC prio 5 for GPU2D_IRQn/GPU2D_ER_IRQn; IRQ handlers in `stm32u5xx_it.c`).
- Nema link: precompiled `libnemagfx-float-abi-hard.a` (cortex_m33_revC; Cortex-M33/Thumb/FPv5-D16 EABI audited via readelf; SHA256 `124DBE8B…`). Link required `-Wl,--start-group … --end-group` (lvgl↔nema mutual references; single-pass archive scan failed with "Unknown destination type (ARM/Thumb)").
- `lv_conf.h`: `LV_USE_NEMA_GFX=1`, `LV_USE_NEMA_HAL=LV_NEMA_HAL_STM32`, `<stm32u5xx_hal.h>`, `LV_NEMA_GFX_MAX_RESX/Y=480` (pool sizing), `LV_USE_NEMA_VG=0` (M4 no VG paths).
- Pool placement (plan §6.4 object-specific, no vendor edit): linker `.nemagfx_pool` captures `*(.bss.nemagfx_pool_mem*)` BEFORE generic RUNCTX `.bss`; result `.nemagfx_pool @0x200D0000 size 0x3AC00 = 240,640 B = 480×480+10240 ✓ (exact plan §4.3 formula).
- lvgl target includes/defines extended for STM32 device headers (`STM32U5A9xx`,`USE_HAL_DRIVER`).
- **Result on-board with Nema active: WRONG rendering** (vertical red band + noise). Root cause: vendor Nema unit computes destination pitch as `lv_draw_buf_width_to_stride(480,RGB565)=960 B`; GFXMMU root buffers are 3072-B lines → per-row misplacement. **Nema unit cannot target the root layer** — reserved for contiguous off-screen layers. `LV_USE_NEMA_GFX` reverted to 0; GPU2D init + SRAMCACHED checks retained (needed for the layer-path stage).

## M4 step B — project DMA2D draw unit (implemented, characterized)

- `platform/graphics/lv_draw_dma2d_u5.{c,h}` — v9.3 private API: `lv_draw_create_unit`; `unit->*_cb`; `list->` semantics per `draw/lv_draw_private.h`; `PROJECT_DMA2D_UNIT_ID=5`, `preference_score=20`; register in `app_freertos` after `lv_init()`, before display (plan §5.1 order). Sync path: DMA2D R2M + OOR (`stride_px - width`) + `HAL_DMA2D_PollForTransfer(50ms)`; timeout/error → `HAL_DMA2D_Abort` + `HAL_DMA2D_Init`, task left WAITING (fallback re-dispatch), counters exposed (`g_u5_dma2d_stats`).
- Routing research (evidence-driven):
  - Initial evaluate rejected everything: `lv_draw_rect_dsc_init` sets `bg_grad.stops_count=2` and themed widgets carry radius/border — refine evaluate conditions = `radius==0 && border_width==0 && shadow_width==0 && bg_opa==LV_OPA_COVER && bg_grad.dir==LV_GRAD_DIR_NONE` (SCENE also stripped theme via `lv_obj_remove_style_all` on the bare rect/alpha).
  - On-board self-test isolated the hard limit: R2M+OOR works on SRAM; GFXMMU virtual target is a dead write → **add target range gate** (only buffers outside `0x24000000–0x24FFFFFF`).
- Final routing (this milestone, honest): root layer = SW unit only (`LV_USE_NEMA_GFX=0`, project DMA2D self-gated); BOTH can serve contiguous layer buffers in the M4 transform stage.

## On-board evidence snapshot

- build0 clean (gated): FLASH 455,348 B; NEMAGFX 0 B (pool captured only when Nema on); `g_gpu2d_sramcached_readback=0`; selftest `gain=0x07` (SRAM R2M ✓, GFXMMU-virt ✗); `g_u5_dma2d_stats`: reject_cf counts root fills as designed; scene renders correctly (user confirmed), line rate ~78 Hz, errors FB=0/DSI=1 baseline/LTDC=0/GFXMMU=0, heap/stack healthy.
- Nema-active build evidenced the 960-pitch corruption (photo) — retained in log as the vendor-unit limit.

## Open items (remaining M4)

- Layer-path stage: transform scene (rotated/scaled image) to exercise Nema (GPU2D/GFXMMU-capable master) + project DMA2D on contiguous layer buffers; CRC/tolerance gates per plan step 1.
- Error-injection path (debug timeout → abort → buffer-not-swapped) per plan step 2.
- Presets (sw_only/nema_only/dma2d_only/both_serial) + 4-profile comparison (step 4).
- 10-min (gate) long-run on final routing.
