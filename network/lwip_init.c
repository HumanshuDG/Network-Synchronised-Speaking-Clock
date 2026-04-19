/*
 * lwip_init.c
 * lwIP stack initialization for the Speaking Clock project.
 *
 * Initializes the lwIP stack in NO_SYS (bare-metal) mode,
 * configures the LAN9118 network interface, and starts DHCP.
 *
 * The lwip_poll() function must be called periodically to:
 *   1. Poll the Ethernet driver for received packets
 *   2. Process lwIP timeouts (ARP, DHCP, DNS, etc.)
 */

#include <string.h>
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/ip4_addr.h"
#include "netif/ethernet.h"

/* External: LAN9118 driver */
extern err_t lan9118_netif_init(struct netif *netif);
extern void  lan9118_poll(struct netif *netif);

/* External: UART for debug output */
extern void uart_puts(const char *s);
extern int  uart_printf(const char *fmt, ...);

/* The network interface instance */
static struct netif netif;

/* Flag: set when DHCP has assigned an IP */
static volatile int dhcp_done = 0;

/* ---------------------------------------------------------------------------
 * Netif status callback — called when the interface gets/loses an IP address
 * --------------------------------------------------------------------------- */
static void netif_status_callback(struct netif *nif)
{
    if (!ip4_addr_isany_val(*netif_ip4_addr(nif))) {
        dhcp_done = 1;
        uart_printf("Network: IP address obtained via DHCP\r\n");
        uart_printf("  IP:      %s\r\n",
                     ip4addr_ntoa(netif_ip4_addr(nif)));
        uart_printf("  Netmask: %s\r\n",
                     ip4addr_ntoa(netif_ip4_netmask(nif)));
        uart_printf("  Gateway: %s\r\n",
                     ip4addr_ntoa(netif_ip4_gw(nif)));
    }
}

/* ---------------------------------------------------------------------------
 * network_init — initialize the lwIP stack and start DHCP
 * --------------------------------------------------------------------------- */
void network_init(void)
{
    ip4_addr_t ipaddr, netmask, gw;

    uart_puts("Initializing lwIP stack...\r\n");

    /* Initialize the lwIP stack */
    lwip_init();
    uart_puts("lwIP init done.\r\n");

    /* Start with zero IP — DHCP will assign one */
    ip4_addr_set_zero(&ipaddr);
    ip4_addr_set_zero(&netmask);
    ip4_addr_set_zero(&gw);

    uart_puts("Adding network interface...\r\n");

    /* Add the LAN9118 network interface */
    netif_add(&netif, &ipaddr, &netmask, &gw,
              NULL, lan9118_netif_init, ethernet_input);

    uart_puts("Network interface added.\r\n");

    /* Set as default interface */
    netif_set_default(&netif);

    /* Register status callback */
    netif_set_status_callback(&netif, netif_status_callback);

    /* Bring the interface up */
    netif_set_up(&netif);

    /* Start DHCP */
    uart_puts("Starting DHCP...\r\n");
    dhcp_start(&netif);
}

/* ---------------------------------------------------------------------------
 * lwip_poll — must be called periodically to process network events
 * --------------------------------------------------------------------------- */
void lwip_periodic_handle(void)
{
    /* Poll the Ethernet driver for received packets */
    lan9118_poll(&netif);

    /* Process lwIP timers (ARP, DHCP, DNS timeouts) */
    sys_check_timeouts();
}

/* ---------------------------------------------------------------------------
 * network_is_up — returns 1 if DHCP has assigned an IP address
 * --------------------------------------------------------------------------- */
int network_is_up(void)
{
    return dhcp_done;
}

/* ---------------------------------------------------------------------------
 * get_netif — returns a pointer to the network interface
 * --------------------------------------------------------------------------- */
struct netif *get_netif(void)
{
    return &netif;
}
