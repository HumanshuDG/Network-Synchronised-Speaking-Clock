/*
 * ntp_client.c
 * NTP (Network Time Protocol) client implementation using lwIP raw UDP API.
 *
 * Constructs a standard 48-byte NTP request, sends it to a public NTP server,
 * receives the response, and extracts the transmit timestamp.
 *
 * The timestamp is converted from NTP epoch (1900-01-01) to Unix epoch (1970-01-01)
 * and then adjusted to Indian Standard Time (UTC+5:30).
 */

#include <stdint.h>
#include <string.h>

#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "lwip/timeouts.h"

#include "time_msg.h"

/* External functions */
extern void uart_puts(const char *s);
extern int  uart_printf(const char *fmt, ...);
extern void lwip_periodic_handle(void);

/* ---------------------------------------------------------------------------
 * NTP Constants
 * --------------------------------------------------------------------------- */
#define NTP_PORT                123
#define NTP_PACKET_SIZE         48

/* Seconds between 1900-01-01 (NTP epoch) and 1970-01-01 (Unix epoch) */
#define NTP_UNIX_EPOCH_DIFF     2208988800UL

/* IST offset: UTC + 5 hours 30 minutes = 19800 seconds */
#define IST_OFFSET_SECONDS      19800

/* NTP request timeout in polling cycles */
#define NTP_TIMEOUT_CYCLES      5000

/* ---------------------------------------------------------------------------
 * NTP Packet Structure (48 bytes)
 *
 * Byte 0: LI (2 bits) | VN (3 bits) | Mode (3 bits)
 *   For client request: LI=0, VN=4, Mode=3 → 0x23
 *
 * Bytes 40-43: Transmit Timestamp (seconds) — this is what we extract
 * --------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * State for async NTP response
 * --------------------------------------------------------------------------- */
static volatile int      ntp_response_received = 0;
static volatile uint32_t ntp_timestamp = 0;

/* ---------------------------------------------------------------------------
 * NTP UDP receive callback
 * --------------------------------------------------------------------------- */
static void ntp_recv_callback(void *arg, struct udp_pcb *pcb,
                              struct pbuf *p, const ip_addr_t *addr,
                              u16_t port)
{
    (void)arg;
    (void)pcb;
    (void)addr;
    (void)port;

    if (p == NULL) {
        return;
    }

    if (p->tot_len >= NTP_PACKET_SIZE) {
        uint8_t buf[NTP_PACKET_SIZE];
        pbuf_copy_partial(p, buf, NTP_PACKET_SIZE, 0);

        /*
         * Extract the Transmit Timestamp (bytes 40-43)
         * This is the time the server sent the response, in seconds
         * since 1900-01-01 00:00:00 UTC, in network byte order (big-endian).
         */
        ntp_timestamp = ((uint32_t)buf[40] << 24) |
                        ((uint32_t)buf[41] << 16) |
                        ((uint32_t)buf[42] <<  8) |
                        ((uint32_t)buf[43]);

        ntp_response_received = 1;
    }

    pbuf_free(p);
}

/* ---------------------------------------------------------------------------
 * ntp_get_time — send NTP request and wait for response
 *
 * Uses a hardcoded NTP server IP (time.cloudflare.com: 162.159.200.1)
 * as the primary server for simplicity. DNS resolution adds complexity
 * that is not essential for demonstrating NTP protocol parsing.
 *
 * Returns: 0 on success, -1 on failure
 * --------------------------------------------------------------------------- */
int ntp_get_time(time_msg *msg)
{
    struct udp_pcb *pcb;
    struct pbuf *p;
    ip_addr_t server_addr;
    uint8_t ntp_request[NTP_PACKET_SIZE];
    int timeout;
    uint32_t unix_time;
    uint32_t ist_time;

    uart_puts("NTP: Sending time request...\r\n");

    /* Reset state */
    ntp_response_received = 0;
    ntp_timestamp = 0;

    /* Create UDP PCB */
    pcb = udp_new();
    if (pcb == NULL) {
        uart_puts("NTP: Failed to create UDP PCB\r\n");
        return -1;
    }

    /* Bind to any local port */
    udp_bind(pcb, IP_ADDR_ANY, 0);

    /* Set receive callback */
    udp_recv(pcb, ntp_recv_callback, NULL);

    /* Build the NTP request packet */
    memset(ntp_request, 0, NTP_PACKET_SIZE);
    /*
     * Byte 0: LI=0 (no warning), VN=4 (NTPv4), Mode=3 (client)
     * Binary: 00 100 011 = 0x23
     */
    ntp_request[0] = 0x23;

    /* Allocate pbuf for the request */
    p = pbuf_alloc(PBUF_TRANSPORT, NTP_PACKET_SIZE, PBUF_RAM);
    if (p == NULL) {
        uart_puts("NTP: Failed to allocate pbuf\r\n");
        udp_remove(pcb);
        return -1;
    }
    memcpy(p->payload, ntp_request, NTP_PACKET_SIZE);

    /*
     * NTP Server: time.cloudflare.com (162.159.200.1)
     * Using a hardcoded IP avoids DNS resolution complexity.
     * Alternative: 216.239.35.0 (time.google.com)
     */
    IP4_ADDR(&server_addr, 162, 159, 200, 1);

    uart_printf("NTP: Sending to %s:%d\r\n",
                ip4addr_ntoa(ip_2_ip4(&server_addr)), NTP_PORT);

    /* Send the NTP request */
    err_t err = udp_sendto(pcb, p, &server_addr, NTP_PORT);
    pbuf_free(p);

    if (err != ERR_OK) {
        uart_printf("NTP: Send failed (err=%d)\r\n", err);
        udp_remove(pcb);
        return -1;
    }

    uart_puts("NTP: Request sent, waiting for response...\r\n");

    /* Poll for response with timeout */
    timeout = NTP_TIMEOUT_CYCLES;
    while (!ntp_response_received && timeout > 0) {
        lwip_periodic_handle();
        timeout--;

        /* Small delay to avoid busy-spin */
        for (volatile int i = 0; i < 10000; i++)
            ;
    }

    /* Clean up UDP PCB */
    udp_remove(pcb);

    if (!ntp_response_received) {
        uart_puts("NTP: Response timeout!\r\n");
        return -1;
    }

    uart_printf("NTP: Received timestamp = %u (NTP epoch)\r\n", ntp_timestamp);

    /*
     * Convert NTP timestamp to Unix time
     * NTP epoch: 1900-01-01 00:00:00
     * Unix epoch: 1970-01-01 00:00:00
     * Difference: 2,208,988,800 seconds
     */
    unix_time = ntp_timestamp - NTP_UNIX_EPOCH_DIFF;

    /*
     * Convert to Indian Standard Time (IST = UTC + 5:30)
     * Offset = 5 * 3600 + 30 * 60 = 19800 seconds
     */
    ist_time = unix_time + IST_OFFSET_SECONDS;

    /* Extract hours, minutes, seconds from seconds-of-day */
    uint32_t seconds_of_day = ist_time % 86400;
    msg->hour   = (uint8_t)(seconds_of_day / 3600);
    msg->minute = (uint8_t)((seconds_of_day % 3600) / 60);
    msg->second = (uint8_t)(seconds_of_day % 60);

    uart_printf("NTP: IST time = %02d:%02d:%02d\r\n",
                msg->hour, msg->minute, msg->second);

    return 0;
}
