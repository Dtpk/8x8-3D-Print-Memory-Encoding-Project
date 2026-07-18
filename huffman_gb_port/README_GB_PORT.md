# Huffman GB Port V1.9

A Game Boy and Game Boy Color port of the Huffman Coding utility, originally developed for the GBA. This tool allows users to encode text into a huffman-compressed bitstream and decode it back into text using an 8x8 bit grid.

## Features
- **Encode Mode:** Enter text using a virtual keyboard with real-time bit counting and overflow detection (64-bit limit).
- **Bit Grid Visualization:** View the resulting huffman codes as an 8x8 grid of 'O' and '.' characters.
- **Decode Mode:** Manually enter bits into an 8x8 grid to decode them back into a text message.
- **Dual Compatibility:** Works on original Game Boy (DMG), Game Boy Color (GBC), and Game Boy Advance (GBA) hardware.
- **Authentic Sound:** Includes beep sound effects for button presses.

## Controls

### Main Menu
- **UP/DOWN:** Select Mode
- **A:** Enter Mode

### Encode Mode
- **D-Pad:** Move cursor
- **A:** Select character / Press virtual button
- **B:** Delete last character
- **SELECT:** Toggle Uppercase/Lowercase
- **START:** View Encoded Bit Grid
- **Virtual Button [^]:** Toggle Uppercase/Lowercase
- **Virtual Button [M]:** Return to Menu

### Bit Grid View (Encode Result)
- **SELECT:** Return to Menu

### Decode Mode
- **D-Pad:** Move cursor
- **A:** Set bit to '1' (O)
- **B:** Set bit to '0' (.)
- **START:** Clear entire current row
- **SELECT:** Return to Menu

## Build Instructions

This project was built using **GBDK-2020**.

### Tools Needed
- [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020/releases) (v4.5.0 or newer recommended)
- `make` (standard build tool)

### Building the ROM
1. Ensure `lcc` (from the GBDK bin folder) is in your system path, or update the `GBDK_HOME` path in the `Makefile`.
2. Run the following command in the project directory:
   ```bash
   make
   ```
3. The resulting ROM will be named `huffman_gb.gb`.

## Credits
Ported and Enhanced by Gemini Systems (2026).
