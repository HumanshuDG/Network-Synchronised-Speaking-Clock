/*
 * ntp_task.c
 * NTP Client Task — fetches network time and sends to Speech Task
 *
 * Blocks on a binary semaphore (signaled by Key Input Task).
 * When signaled:
 *   1. Waits for network (DHCP) to be ready
 *   2. Polls lwIP while waiting
 *   3. Sends NTP request, receives response
 *   4. Converts timestamp to IST
 *   5. Sends time_msg to Speech Task via queue
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include <stdint.h>
#include "time_msg.h"

/* External: shared IPC primitives (created in main.c) */
extern SemaphoreHandle_t time_request_sem;
extern QueueHandle_t     time_queue;

/* External: NTP client */
extern int ntp_get_time(time_msg *msg);

/* External: network helpers */
extern void network_init(void);
extern void lwip_periodic_handle(void);
extern int  network_is_up(void);

/* External: UART */
extern void uart_puts(const char *s);
extern int  uart_printf(const char *fmt, ...);

/* ---------------------------------------------------------------------------
 * ntp_task — FreeRTOS task
 *
 * Priority: 2 (higher than key_task and speech_task)
 * Stack:    1024 words
 *
 * This task:
 *   1. Initializes the lwIP network stack (must happen after scheduler starts)
 *   2. Drives the lwIP polling loop (NO_SYS mode)
 *   3. Handles NTP time requests from Key Task via semaphore
 * --------------------------------------------------------------------------- */
void ntp_task(void *p)
{
    (void)p;
    time_msg msg;
    int retries;

    /*
     * Phase 0: Initialize the network stack
     * Must be done in task context because lwIP's sys_now() uses
     * xTaskGetTickCount() which requires the scheduler to be running.
     */
    uart_puts("NTP Task: Initializing network stack...\r\n");
    network_init();
    uart_puts("NTP Task: Network stack initialized.\r\n");

    uart_puts("NTP Task: Waiting for DHCP...\r\n");

    /*
     * Phase 1: Wait for DHCP to assign an IP address
     * Keep polling lwIP while waiting
     */
    while (!network_is_up()) {
        lwip_periodic_handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    uart_puts("NTP Task: Network is ready!\r\n");
    uart_puts("NTP Task: Waiting for time requests...\r\n");

    while (1) {
        /*
         * Use a timeout on the semaphore so we can continue
         * polling lwIP even when no request is pending.
         * This keeps DHCP leases alive, processes ARP, etc.
         */
        if (xSemaphoreTake(time_request_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
            uart_puts("NTP Task: Processing time request...\r\n");

            /* Attempt NTP query with retries */
            retries = 3;
            while (retries > 0) {
                if (ntp_get_time(&msg) == 0) {
                    /* Success — send time to speech task */
                    uart_printf("NTP Task: Time = %02d:%02d:%02d IST\r\n",
                                msg.hour, msg.minute, msg.second);

                    if (xQueueSend(time_queue, &msg, pdMS_TO_TICKS(1000)) != pdTRUE) {
                        uart_puts("NTP Task: Failed to send to queue!\r\n");
                    }
                    break;
                }

                retries--;
                if (retries > 0) {
                    uart_printf("NTP Task: Retry (%d remaining)...\r\n", retries);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }

            if (retries == 0) {
                uart_puts("NTP Task: All retries failed!\r\n");
            }
        }

        /* Always poll lwIP to keep the network stack alive */
        lwip_periodic_handle();
    }
}
