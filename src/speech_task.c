/*
 * speech_task.c
 * Speech Task — generates spoken time output as token stream
 *
 * Receives time_msg from NTP Client Task via FreeRTOS queue.
 * Emits speech tokens over UART in the format:
 *   TOKEN <word>
 *   ...
 *   END
 *
 * A host-side Python bridge (tts_bridge.py) reads these tokens
 * and converts them to audible speech using eSpeak or similar.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdint.h>
#include <string.h>

#include "time_msg.h"
#include "speech_tokens.h"

/* External: shared queue (created in main.c) */
extern QueueHandle_t time_queue;

/* External: UART functions */
extern void uart_puts(const char *s);
extern int  uart_printf(const char *fmt, ...);

/* ---------------------------------------------------------------------------
 * uart_send_token — send a single speech token over UART
 *
 * Format: "TOKEN <word>\r\n"
 * --------------------------------------------------------------------------- */
void uart_send_token(const char *token)
{
    uart_puts("TOKEN ");
    uart_puts(token);
    uart_puts("\r\n");
}

/* ---------------------------------------------------------------------------
 * speak_time — emit the full token sequence for a given time
 *
 * Output format:
 *   TOKEN THE
 *   TOKEN TIME
 *   TOKEN IS
 *   TOKEN <hour_word>
 *   TOKEN HOURS / TOKEN HOUR
 *   TOKEN <minute_word>
 *   TOKEN MINUTES / TOKEN MINUTE
 *   TOKEN AND
 *   TOKEN <second_word>
 *   TOKEN SECONDS / TOKEN SECOND
 *   END
 * --------------------------------------------------------------------------- */
void speak_time(uint8_t hour, uint8_t minute, uint8_t second)
{
    uart_puts("\r\n--- Speech Output ---\r\n");

    /* "The time is" */
    uart_send_token(TOKEN_THE);
    uart_send_token(TOKEN_TIME);
    uart_send_token(TOKEN_IS);

    /* Hour */
    if (hour < 60) {
        uart_send_token(number_words[hour]);
    }
    uart_send_token((hour == 1) ? TOKEN_HOUR : TOKEN_HOURS);

    /* Minute */
    if (minute < 60) {
        uart_send_token(number_words[minute]);
    }
    uart_send_token((minute == 1) ? TOKEN_MINUTE : TOKEN_MINUTES);

    /* "and" */
    uart_send_token(TOKEN_AND);

    /* Second */
    if (second < 60) {
        uart_send_token(number_words[second]);
    }
    uart_send_token((second == 1) ? TOKEN_SECOND : TOKEN_SECONDS);

    /* End marker */
    uart_puts("END\r\n");

    uart_puts("--- End Speech ---\r\n\r\n");
}

/* ---------------------------------------------------------------------------
 * speech_task — FreeRTOS task
 *
 * Priority: 1 (same as key_task)
 * Stack:    512 words
 *
 * Blocks on the time_queue. When a time_msg arrives, calls speak_time()
 * to emit the token stream.
 * --------------------------------------------------------------------------- */
void speech_task(void *p)
{
    (void)p;
    time_msg msg;

    uart_puts("Speech Task: Started, waiting for time messages...\r\n");

    while (1) {
        /* Block until a time message is available */
        if (xQueueReceive(time_queue, &msg, portMAX_DELAY) == pdTRUE) {
            uart_printf("Speech Task: Announcing %02d:%02d:%02d\r\n",
                        msg.hour, msg.minute, msg.second);
            speak_time(msg.hour, msg.minute, msg.second);
        }
    }
}
