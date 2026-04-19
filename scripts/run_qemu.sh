#!/bin/bash
# ==============================================================================
# run_qemu.sh
# Launch QEMU with the speaking clock firmware
#
# Usage:
#   ./scripts/run_qemu.sh              # User-mode networking (NAT, no TAP)
#   ./scripts/run_qemu.sh --tap        # TAP networking (requires setup_network.sh)
#
# Press Ctrl+A, X to exit QEMU
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
ELF="$PROJECT_DIR/output/speaking_clock.elf"

# Check if firmware exists
if [ ! -f "$ELF" ]; then
    echo "Error: Firmware not found at $ELF"
    echo "Run 'make' first to build the project."
    exit 1
fi

# Check QEMU is installed
if ! command -v qemu-system-arm &> /dev/null; then
    echo "Error: qemu-system-arm not found."
    echo "Install with: sudo apt install qemu-system-arm"
    exit 1
fi

echo "================================================"
echo "  Network-Synchronised Speaking Clock"
echo "  QEMU MPS2-AN385 (Cortex-M3)"
echo "  Press Ctrl+A, X to exit"
echo "================================================"
echo ""

if [ "$1" == "--tap" ]; then
    echo "Using TAP networking (tap0)..."
    echo "Make sure you've run: sudo ./scripts/setup_network.sh"
    echo ""
    qemu-system-arm \
        -machine mps2-an385 \
        -cpu cortex-m3 \
        -kernel "$ELF" \
        -monitor none \
        -nographic \
        -serial stdio \
        -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
        -device lan9118,netdev=net0
else
    echo "Using user-mode networking (SLIRP NAT)..."
    echo ""
    qemu-system-arm \
        -machine mps2-an385 \
        -cpu cortex-m3 \
        -kernel "$ELF" \
        -monitor none \
        -nographic \
        -serial stdio \
        -netdev user,id=net0 \
        -device lan9118,netdev=net0
fi
