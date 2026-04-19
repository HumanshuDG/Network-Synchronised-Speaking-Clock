/*
 * main.c
 * Entry point for the Network-Synchronised Speaking Clock
 *
 * Initializes hardware (UART), networking (lwIP + LAN9118),
 * creates FreeRTOS IPC primitives and tasks, then starts the scheduler.
 *
 * Architecture:
 *   Key Input Task --[semaphore]--> NTP Client Task --[queue]--> Speech Task
 *                                        |
 *                                   lwIP + LAN9118
 *                                        |
 *                                   NTP Server (Internet)
 */

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "time_msg.h"

/* ---------------------------------------------------------------------------
 * CMSDK UART0 Register Definitions (MPS2-AN385)
 * --------------------------------------------------------------------------- */
#define UART0_BASE      0x40004000UL
#define UART0_DATA      (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_STATE     (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART0_CTRL      (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART0_BAUDDIV   (*(volatile uint32_t *)(UART0_BASE + 0x10))

/* State register bits */
#define UART_STATE_TXFULL   (1U << 0)
#define UART_STATE_RXFULL   (1U << 1)

/* Control register bits */
#define UART_CTRL_TXEN      (1U << 0)
#define UART_CTRL_RXEN      (1U << 1)

/* ---------------------------------------------------------------------------
 * Global IPC Primitives
 *
 * time_request_sem: Binary semaphore signaled by Key Task to wake NTP Task
 * time_queue:       Queue carrying time_msg from NTP Task to Speech Task
 * --------------------------------------------------------------------------- */
SemaphoreHandle_t time_request_sem = NULL;
QueueHandle_t     time_queue       = NULL;

/* ---------------------------------------------------------------------------
 * External task functions
 * --------------------------------------------------------------------------- */
extern void key_task(void *p);
extern void ntp_task(void *p);
extern void speech_task(void *p);

/* External network init */
extern void network_init(void);

/* ---------------------------------------------------------------------------
 * UART Output Functions
 * --------------------------------------------------------------------------- */

/*
 * uart_putc — send a single character to UART0
 */
void uart_putc(char c)
{
    /* Wait for TX buffer to be ready */
    while (UART0_STATE & UART_STATE_TXFULL)
        ;
    UART0_DATA = (uint32_t)c;
}

/*
 * uart_puts — send a null-terminated string to UART0
 */
void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

/*
 * uart_printf — formatted print to UART0
 * Uses a static buffer (not reentrant, but fine for our single-writer usage)
 */
int uart_printf(const char *fmt, ...)
{
    static char buf[256];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    uart_puts(buf);
    return len;
}

/* ---------------------------------------------------------------------------
 * System Initialization
 * --------------------------------------------------------------------------- */
static void system_init(void)
{
    /*
     * Initialize UART0 on MPS2-AN385
     *
     * In QEMU, the CMSDK UART works without baud rate configuration,
     * but we set it up for completeness.
     *
     * System clock = 25 MHz, desired baud = 115200
     * BAUDDIV = 25000000 / 115200 ≈ 217
     */
    UART0_CTRL = 0;                         /* Disable UART during config */
    UART0_BAUDDIV = 25000000 / 115200;      /* Set baud rate divisor      */
    UART0_CTRL = UART_CTRL_TXEN |           /* Enable TX                  */
                 UART_CTRL_RXEN;            /* Enable RX                  */

    uart_puts("\r\n");
    uart_puts("================================================\r\n");
    uart_puts("  Speaking Clock - STM32 / FreeRTOS / QEMU\r\n");
    uart_puts("  Network Time Protocol (NTP) Client\r\n");
    uart_puts("================================================\r\n");
    uart_puts("\r\n");
}

/* ---------------------------------------------------------------------------
 * FreeRTOS Hook Functions
 * --------------------------------------------------------------------------- */

/*
 * vApplicationStackOverflowHook — called if a stack overflow is detected
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    uart_puts("FATAL: Stack overflow in task: ");
    uart_puts(pcTaskName);
    uart_puts("\r\n");
    while (1)
        ;
}

/*
 * vApplicationMallocFailedHook — called if pvPortMalloc() fails
 */
void vApplicationMallocFailedHook(void)
{
    uart_puts("FATAL: Malloc failed!\r\n");
    while (1)
        ;
}

/*
 * vAssertCalled — called by configASSERT() on failure
 */
void vAssertCalled(const char *pcFile, uint32_t ulLine)
{
    uart_printf("ASSERT FAILED: %s:%u\r\n", pcFile, (unsigned)ulLine);
    while (1)
        ;
}

/* ---------------------------------------------------------------------------
 * lwIP sys_now — returns current time in milliseconds
 * Required by lwIP timers in NO_SYS mode
 * Uses the FreeRTOS tick count (1 tick = 1ms at configTICK_RATE_HZ=1000)
 * --------------------------------------------------------------------------- */
#include "lwip/arch.h"

u32_t sys_now(void)
{
    return (u32_t)xTaskGetTickCount();
}

/* ---------------------------------------------------------------------------
 * Newlib stubs — required by nano.specs libc
 * --------------------------------------------------------------------------- */
extern unsigned int _heap_bottom;
extern unsigned int _heap_top;

void *_sbrk(int incr)
{
    static unsigned char *heap_ptr = NULL;
    unsigned char *prev;

    if (heap_ptr == NULL) {
        heap_ptr = (unsigned char *)&_heap_bottom;
    }

    prev = heap_ptr;
    if (heap_ptr + incr > (unsigned char *)&_heap_top) {
        return (void *)-1;
    }
    heap_ptr += incr;
    return (void *)prev;
}

/* ---------------------------------------------------------------------------
 * Fault Handlers (override weak defaults from startup)
 * --------------------------------------------------------------------------- */
void HardFault_Handler(void)
{
    uart_puts("\r\n!!! HARDFAULT !!!\r\n");
    while (1)
        ;
}

void MemManage_Handler(void)
{
    uart_puts("\r\n!!! MEMMANAGE FAULT !!!\r\n");
    while (1)
        ;
}

void BusFault_Handler(void)
{
    uart_puts("\r\n!!! BUS FAULT !!!\r\n");
    while (1)
        ;
}

void UsageFault_Handler(void)
{
    uart_puts("\r\n!!! USAGE FAULT !!!\r\n");
    while (1)
        ;
}

/* ---------------------------------------------------------------------------
 * main — application entry point
 *
 * Network initialization is done inside the NTP task (after scheduler starts)
 * because lwIP's sys_now() needs the FreeRTOS tick counter to be running.
 * --------------------------------------------------------------------------- */
int main(void)
{
    /* 1. Initialize UART for console I/O */
    system_init();

    /* 2. Create IPC primitives */
    uart_puts("Creating IPC primitives...\r\n");

    time_request_sem = xSemaphoreCreateBinary();
    if (time_request_sem == NULL) {
        uart_puts("FATAL: Failed to create semaphore!\r\n");
        while (1)
            ;
    }

    time_queue = xQueueCreate(1, sizeof(time_msg));
    if (time_queue == NULL) {
        uart_puts("FATAL: Failed to create queue!\r\n");
        while (1)
            ;
    }

    /* 3. Create FreeRTOS tasks */
    uart_puts("Creating tasks...\r\n");

    /*
     * Key Input Task:
     *   - Monitors UART for 't' keypress
     *   - Signals NTP task via semaphore
     *   - Priority 1 (low), Stack 256 words
     */
    xTaskCreate(key_task, "KEY", 256, NULL, 1, NULL);

    /*
     * NTP Client Task:
     *   - Initializes lwIP + LAN9118 network stack
     *   - Drives lwIP polling loop
     *   - Fetches time from NTP server on semaphore signal
     *   - Sends time_msg to speech task via queue
     *   - Priority 2 (medium), Stack 1024 words (needs extra for lwIP init)
     */
    xTaskCreate(ntp_task, "NTP", 1024, NULL, 2, NULL);

    /*
     * Speech Task:
     *   - Receives time_msg from queue
     *   - Emits TOKEN/END stream over UART
     *   - Priority 1 (low), Stack 512 words
     */
    xTaskCreate(speech_task, "SPEECH", 512, NULL, 1, NULL);

    /* 4. Start the FreeRTOS scheduler — this never returns */
    uart_puts("Starting FreeRTOS scheduler...\r\n\r\n");
    vTaskStartScheduler();

    /* Should never reach here */
    uart_puts("FATAL: Scheduler returned!\r\n");
    while (1)
        ;

    return 0;
}

