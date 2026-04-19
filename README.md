# National Time Network-Synchronised Speaking Clock using STM32, FreeRTOS and QEMU

<video src="Demonstration.mp4" width=100%>
</video>

[Demonstration](Demonstration.mp4)

## Instructions to Run

### Prerequisites

Install the following packages (Ubuntu):

```bash
# ARM cross-compiler toolchain
sudo apt install gcc-arm-none-eabi

# QEMU system emulator for ARM
sudo apt install qemu-system-arm

# Text-to-speech engine (for speech output)
sudo apt install espeak

# Python 3 (for TTS bridge)
sudo apt install python3
```

### Build

```bash
cd speaking_clock
make clean && make
```

### Run (Console Only)

Run the firmware in QEMU with user-mode (NAT) networking:

```bash
make flash
```

> Press `t` to request the time. The TOKEN/END output will appear in the console. Press `Ctrl+A`, then `X` to exit QEMU.

### Run with Speech (TTS Bridge)

Launch QEMU through the TTS bridge for actual spoken audio:

```bash
make tts
```

This starts QEMU as a subprocess with keyboard forwarding. Press `t` to hear the time spoken aloud via eSpeak. Press `Ctrl+C` to exit.

