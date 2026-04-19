/*
 * time_msg.h
 * Time message structure passed between NTP Client Task and Speech Task
 * via a FreeRTOS queue.
 */

#ifndef TIME_MSG_H
#define TIME_MSG_H

#include <stdint.h>

/*
 * time_msg
 * Holds the current time in IST (Indian Standard Time, UTC+5:30)
 * Fields are in 24-hour format.
 */
typedef struct {
    uint8_t hour;       /* 0 - 23 */
    uint8_t minute;     /* 0 - 59 */
    uint8_t second;     /* 0 - 59 */
} time_msg;

#endif /* TIME_MSG_H */
