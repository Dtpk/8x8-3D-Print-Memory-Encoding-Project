# GBA to Pico Serial Bridge - Debug Findings

## 1. Key Discovery: GBA Baud Rate
The GBA "bit-bangs" its UART signal on Pin 3 (SI). While intended for 9600 baud, code execution from ROM results in a significant slowdown. 

**Measured Timing:**
- **Start Bit Width:** ~840 microseconds
- **Actual Baud Rate:** **1200 Baud**
- **Logic Level:** Standard UART (Idle HIGH, Start Bit LOW). NO signal inversion is required.

## 2. Wiring
- **GBA Pin 3 (SI - Black)** -> **Pico GP5 (RX)**
- **GBA Pin 6 (GND - White)** -> **Pico GND**
- **GBA Pin 4 -> **Pico gpio 4** When finally done for testing only 3 and 6 needed.

## 3. Included Tools
- `gba_serial_bridge.uf2`: Bridges GBA (1200 baud) to PC Serial (115200 baud).
- `detector.uf2`: Measures pulse width of GBA signal to confirm baud rate.

## 4. How to use Serial Bridge
1. Flash `gba_serial_bridge.uf2`.
2. Connect Pico to PC via USB.
3. Open terminal: `sudo screen /dev/ttyACM* 115200`.
4. Trigger a command on GBA (e.g., Preheat).
5. G-Code will appear in the terminal.

## 6. OTG Bridge (Production Mode)
- `2wayPico.uf2`: The final version that connects the GBA to a 3D printer you will freeze with no handshake hitting print that is normal.. If print connected link cable up and pico turned on and it does handshakes are failing.
- **Calibration:** Uses 1200 baud to match the GBA's ROM-based bit-banging speed.
- **Dual Output:** While connected to a printer, it *still* mirrors G-Code to the PC debug serial (115200 baud). This allows you to watch the G-Code while the printer is actually moving.
- **LED Indicators:**
    - Slow Blink: Searching for printer.
    - Solid ON: Printer connected.
    - Rapid Flicker: Data activity (GBA <-> Printer).
    - Rapid Flicker: When print is done means gcode lines were dropped.

## 7. Detector.uf2
1. Flash detector.uf2
2. Connect up like you would with step 4
3. Push a button and it should detect the baud rate.
