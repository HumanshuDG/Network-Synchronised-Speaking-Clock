#!/usr/bin/env python3
"""
tts_bridge.py
Python Text-to-Speech Bridge for the Speaking Clock

Launches QEMU, reads its serial output, parses TOKEN/END protocol messages,
and synthesizes speech using eSpeak. User keyboard input is forwarded to
QEMU so you can press 't' to request the time.

Usage:
    python3 scripts/tts_bridge.py

    # Or pipe QEMU output (keyboard input won't work in this mode):
    make flash 2>&1 | python3 scripts/tts_bridge.py --pipe

Token Protocol:
    TOKEN <word>    — accumulate word into sentence
    END             — synthesize and speak the accumulated sentence

Example:
    TOKEN THE
    TOKEN TIME
    TOKEN IS
    TOKEN FOURTEEN
    TOKEN HOURS
    END

Produces speech: "The time is fourteen hours"
"""

import sys
import subprocess
import os
import threading
import select
import tty
import termios
import signal
import time


def token_to_word(token):
    """
    Convert a speech token to its spoken English word.
    Handles underscore-separated compound numbers (e.g., TWENTY_ONE → twenty one).
    """
    return token.lower().replace('_', ' ')


def speak(text):
    """
    Synthesize and play speech using eSpeak.
    Falls back to printing if eSpeak is not available.
    """
    print(f"\n🔊 Speaking: \"{text}\"")

    try:
        subprocess.run(
            ['espeak', '-v', 'en+f3', '-s', '140', text],
            check=True,
            capture_output=True,
            timeout=10
        )
    except FileNotFoundError:
        print("   (eSpeak not found — install with: sudo apt install espeak)")
        print(f"   Text: {text}")
    except subprocess.TimeoutExpired:
        print("   (eSpeak timed out)")
    except subprocess.CalledProcessError as e:
        print(f"   (eSpeak error: {e})")


def process_line(line, tokens):
    """
    Process a single line of output from QEMU.

    Args:
        line: The input line to process
        tokens: List to accumulate tokens into

    Returns:
        True if END was reached and speech was produced, False otherwise
    """
    line = line.strip()

    if line.startswith('TOKEN '):
        word = line[6:].strip()
        if word:
            tokens.append(token_to_word(word))
        return False

    elif line == 'END':
        if tokens:
            sentence = ' '.join(tokens)
            speak(sentence)
            tokens.clear()
        return True

    else:
        # Pass through non-token lines (debug output, prompts, etc.)
        if line:
            print(line)
        return False


def pipe_mode():
    """
    Pipe mode: read lines from stdin (QEMU output piped in).
    Keyboard input to QEMU is NOT possible in this mode.
    """
    print("=" * 50)
    print("  Speaking Clock — TTS Bridge (pipe mode)")
    print("  Reading token stream from stdin...")
    print("=" * 50)
    print()

    tokens = []

    try:
        for line in sys.stdin:
            process_line(line, tokens)
    except KeyboardInterrupt:
        print("\n\nTTS Bridge terminated.")
    except EOFError:
        print("\n\nInput stream ended.")

    if tokens:
        sentence = ' '.join(tokens)
        speak(sentence)


def interactive_mode():
    """
    Interactive mode: launch QEMU, forward keyboard input, parse output.
    This is the default mode that provides full functionality.
    """
    # Find the project root (one level up from scripts/)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(script_dir)
    elf_path = os.path.join(project_dir, "output", "speaking_clock.elf")

    if not os.path.exists(elf_path):
        print(f"ERROR: Firmware not found at {elf_path}")
        print("       Run 'make' first to build the firmware.")
        sys.exit(1)

    qemu_cmd = [
        "qemu-system-arm",
        "-machine", "mps2-an385",
        "-cpu", "cortex-m3",
        "-kernel", elf_path,
        "-monitor", "none",
        "-nographic",
        "-serial", "stdio",
        "-nic", "user",
    ]

    print("=" * 50)
    print("  Speaking Clock — TTS Bridge")
    print("  Launching QEMU with speech synthesis...")
    print("  Press 't' to announce the time")
    print("  Press Ctrl+C to exit")
    print("=" * 50)
    print()

    # Save terminal settings for raw mode
    old_settings = termios.tcgetattr(sys.stdin)

    # Start QEMU
    proc = subprocess.Popen(
        qemu_cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=0,
        cwd=project_dir,
    )

    tokens = []
    running = True

    def handle_sigint(sig, frame):
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, handle_sigint)

    def read_output():
        """Thread to read QEMU output and process tokens."""
        buf = b""
        try:
            while running and proc.poll() is None:
                data = proc.stdout.read(1)
                if not data:
                    break

                # Echo character to terminal
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()

                if data == b'\n':
                    line = buf.decode('utf-8', errors='replace')
                    # Remove \r from line
                    line = line.replace('\r', '')
                    process_line(line, tokens)
                    buf = b""
                else:
                    buf += data
        except Exception:
            pass

    # Start output reader thread
    output_thread = threading.Thread(target=read_output, daemon=True)
    output_thread.start()

    try:
        # Set terminal to raw mode for character-by-character input
        tty.setraw(sys.stdin.fileno())

        while running and proc.poll() is None:
            # Check for keyboard input
            if select.select([sys.stdin], [], [], 0.1)[0]:
                ch = sys.stdin.buffer.read(1)
                if ch == b'\x01':  # Ctrl+A
                    # Read next char for QEMU monitor escape
                    ch2 = sys.stdin.buffer.read(1)
                    if ch2 == b'x' or ch2 == b'X':
                        # Ctrl+A, X = exit QEMU
                        running = False
                        break
                    else:
                        proc.stdin.write(ch + ch2)
                        proc.stdin.flush()
                elif ch == b'\x03':  # Ctrl+C
                    running = False
                    break
                else:
                    proc.stdin.write(ch)
                    proc.stdin.flush()

    except Exception as e:
        print(f"\nError: {e}")
    finally:
        # Restore terminal settings
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)

        # Clean up QEMU
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()

        print("\n\nTTS Bridge terminated.")

        # Speak any remaining tokens
        if tokens:
            sentence = ' '.join(tokens)
            speak(sentence)


def main():
    if '--pipe' in sys.argv:
        pipe_mode()
    else:
        interactive_mode()


if __name__ == '__main__':
    main()
