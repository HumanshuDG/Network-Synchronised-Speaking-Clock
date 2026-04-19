/*
 * stm32.ld
 * Linker script for MPS2-AN385 (Cortex-M3) on QEMU
 * Memory layout compatible with STM32-class microcontrollers
 *
 * Flash: 0x00000000, 4 MB
 * RAM:   0x20000000, 4 MB
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}

/* Stack grows downward from end of RAM */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Minimum sizes for heap and stack */
_Min_Heap_Size  = 0x1000;      /* 4KB for newlib (rand, printf, etc.) */
_Min_Stack_Size = 0x400;       /* 1 KB for main() stack, then reused as ISR stack */

SECTIONS
{
    /* ---------------------------------------------------------------
     * Vector table — must be at address 0x00000000
     * --------------------------------------------------------------- */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* ---------------------------------------------------------------
     * Executable code
     * --------------------------------------------------------------- */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(4);
    } > FLASH

    /* ---------------------------------------------------------------
     * Read-only data (constants, strings)
     * --------------------------------------------------------------- */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        *(.constdata*)
        . = ALIGN(4);
    } > FLASH

    /* ---------------------------------------------------------------
     * ARM exception unwinding tables
     * --------------------------------------------------------------- */
    .ARM.extab :
    {
        *(.ARM.extab*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    _etext = .;
    _sidata = LOADADDR(.data);

    /* ---------------------------------------------------------------
     * Initialized data — stored in Flash, copied to RAM at startup
     * --------------------------------------------------------------- */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(vtable)
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    /* ---------------------------------------------------------------
     * Uninitialized data — zeroed at startup
     * --------------------------------------------------------------- */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* ---------------------------------------------------------------
     * Heap and stack space
     * --------------------------------------------------------------- */
    .heap :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        _heap_bottom = .;
        . = . + _Min_Heap_Size;
        _heap_top = .;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* Stack top at end of RAM */
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - _Min_Stack_Size;
    PROVIDE(__stack = __StackTop);

    /* Overflow check */
    ASSERT(__StackLimit >= _heap_top, "region RAM overflowed with stack")
}
