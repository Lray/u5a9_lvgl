/*
******************************************************************************
**
**  File        : STM32U5A9NJHXQ_FLASH.ld
**
**  Abstract    : Formal SRAM partition linker (docs/00_plan.md §4.3/§6.4).
**                SRAM1/2/3/5 declared as independent MEMORY windows.
**
**                M2-A RGB565 phase: .fb0_phys is 384 KiB (RGB565 LUT
**                footprint 370,256 B + slack). ARGB8888 reproduction phase
**                used 720 KiB.
**
**  Target      : STMicroelectronics STM32U5A9xJ, 4096K FLASH / 2496K main SRAM
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Memories definition */
MEMORY
{
  FLASH    (rx)  : ORIGIN = 0x08000000, LENGTH = 4096K
  SRAM1    (xrw) : ORIGIN = 0x20000000, LENGTH = 768K
  DMASTG   (xrw) : ORIGIN = 0x200C0000, LENGTH = 32K
  PERTRC   (xrw) : ORIGIN = 0x200C8000, LENGTH = 16K
  NEMAGFX  (xrw) : ORIGIN = 0x200D0000, LENGTH = 256K
  LVGLH    (xrw) : ORIGIN = 0x20110000, LENGTH = 256K
  RTOSHEAP (xrw) : ORIGIN = 0x20150000, LENGTH = 128K
  RUNCTX   (xrw) : ORIGIN = 0x20170000, LENGTH = 176K
  GUARD    (xrw) : ORIGIN = 0x2019C000, LENGTH = 16K
  SRAM5    (xrw) : ORIGIN = 0x201A0000, LENGTH = 832K
  SRAM4    (xrw) : ORIGIN = 0x28000000, LENGTH = 16K
}
/* Highest address of the user mode stack: SRAM3 runtime window end */
_estack = ORIGIN(RUNCTX) + LENGTH(RUNCTX);

_Min_Heap_Size = 0x600;      /* required amount of heap  */
_Min_Stack_Size = 0x2000;    /* required amount of stack */

/* Sections */
SECTIONS
{
  /* The startup code into "FLASH" Rom type memory */
  .isr_vector :
  {
    KEEP(*(.isr_vector)) /* Startup code */
  } >FLASH

  /* The program code and other data into "FLASH" Rom type memory */
  .text :
  {
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    _etext = .;        /* define a global symbol at end of code */
  } >FLASH

  /* Constant data into "FLASH" Rom type memory */
  .rodata :
  {
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
  } >FLASH

  .ARM.extab (READONLY) :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH

  .ARM (READONLY) :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array (READONLY) :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array (READONLY) :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array (READONLY) :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections into SRAM3 runtime window */
  .data :
  {
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

  } >RUNCTX AT> FLASH

  /* Initialized TLS data section */
  .tdata : ALIGN(4)
  {
    *(.tdata .tdata.* .gnu.linkonce.td.*)
    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
    PROVIDE(__data_end = .);
    PROVIDE(__tdata_end = .);
  } >RUNCTX AT> FLASH

  PROVIDE( __tdata_start = ADDR(.tdata) );
  PROVIDE( __tdata_size = __tdata_end - __tdata_start );

  PROVIDE( __data_start = ADDR(.data) );
  PROVIDE( __data_size = __data_end - __data_start );

  PROVIDE( __tdata_source = LOADADDR(.tdata) );
  PROVIDE( __tdata_source_end = LOADADDR(.tdata) + SIZEOF(.tdata) );
  PROVIDE( __tdata_source_size = __tdata_source_end - __tdata_source );

  PROVIDE( __data_source = LOADADDR(.data) );
  PROVIDE( __data_source_end = __tdata_source_end );
  PROVIDE( __data_source_size = __data_source_end - __data_source );


  /* Uninitialized TLS data section */
  .tbss (NOLOAD) : ALIGN(4)
  {
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.tbss .tbss.*)
    . = ALIGN(4);
    PROVIDE( __tbss_end = . );
  } >RUNCTX

  PROVIDE( __tbss_start = ADDR(.tbss) );
  PROVIDE( __tbss_size = __tbss_end - __tbss_start );
  PROVIDE( __tbss_offset = ADDR(.tbss) - ADDR(.tdata) );

  PROVIDE( __tls_base = __tdata_start );
  PROVIDE( __tls_end = __tbss_end );
  PROVIDE( __tls_size = __tls_end - __tls_base );
  PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );
  PROVIDE( __tls_size_align = (__tls_size + __tls_align - 1) & ~(__tls_align - 1) );
  PROVIDE( __arm32_tls_tcb_offset = MAX(8, __tls_align) );
  PROVIDE( __arm64_tls_tcb_offset = MAX(16, __tls_align) );

  .bss (NOLOAD) : ALIGN(4)
  {
    *(.bss)
    *(.bss*)
    *(COMMON)

      . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
      PROVIDE( __bss_end = .);
  } >RUNCTX
  PROVIDE( __non_tls_bss_start = ADDR(.bss) );

  PROVIDE( __bss_start = __tbss_start );
  PROVIDE( __bss_size = __bss_end - __bss_start );

  /* User_heap_stack section: heap grows from _end toward _estack */
  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RUNCTX
  ASSERT(ABSOLUTE(.) <= 0x2019C000, "SRAM3 runtime exceeds [0x20170000,0x2019C000) window")

  /* GFXMMU physical buffer 0: SRAM1, 384 KiB RGB565 (footprint 370,256 B + slack) */
  .fb0_phys (NOLOAD) :
  {
    . = ALIGN(16);
    __fb0_phys_start__ = .;
    KEEP(*(.fb0_phys))
    . = ALIGN(4);
    __fb0_phys_end__ = .;
  } >SRAM1
  ASSERT(ADDR(.fb0_phys) == 0x20000000, "fb0_phys not at 0x20000000")
  ASSERT(SIZEOF(.fb0_phys) <= 393216, "fb0_phys exceeds 384 KiB")

/* GFXMMU physical buffer 1: SRAM5, 384 KiB RGB565 */
  .fb1_phys (NOLOAD) :
  {
    . = ALIGN(16);
    __fb1_phys_start__ = .;
    KEEP(*(.fb1_phys))
    . = ALIGN(4);
    __fb1_phys_end__ = .;
  } >SRAM5
  ASSERT(ADDR(.fb1_phys) == 0x201A0000, "fb1_phys not at 0x201A0000")
  ASSERT(SIZEOF(.fb1_phys) <= 393216, "fb1_phys exceeds 384 KiB")

  /* NemaGFX pool: SRAM3, 256 KiB window */
  .nemagfx_pool (NOLOAD) :
  {
    . = ALIGN(16);
    __nemagfx_pool_start__ = .;
    KEEP(*(.nemagfx_pool))
    . = ALIGN(4);
    __nemagfx_pool_end__ = .;
  } >NEMAGFX
  ASSERT(ADDR(.nemagfx_pool) == 0x200D0000, "nemagfx_pool not at 0x200D0000")
  ASSERT(SIZEOF(.nemagfx_pool) <= 262144, "nemagfx_pool exceeds 256 KiB")

  /* LVGL heap: SRAM3, 256 KiB window */
  .lvgl_heap (NOLOAD) :
  {
    . = ALIGN(16);
    __lvgl_heap_start__ = .;
    KEEP(*(.lvgl_heap))
    . = ALIGN(4);
    __lvgl_heap_end__ = .;
  } >LVGLH
  ASSERT(ADDR(.lvgl_heap) == 0x20110000, "lvgl_heap not at 0x20110000")
  ASSERT(SIZEOF(.lvgl_heap) <= 262144, "lvgl_heap exceeds 256 KiB")

  /* FreeRTOS heap: SRAM3, 128 KiB window */
  .freertos_heap (NOLOAD) :
  {
    . = ALIGN(16);
    __freertos_heap_start__ = .;
    KEEP(*(.freertos_heap))
    . = ALIGN(4);
    __freertos_heap_end__ = .;
  } >RTOSHEAP
  ASSERT(ADDR(.freertos_heap) == 0x20150000, "freertos_heap not at 0x20150000")
  ASSERT(SIZEOF(.freertos_heap) <= 131072, "freertos_heap exceeds 128 KiB")

  /* DMA staging: SRAM2, 32 KiB, non-cacheable */
  .dma_nocache (NOLOAD) :
  {
    . = ALIGN(32);
    __dma_nocache_start__ = .;
    KEEP(*(.dma_nocache))
    . = ALIGN(4);
    __dma_nocache_end__ = .;
  } >DMASTG
  ASSERT(ADDR(.dma_nocache) == 0x200C0000, "dma_nocache not at 0x200C0000")
  ASSERT(SIZEOF(.dma_nocache) <= 32768, "dma_nocache exceeds 32 KiB")

  /* Profiler trace ring: SRAM2, 16 KiB */
  .perf_trace (NOLOAD) :
  {
    . = ALIGN(32);
    __perf_trace_start__ = .;
    KEEP(*(.perf_trace))
    . = ALIGN(4);
    __perf_trace_end__ = .;
  } >PERTRC
  ASSERT(ADDR(.perf_trace) == 0x200C8000, "perf_trace not at 0x200C8000")
  ASSERT(SIZEOF(.perf_trace) <= 16384, "perf_trace exceeds 16 KiB")

  /* SRAM3-to-SRAM5 boundary guard: 16 KiB, MPU no-access region 7 */
  .sram3_guard (NOLOAD) :
  {
    . = ALIGN(16);
    __sram3_guard_start__ = .;
    KEEP(*(.sram3_guard))
    . = ALIGN(4);
    __sram3_guard_end__ = .;
  } >GUARD
  ASSERT(ADDR(.sram3_guard) == 0x2019C000, "sram3_guard not at 0x2019C000")
  ASSERT(SIZEOF(.sram3_guard) <= 16384, "sram3_guard exceeds 16 KiB")

  /* Remove information from the compiler libraries */
  /DISCARD/ :
  {
    libc.a:* ( * )
    libm.a:* ( * )
    libgcc.a:* ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}