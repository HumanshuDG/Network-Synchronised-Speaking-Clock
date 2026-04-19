/*
 * lwipopts.h
 * lwIP configuration for Speaking Clock project
 * Bare-metal mode (NO_SYS=1) with UDP, DHCP, DNS
 * No TCP required.
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* ---------------------------------------------------------------------------
 * System Mode: NO_SYS = 1 (bare-metal, no OS integration)
 * We drive lwIP from a polling loop inside a FreeRTOS task
 * --------------------------------------------------------------------------- */
#define NO_SYS                          1
#define NO_SYS_NO_TIMERS                0
#define LWIP_TIMERS                     1

/* Skip lwIP internal compile/runtime checks that can cause false asserts */
#define LWIP_SKIP_PACKING_CHECK         1
#define LWIP_SKIP_CONST_CHECK           1

/* ---------------------------------------------------------------------------
 * Memory Configuration
 * --------------------------------------------------------------------------- */
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)

/* Memory pool sizes */
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                0
#define MEMP_NUM_TCP_PCB_LISTEN         0
#define MEMP_NUM_TCP_SEG                0
#define MEMP_NUM_ARP_QUEUE              8
#define MEMP_NUM_SYS_TIMEOUT            8
#define MEMP_NUM_NETBUF                 0
#define MEMP_NUM_NETCONN                0

/* Pbuf pool */
#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1536

/* ---------------------------------------------------------------------------
 * Protocol Configuration
 * --------------------------------------------------------------------------- */
/* IPv4 */
#define LWIP_IPV4                       1
#define LWIP_IPV6                       0

/* UDP — required for NTP */
#define LWIP_UDP                        1

/* TCP — not needed */
#define LWIP_TCP                        0

/* DHCP — required for IP assignment */
#define LWIP_DHCP                       1
#define DHCP_DOES_ARP_CHECK             0
#define LWIP_DHCP_DOES_ACD_CHECK        0
#define LWIP_ACD                        0

/* DNS — for NTP server hostname resolution */
#define LWIP_DNS                        1
#define DNS_MAX_SERVERS                 2

/* ICMP — useful for ping debugging */
#define LWIP_ICMP                       1

/* ARP */
#define LWIP_ARP                        1
#define ARP_TABLE_SIZE                  10
#define ARP_QUEUEING                    1

/* RAW sockets */
#define LWIP_RAW                        0

/* ---------------------------------------------------------------------------
 * Network Interface
 * --------------------------------------------------------------------------- */
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_SINGLE_NETIF              1

/* Ethernet */
#define LWIP_ETHERNET                   1
#define ETH_PAD_SIZE                    0

/* ---------------------------------------------------------------------------
 * Checksum Configuration
 * LAN9118 in QEMU does NOT support hardware checksum offloading
 * All checksums must be computed in software
 * --------------------------------------------------------------------------- */
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_CHECK_ICMP             1

/* ---------------------------------------------------------------------------
 * Socket/Netconn API — disabled (using raw API only)
 * --------------------------------------------------------------------------- */
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0

/* ---------------------------------------------------------------------------
 * Statistics — disabled to save memory
 * --------------------------------------------------------------------------- */
#define LWIP_STATS                      0
#define LWIP_STATS_DISPLAY              0

/* ---------------------------------------------------------------------------
 * Debug — enable selectively for troubleshooting
 * --------------------------------------------------------------------------- */
#define LWIP_DEBUG                      0

#if LWIP_DEBUG
#define ETHARP_DEBUG                    LWIP_DBG_ON
#define NETIF_DEBUG                     LWIP_DBG_ON
#define DHCP_DEBUG                      LWIP_DBG_ON
#define UDP_DEBUG                       LWIP_DBG_ON
#define IP_DEBUG                        LWIP_DBG_ON
#define DNS_DEBUG                       LWIP_DBG_ON
#endif

/* ---------------------------------------------------------------------------
 * Miscellaneous
 * --------------------------------------------------------------------------- */
#define LWIP_PROVIDE_ERRNO              1
#define LWIP_RAND()                     ((u32_t)rand())

/* Disable features not needed */
#define LWIP_AUTOIP                     0
#define LWIP_DHCP_AUTOIP_COOP           0

#endif /* LWIPOPTS_H */
