#!/bin/bash
# ==============================================================================
# setup_network.sh
# Configure host networking for QEMU TAP interface
#
# Creates a TAP interface (tap0) and sets up NAT so the QEMU guest
# can reach the internet (needed for NTP server access).
#
# Usage: sudo ./scripts/setup_network.sh [up|down]
#
# Requirements: root privileges
# ==============================================================================

set -e

# Auto-detect the default internet-facing interface
DEFAULT_IFACE=$(ip route | grep default | awk '{print $5}' | head -1)
TAP_IFACE="tap0"
TAP_IP="192.168.100.1"
TAP_SUBNET="192.168.100.0/24"

if [ "$(id -u)" -ne 0 ]; then
    echo "Error: This script must be run as root (sudo)."
    exit 1
fi

case "${1:-up}" in
    up)
        echo "=== Setting up TAP networking ==="
        echo "Default interface: $DEFAULT_IFACE"
        echo ""

        # Create TAP interface
        if ! ip link show "$TAP_IFACE" &>/dev/null; then
            echo "Creating TAP interface: $TAP_IFACE"
            ip tuntap add dev "$TAP_IFACE" mode tap user "$(logname 2>/dev/null || echo $SUDO_USER)"
        else
            echo "TAP interface $TAP_IFACE already exists"
        fi

        # Assign IP to TAP
        ip addr flush dev "$TAP_IFACE" 2>/dev/null || true
        ip addr add "$TAP_IP/24" dev "$TAP_IFACE"
        ip link set "$TAP_IFACE" up

        # Enable IP forwarding
        echo 1 > /proc/sys/net/ipv4/ip_forward

        # Set up NAT masquerading
        iptables -t nat -C POSTROUTING -s "$TAP_SUBNET" -o "$DEFAULT_IFACE" -j MASQUERADE 2>/dev/null || \
            iptables -t nat -A POSTROUTING -s "$TAP_SUBNET" -o "$DEFAULT_IFACE" -j MASQUERADE

        iptables -C FORWARD -i "$TAP_IFACE" -o "$DEFAULT_IFACE" -j ACCEPT 2>/dev/null || \
            iptables -A FORWARD -i "$TAP_IFACE" -o "$DEFAULT_IFACE" -j ACCEPT

        iptables -C FORWARD -i "$DEFAULT_IFACE" -o "$TAP_IFACE" -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || \
            iptables -A FORWARD -i "$DEFAULT_IFACE" -o "$TAP_IFACE" -m state --state RELATED,ESTABLISHED -j ACCEPT

        echo ""
        echo "=== Network setup complete ==="
        echo "TAP IP:   $TAP_IP"
        echo "Subnet:   $TAP_SUBNET"
        echo "Gateway:  $TAP_IP (for QEMU guest)"
        echo ""
        echo "QEMU guest should use:"
        echo "  Static IP: 192.168.100.2"
        echo "  Netmask:   255.255.255.0"
        echo "  Gateway:   $TAP_IP"
        echo "  DNS:       8.8.8.8"
        echo ""
        echo "Or use DHCP if a DHCP server is configured."
        ;;

    down)
        echo "=== Tearing down TAP networking ==="

        # Remove iptables rules
        iptables -t nat -D POSTROUTING -s "$TAP_SUBNET" -o "$DEFAULT_IFACE" -j MASQUERADE 2>/dev/null || true
        iptables -D FORWARD -i "$TAP_IFACE" -o "$DEFAULT_IFACE" -j ACCEPT 2>/dev/null || true
        iptables -D FORWARD -i "$DEFAULT_IFACE" -o "$TAP_IFACE" -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || true

        # Remove TAP interface
        if ip link show "$TAP_IFACE" &>/dev/null; then
            ip link set "$TAP_IFACE" down
            ip tuntap del dev "$TAP_IFACE" mode tap
            echo "Removed TAP interface: $TAP_IFACE"
        fi

        echo "=== Teardown complete ==="
        ;;

    *)
        echo "Usage: $0 [up|down]"
        exit 1
        ;;
esac
