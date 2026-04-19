/*
 * key_task.c
 * Key Input Task — monitors UART for user keypresses
 *
 * Waits for the user to press 't' on the QEMU console.
 * When detected, signals the NTP Client Task via a binary semaphore.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * CMSDK UART0 Registers (MPS2-AN385)
 * Base address: 0x40004000
 * --------------------------------------------------------------------------- */
#define UART0_BASE      0x40004000UL
#define UART0_DATA      (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_STATE     (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART0_CTRL      (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART0_BAUDDIV   (*(volatile uint32_t *)(UART0_BASE + 0x10))

/* State register bits */
#define UART_STATE_RXFULL   (1U << 1)

/* External: shared semaphore (created in main.c) */
extern SemaphoreHandle_t time_request_sem;

/* External: UART helper */
extern void uart_puts(const char *s);

/* ---------------------------------------------------------------------------
 * key_task — FreeRTOS task
 *
 * Priority: 1 (low)
 * Stack:    256 words
 *
 * Polls UART0 for incoming characters. On receiving 't', gives the
 * binary semaphore to wake the NTP Client Task.
 * --------------------------------------------------------------------------- */
void key_task(void *p)
{
    (void)p;
    char c;

    uart_puts("\r\n========================================\r\n");
    uart_puts("  Network-Synchronised Speaking Clock\r\n");
    uart_puts("  Press 't' to announce the time\r\n");
    uart_puts("========================================\r\n\r\n");

    while (1) {
        /* Check if UART has received a character */
        if (UART0_STATE & UART_STATE_RXFULL) {
            c = (char)(UART0_DATA & 0xFF);

            if (c == 't' || c == 'T') {
                uart_puts("Key: Time request received!\r\n");
                xSemaphoreGive(time_request_sem);
            }
        }

        /* Yield to other tasks, check again after 50ms */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
