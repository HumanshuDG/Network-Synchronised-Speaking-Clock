/*
 * netif_qemu.c
 * LAN9118 (SMSC9118) Ethernet driver for QEMU MPS2-AN385
 *
 * This driver provides the lwIP netif interface for the LAN9118
 * Ethernet controller emulated by QEMU at base address 0x40200000.
 *
 * Operates in polling mode — lan9118_poll() is called periodically
 * from the NTP task to process received packets.
 */

#include <stdint.h>
#include <string.h>

#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/ip4_addr.h"
#include "netif/ethernet.h"

/* External UART for debug */
extern void uart_puts(const char *s);
extern int  uart_printf(const char *fmt, ...);

/* ---------------------------------------------------------------------------
 * LAN9118 Register Definitions
 * Base address on MPS2-AN385: 0x40200000
 * --------------------------------------------------------------------------- */
#define LAN9118_BASE            0x40200000UL

/* Direct registers (offset from base) */
#define REG(off)                (*(volatile uint32_t *)(LAN9118_BASE + (off)))

#define RX_DATA_FIFO            REG(0x00)
#define TX_DATA_FIFO            REG(0x20)
#define RX_STATUS_FIFO          REG(0x40)
#define RX_STATUS_FIFO_PEEK     REG(0x44)
#define TX_STATUS_FIFO          REG(0x48)
#define TX_STATUS_FIFO_PEEK     REG(0x4C)

#define ID_REV                  REG(0x50)
#define IRQ_CFG                 REG(0x54)
#define INT_STS                 REG(0x58)
#define INT_EN                  REG(0x5C)
#define BYTE_TEST               REG(0x64)
#define FIFO_INT                REG(0x68)
#define RX_CFG                  REG(0x6C)
#define TX_CFG                  REG(0x70)
#define HW_CFG                  REG(0x74)
#define RX_DP_CTRL              REG(0x78)
#define RX_FIFO_INF             REG(0x7C)
#define TX_FIFO_INF             REG(0x80)
#define PMT_CTRL                REG(0x84)
#define GPIO_CFG                REG(0x88)
#define GPT_CFG                 REG(0x8C)
#define GPT_CNT                 REG(0x90)
#define FREE_RUN                REG(0x9C)
#define RX_DROP                 REG(0xA0)
#define MAC_CSR_CMD             REG(0xA4)
#define MAC_CSR_DATA            REG(0xA8)
#define AFC_CFG                 REG(0xAC)

/* MAC registers (accessed indirectly via MAC_CSR_CMD / MAC_CSR_DATA) */
#define MAC_CR                  1
#define MAC_ADDRH               2
#define MAC_ADDRL               3
#define MAC_HASHH               4
#define MAC_HASHL               5
#define MAC_MII_ACC             6
#define MAC_MII_DATA            7
#define MAC_FLOW                8

/* MAC_CSR_CMD bits */
#define MAC_CSR_CMD_BUSY        (1U << 31)
#define MAC_CSR_CMD_READ        (1U << 30)

/* TX_CMD_A bits */
#define TX_CMD_A_INT_ON_COMP    (1U << 31)
#define TX_CMD_A_FIRST_SEG      (1U << 13)
#define TX_CMD_A_LAST_SEG       (1U << 12)

/* TX_CFG bits */
#define TX_CFG_TX_ON            (1U << 1)
#define TX_CFG_TXSAO            (1U << 2)

/* HW_CFG bits */
#define HW_CFG_SRST             (1U << 0)
#define HW_CFG_MBO              (1U << 20)

/* MAC_CR bits */
#define MAC_CR_RXEN             (1U << 2)
#define MAC_CR_TXEN             (1U << 3)
#define MAC_CR_FDPX             (1U << 20)

/* ---------------------------------------------------------------------------
 * MAC address for the virtual NIC
 * --------------------------------------------------------------------------- */
static const uint8_t mac_addr[6] = {0x00, 0x02, 0xF7, 0xF0, 0x00, 0x01};

/* ---------------------------------------------------------------------------
 * Internal helpers — MAC register access with timeout protection
 * --------------------------------------------------------------------------- */

static int mac_wait_busy(void)
{
    int timeout = 10000;
    while ((MAC_CSR_CMD & MAC_CSR_CMD_BUSY) && --timeout)
        ;
    return timeout > 0;
}

static uint32_t mac_read(uint8_t reg)
{
    mac_wait_busy();
    MAC_CSR_CMD = MAC_CSR_CMD_BUSY | MAC_CSR_CMD_READ | (uint32_t)reg;
    mac_wait_busy();
    return MAC_CSR_DATA;
}

static void mac_write(uint8_t reg, uint32_t val)
{
    mac_wait_busy();
    MAC_CSR_DATA = val;
    MAC_CSR_CMD = MAC_CSR_CMD_BUSY | (uint32_t)reg;
    mac_wait_busy();
}

/* ---------------------------------------------------------------------------
 * LAN9118 Initialization
 * --------------------------------------------------------------------------- */
static void lan9118_hw_init(void)
{
    uint32_t id_rev;
    int timeout;

    uart_puts("LAN9118: Initializing...\r\n");

    /* Read and verify the chip ID */
    id_rev = ID_REV;
    uart_printf("LAN9118: ID_REV = 0x%08x\r\n", (unsigned)id_rev);

    /* Verify BYTE_TEST register (should always read 0x87654321) */
    uart_printf("LAN9118: BYTE_TEST = 0x%08x\r\n", (unsigned)BYTE_TEST);

    /* Soft reset */
    HW_CFG |= HW_CFG_SRST;
    timeout = 100000;
    while ((HW_CFG & HW_CFG_SRST) && --timeout) {
        /* spin */
    }
    if (timeout == 0) {
        uart_puts("LAN9118: WARNING - soft reset timeout (continuing)\r\n");
    } else {
        uart_puts("LAN9118: Soft reset complete\r\n");
    }

    /* Small delay for reset settling */
    for (volatile int i = 0; i < 100000; i++)
        ;

    /* Set must-be-one bit in HW_CFG */
    HW_CFG = HW_CFG_MBO;

    /* Configure TX */
    TX_CFG = TX_CFG_TXSAO;  /* Flush TX status FIFO */

    /* Configure RX - no offset */
    RX_CFG = 0;

    /* Drain any stale RX packets */
    timeout = 100;
    while ((RX_FIFO_INF & 0x00FF0000) && --timeout) {
        volatile uint32_t dummy = RX_STATUS_FIFO;
        (void)dummy;
    }

    /* Set MAC address */
    uart_puts("LAN9118: Setting MAC address...\r\n");
    mac_write(MAC_ADDRL,
              (uint32_t)mac_addr[0] |
              ((uint32_t)mac_addr[1] << 8) |
              ((uint32_t)mac_addr[2] << 16) |
              ((uint32_t)mac_addr[3] << 24));
    mac_write(MAC_ADDRH,
              (uint32_t)mac_addr[4] |
              ((uint32_t)mac_addr[5] << 8));

    /* Enable TX and RX in MAC */
    mac_write(MAC_CR, MAC_CR_TXEN | MAC_CR_RXEN | MAC_CR_FDPX);

    /* Enable transmitter */
    TX_CFG = TX_CFG_TX_ON;

    /* Clear all interrupts */
    INT_STS = 0xFFFFFFFF;

    /* Disable all interrupts (we use polling) */
    INT_EN = 0;

    uart_printf("LAN9118: MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                mac_addr[0], mac_addr[1], mac_addr[2],
                mac_addr[3], mac_addr[4], mac_addr[5]);
    uart_puts("LAN9118: Initialization complete\r\n");
}

/* ---------------------------------------------------------------------------
 * lwIP netif output — transmit a frame
 * --------------------------------------------------------------------------- */
static err_t lan9118_linkoutput(struct netif *netif, struct pbuf *p)
{
    uint32_t tx_cmd_a, tx_cmd_b;
    struct pbuf *q;
    uint32_t total_len = p->tot_len;
    uint32_t *buf;
    uint32_t i, words;

    (void)netif;

    /* TX Command A: first+last segment, buffer size = total length */
    tx_cmd_a = TX_CMD_A_FIRST_SEG | TX_CMD_A_LAST_SEG | (total_len & 0x7FF);
    /* TX Command B: packet length */
    tx_cmd_b = (total_len & 0x7FF);

    /* Write TX commands to the FIFO */
    TX_DATA_FIFO = tx_cmd_a;
    TX_DATA_FIFO = tx_cmd_b;

    /* Write frame data, word by word */
    for (q = p; q != NULL; q = q->next) {
        buf = (uint32_t *)q->payload;
        words = (q->len + 3) / 4;
        for (i = 0; i < words; i++) {
            TX_DATA_FIFO = buf[i];
        }
    }

    return ERR_OK;
}

/* ---------------------------------------------------------------------------
 * lan9118_poll — check for and process received packets
 * Must be called periodically from the application.
 * --------------------------------------------------------------------------- */
void lan9118_poll(struct netif *netif)
{
    uint32_t rx_status;
    uint32_t pkt_len, raw_len;
    uint32_t words, i;
    struct pbuf *p;
    uint32_t *buf;

    /* Check if any packets in the RX FIFO (bits [23:16] = # used status words) */
    while ((RX_FIFO_INF & 0x00FF0000) != 0) {
        /* Read RX status */
        rx_status = RX_STATUS_FIFO;
        raw_len = (rx_status >> 16) & 0x3FFF;

        if (raw_len == 0) {
            continue;
        }

        /* Packet length includes CRC (4 bytes) */
        pkt_len = (raw_len > 4) ? (raw_len - 4) : raw_len;

        /* Number of 32-bit words to read from FIFO */
        words = (raw_len + 3) / 4;

        /* Allocate a pbuf */
        p = pbuf_alloc(PBUF_RAW, pkt_len, PBUF_POOL);

        if (p != NULL) {
            /* Read frame data into pbuf chain */
            struct pbuf *q;
            uint32_t read_words = 0;

            for (q = p; q != NULL; q = q->next) {
                buf = (uint32_t *)q->payload;
                uint32_t chunk_words = (q->len + 3) / 4;
                for (i = 0; i < chunk_words && read_words < words; i++, read_words++) {
                    buf[i] = RX_DATA_FIFO;
                }
            }

            /* Read any remaining FIFO words (padding/CRC) */
            while (read_words < words) {
                volatile uint32_t dummy = RX_DATA_FIFO;
                (void)dummy;
                read_words++;
            }

            /* Pass to lwIP */
            if (netif->input(p, netif) != ERR_OK) {
                pbuf_free(p);
            }
        } else {
            /* No memory — flush the packet from FIFO */
            for (i = 0; i < words; i++) {
                volatile uint32_t dummy = RX_DATA_FIFO;
                (void)dummy;
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * lwIP netif init callback
 * --------------------------------------------------------------------------- */
err_t lan9118_netif_init(struct netif *netif)
{
    /* Set interface name */
    netif->name[0] = 'e';
    netif->name[1] = 'n';

    /* Set output functions */
    netif->output = etharp_output;
    netif->linkoutput = lan9118_linkoutput;

    /* Set hardware address */
    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, mac_addr, 6);

    /* Set MTU */
    netif->mtu = 1500;

    /* Set device capabilities */
    netif->flags = NETIF_FLAG_BROADCAST |
                   NETIF_FLAG_ETHARP |
                   NETIF_FLAG_LINK_UP |
                   NETIF_FLAG_UP;

    /* Initialize the hardware */
    lan9118_hw_init();

    return ERR_OK;
}
