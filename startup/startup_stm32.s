/*
 * startup_stm32.s
 * Startup code for Cortex-M3 (MPS2-AN385 / STM32-class)
 * Vector table, Reset_Handler, runtime initialization (.data copy, .bss zero)
 *
 * Note: SVC_Handler and PendSV_Handler are provided by the FreeRTOS port
 *       (port.c), so they are NOT defined here as weak defaults.
 */

    .syntax unified
    .cpu    cortex-m3
    .thumb

/* ---------------------------------------------------------------------------
 * External symbols from linker script
 * --------------------------------------------------------------------------- */
    .extern _estack
    .extern _sidata
    .extern _sdata
    .extern _edata
    .extern _sbss
    .extern _ebss

/* Export Reset_Handler and Default_Handler */
    .global Reset_Handler
    .global Default_Handler

/* ===========================================================================
 * Vector Table
 * Placed in .isr_vector section, mapped to address 0x00000000
 * =========================================================================== */
    .section .isr_vector, "a", %progbits
    .type   g_pfnVectors, %object
g_pfnVectors:
    .word   _estack                 /* 0x00: Initial Stack Pointer             */
    .word   Reset_Handler           /* 0x04: Reset Handler                     */
    .word   NMI_Handler             /* 0x08: NMI Handler                       */
    .word   HardFault_Handler       /* 0x0C: Hard Fault Handler                */
    .word   MemManage_Handler       /* 0x10: MPU Fault Handler                 */
    .word   BusFault_Handler        /* 0x14: Bus Fault Handler                 */
    .word   UsageFault_Handler      /* 0x18: Usage Fault Handler               */
    .word   0                       /* 0x1C: Reserved                          */
    .word   0                       /* 0x20: Reserved                          */
    .word   0                       /* 0x24: Reserved                          */
    .word   0                       /* 0x28: Reserved                          */
    .word   SVC_Handler             /* 0x2C: SVCall (FreeRTOS)                 */
    .word   DebugMon_Handler        /* 0x30: Debug Monitor                     */
    .word   0                       /* 0x34: Reserved                          */
    .word   PendSV_Handler          /* 0x38: PendSV (FreeRTOS)                 */
    .word   SysTick_Handler         /* 0x3C: SysTick (FreeRTOS)                */

    /* External Interrupts (IRQ 0 - 15 for MPS2-AN385) */
    .word   Default_Handler         /* IRQ  0: UART0 RX                        */
    .word   Default_Handler         /* IRQ  1: UART0 TX                        */
    .word   Default_Handler         /* IRQ  2: UART1 RX                        */
    .word   Default_Handler         /* IRQ  3: UART1 TX                        */
    .word   Default_Handler         /* IRQ  4: UART2 RX                        */
    .word   Default_Handler         /* IRQ  5: UART2 TX                        */
    .word   Default_Handler         /* IRQ  6: GPIO Port 0 combined            */
    .word   Default_Handler         /* IRQ  7: GPIO Port 1 combined            */
    .word   Default_Handler         /* IRQ  8: Timer 0                         */
    .word   Default_Handler         /* IRQ  9: Timer 1                         */
    .word   Default_Handler         /* IRQ 10: Dual Timer                      */
    .word   Default_Handler         /* IRQ 11: SPI                             */
    .word   Default_Handler         /* IRQ 12: UART Overflow                   */
    .word   EthernetISR             /* IRQ 13: Ethernet (LAN9118)              */
    .word   Default_Handler         /* IRQ 14: Audio I2S                       */
    .word   Default_Handler         /* IRQ 15: Touch Screen                    */

    .size   g_pfnVectors, .-g_pfnVectors

/* ===========================================================================
 * Reset_Handler
 * Called on startup. Copies .data from Flash to SRAM, zeros .bss, calls main()
 * =========================================================================== */
    .section .text.Reset_Handler, "ax", %progbits
    .type   Reset_Handler, %function
    .thumb_func
Reset_Handler:
    /* Set stack pointer (redundant but safe) */
    ldr     sp, =_estack

    /*
     * 1. Copy .data from Flash (LMA) to SRAM (VMA)
     */
    ldr     r0, =_sidata            /* Source: end of .text in Flash */
    ldr     r1, =_sdata             /* Destination start in SRAM    */
    ldr     r2, =_edata             /* Destination end in SRAM      */
copy_data_loop:
    cmp     r1, r2
    bge     zero_bss_init
    ldr     r3, [r0], #4
    str     r3, [r1], #4
    b       copy_data_loop

    /*
     * 2. Zero-initialize .bss in SRAM
     */
zero_bss_init:
    ldr     r0, =_sbss
    ldr     r1, =_ebss
    movs    r2, #0
zero_bss_loop:
    cmp     r0, r1
    bge     call_main
    str     r2, [r0], #4
    b       zero_bss_loop

    /*
     * 3. Call main()
     */
call_main:
    bl      main
    /* If main() returns, loop forever */
infinite_loop:
    b       infinite_loop

    .size   Reset_Handler, .-Reset_Handler

/* ===========================================================================
 * Default_Handler - infinite loop for unhandled interrupts
 * =========================================================================== */
    .section .text.Default_Handler, "ax", %progbits
    .thumb_func
Default_Handler:
    b       Default_Handler
    .size   Default_Handler, .-Default_Handler

/* ===========================================================================
 * Weak aliases for exception handlers
 * SVC_Handler, PendSV_Handler, SysTick_Handler are NOT weak here —
 *   they are provided by FreeRTOS port.c
 * =========================================================================== */
    .weak   NMI_Handler
    .thumb_set NMI_Handler, Default_Handler

    .weak   HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler

    .weak   MemManage_Handler
    .thumb_set MemManage_Handler, Default_Handler

    .weak   BusFault_Handler
    .thumb_set BusFault_Handler, Default_Handler

    .weak   UsageFault_Handler
    .thumb_set UsageFault_Handler, Default_Handler

    .weak   DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler

    .weak   EthernetISR
    .thumb_set EthernetISR, Default_Handler

    .end
