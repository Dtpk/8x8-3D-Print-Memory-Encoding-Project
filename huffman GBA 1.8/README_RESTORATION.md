# Huffman GBA Project Summary (V1.9 GOLD+)

## Project Overview
This project is a GBA ROM implementation of a Huffman Encoder/Decoder. V1.9 adds full hardware compatibility by implementing a custom header fixer.

## Key Files
- **main.c**: Core logic, hardware register definitions, UI states, and rendering loops.
- **huffman_data.h**: Huffman Tree and Codebook.
- **font_data.h**: Custom 8x8 bitmap font.
- **Makefile**: Updated to automatically fix ROM headers for real hardware.
- **gbafix.py**: Custom Python tool to inject the Nintendo logo, checksum, and 32KB padding.
- **gba.ld**: Linker script for memory mapping.
- **crt0.s**: Assembly startup code.

## Hardware Compatibility Note
Previous versions (V1.8 and below) failed on real hardware due to missing BIOS validation (Nintendo Logo and Checksum). V1.9 resolves this. Small ROMs are now padded to 32KB to ensure compatibility with all flashcarts.

## Features Implemented
- **On-Screen Keyboard**: Supports Uppercase/Lowercase toggle (L/R buttons).
- **Real-Time Bit Counter**: Displays current bit length vs. the 64-bit maximum.
- **Overflow Detection**: Visual warning when text exceeds the 8x8 grid capacity.
- **Interactive Decoder**: Navigate a grid with D-Pad and toggle bits with A/B. Shows real-time message translation.
- **Boot Animation**: "GEMINI SYSTEMS" splash screen with fade-in and audio cue.
- **Scrolling Background**: A dim grayish-blue 8x8 grid that scrolls continuously for a "tech" aesthetic.
- **Audio**: Sound 1 (Square Wave) beeps on menu movement, typing, and bit toggling.

## Build Instructions (External)
1. Install `devkitPro` (devkitARM).
2. Run `make` in the directory.
3. Use a GBA Fix tool or the Python checksum script to fix the header (0xBD).

## Restoration Note
If starting a new session, provide these files to the agent. The agent will need the `gcc-arm-none-eabi` toolchain to build the ROM again.
