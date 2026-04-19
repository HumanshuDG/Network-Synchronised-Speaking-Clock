# ==============================================================================
# Makefile for Network-Synchronised Speaking Clock
# Target: MPS2-AN385 (Cortex-M3) on QEMU
# Uses: FreeRTOS, lwIP, LAN9118 Ethernet
# ==============================================================================

# Toolchain
CC       = arm-none-eabi-gcc
AS       = arm-none-eabi-gcc
LD       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
OBJDUMP  = arm-none-eabi-objdump
SIZE     = arm-none-eabi-size

# Output
OUTPUT_DIR = output
TARGET     = $(OUTPUT_DIR)/speaking_clock
ELF        = $(TARGET).elf
BIN        = $(TARGET).bin
MAP        = $(TARGET).map

# ==============================================================================
# Compiler Flags
# ==============================================================================
CFLAGS  = -mcpu=cortex-m3 -mthumb
CFLAGS += -O1 -g3
CFLAGS += -ffreestanding -ffunction-sections -fdata-sections
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -MMD -MP

ASFLAGS = -mcpu=cortex-m3 -mthumb -g3

LDFLAGS  = -mcpu=cortex-m3 -mthumb
LDFLAGS += -T linker/stm32.ld
LDFLAGS += -Wl,-Map=$(MAP)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -nostartfiles
LDFLAGS += -specs=nano.specs -specs=nosys.specs

# ==============================================================================
# Include Paths
# ==============================================================================
INCLUDES  = -Iinclude
INCLUDES += -Iconfig
INCLUDES += -Iconfig/arch
INCLUDES += -Ifreertos/include
INCLUDES += -Ifreertos/portable/GCC/ARM_CM3
INCLUDES += -Ilwip/src/include
INCLUDES += -Inetwork

# lwIP arch include (cc.h is in config/arch/)
# lwIP looks for "arch/cc.h", so we point it to config/
INCLUDES += -Iconfig

CFLAGS += $(INCLUDES)

# ==============================================================================
# Source Files
# ==============================================================================

# Application sources
APP_SRCS  = src/main.c
APP_SRCS += src/key_task.c
APP_SRCS += src/ntp_task.c
APP_SRCS += src/speech_task.c

# Network sources
NET_SRCS  = network/lwip_init.c
NET_SRCS += network/ntp_client.c
NET_SRCS += network/netif_qemu.c

# FreeRTOS kernel
RTOS_SRCS  = freertos/tasks.c
RTOS_SRCS += freertos/queue.c
RTOS_SRCS += freertos/list.c
RTOS_SRCS += freertos/timers.c
RTOS_SRCS += freertos/event_groups.c
RTOS_SRCS += freertos/portable/GCC/ARM_CM3/port.c
RTOS_SRCS += freertos/portable/MemMang/heap_4.c

# lwIP core
LWIP_CORE  = lwip/src/core/init.c
LWIP_CORE += lwip/src/core/mem.c
LWIP_CORE += lwip/src/core/memp.c
LWIP_CORE += lwip/src/core/pbuf.c
LWIP_CORE += lwip/src/core/netif.c
LWIP_CORE += lwip/src/core/udp.c
LWIP_CORE += lwip/src/core/def.c
LWIP_CORE += lwip/src/core/dns.c
LWIP_CORE += lwip/src/core/inet_chksum.c
LWIP_CORE += lwip/src/core/ip.c
LWIP_CORE += lwip/src/core/timeouts.c
LWIP_CORE += lwip/src/core/raw.c

# lwIP IPv4
LWIP_IPV4  = lwip/src/core/ipv4/ip4.c
LWIP_IPV4 += lwip/src/core/ipv4/ip4_addr.c
LWIP_IPV4 += lwip/src/core/ipv4/ip4_frag.c
LWIP_IPV4 += lwip/src/core/ipv4/etharp.c
LWIP_IPV4 += lwip/src/core/ipv4/dhcp.c
LWIP_IPV4 += lwip/src/core/ipv4/icmp.c

# lwIP netif
LWIP_NETIF = lwip/src/netif/ethernet.c

# All lwIP sources
LWIP_SRCS = $(LWIP_CORE) $(LWIP_IPV4) $(LWIP_NETIF)

# Startup assembly
ASM_SRCS = startup/startup_stm32.s

# All sources
C_SRCS = $(APP_SRCS) $(NET_SRCS) $(RTOS_SRCS) $(LWIP_SRCS)

# ==============================================================================
# Object Files
# ==============================================================================
C_OBJS   = $(C_SRCS:%.c=$(OUTPUT_DIR)/%.o)
ASM_OBJS = $(ASM_SRCS:%.s=$(OUTPUT_DIR)/%.o)
ALL_OBJS = $(C_OBJS) $(ASM_OBJS)

# Dependency files
DEPS = $(C_OBJS:%.o=%.d)

# ==============================================================================
# Rules
# ==============================================================================

.PHONY: all clean flash flash-tap debug tts flash-pipe zip

all: $(ELF) $(BIN)
	@echo ""
	@echo "Build complete!"
	@$(SIZE) $(ELF)

# Create output directories
$(OUTPUT_DIR)/%.o: | $(OUTPUT_DIR)

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

# Compile C sources
$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup
$(OUTPUT_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Link
$(ELF): $(ALL_OBJS)
	@echo ""
	@echo "=== Linking ==="
	$(LD) $(LDFLAGS) $^ -o $@

# Create binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Clean
clean:
	rm -rf $(OUTPUT_DIR)

# Run in QEMU (user-mode networking - built-in LAN9118)
flash: $(ELF)
	@echo "Starting QEMU (press Ctrl+A, X to exit)..."
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $(ELF) \
		-monitor none \
		-nographic \
		-serial stdio \
		-nic user

# Run with TAP networking (requires setup_network.sh)
flash-tap: $(ELF)
	@echo "Starting QEMU with TAP networking..."
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $(ELF) \
		-monitor none \
		-nographic \
		-serial stdio \
		-nic tap,ifname=tap0,script=no,downscript=no

# Debug with GDB
debug: $(ELF)
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $(ELF) \
		-monitor none \
		-nographic \
		-serial stdio \
		-nic user \
		-S -gdb tcp::1234

# Run with TTS bridge (speech synthesis)
tts: $(ELF)
	@echo "Starting Speaking Clock with TTS..."
	python3 scripts/tts_bridge.py

# Run QEMU with output piped to TTS bridge
flash-pipe: $(ELF)
	@echo "Starting QEMU with piped TTS (press 't' then wait)..."
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $(ELF) \
		-monitor none \
		-nographic \
		-serial stdio \
		-nic user 2>&1 | python3 scripts/tts_bridge.py --pipe

# Create submission zip
zip: clean
	cd .. && zip -r speaking_clock_submission.zip speaking_clock/ \
		-x "speaking_clock/.git/*" \
		-x "speaking_clock/output/*"

# Include dependency files
-include $(DEPS)
