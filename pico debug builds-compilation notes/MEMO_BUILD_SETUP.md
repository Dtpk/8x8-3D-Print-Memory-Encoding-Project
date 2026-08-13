# Build Environment & Configuration Memo: Pico USB Host Mode

This document records the specific environment setup and library modifications required to successfully compile the GBA-Pico-Printer bridge in **Native USB Host Mode**.

## 1. Toolchain Setup
- **CLI**: `arduino-cli` (v1.5.1 used).
- **Core**: `rp2040:rp2040` (v6.0.0, Earle Philhower core).
- **Required Library**: `Adafruit TinyUSB Library` (v3.7.7).

## 2. Critical Library Modification
By default, the Adafruit TinyUSB library defaults to Device mode or requires an external MAX3421 chip. To force **Native USB Host Mode** on the Pico's micro-USB port, the following file must be edited:

**File Path:** `~/Arduino/libraries/Adafruit_TinyUSB_Library/src/arduino/ports/rp2040/tusb_config_rp2040.h`

**Changes applied:**
```cpp
// Force Native Host mode and disable Device mode stubs
#define CFG_TUD_ENABLED 0
#define CFG_TUH_ENABLED 1
#define CFG_TUH_RPI_PIO_USB 0
#define CFG_TUH_MAX3421 0
```

## 3. Compilation Command
To ensure the TinyUSB stack is correctly linked and that no stale "Device Mode" cache persists, use the following command structure:

```bash
arduino-cli compile \
  --clean \
  --fqbn rp2040:rp2040:rpipico:usbstack=tinyusb \
  --build-property "compiler.cpp.extra_flags=-DUSE_TINYUSB_HOST -DCFG_TUH_ENABLED=1 -DCFG_TUH_CDC=1" \
  [SKETCH_PATH]
```

### Key Flags:
- `--clean`: Essential to clear the `core.a` cache which might contain Device-mode stubs.
- `-DUSE_TINYUSB_HOST`: Tells the library to activate host-specific code paths.
- `-DCFG_TUH_CDC=1`: Ensures the Communication Device Class (CDC) Host driver is included for printer communication.

## 4. Troubleshooting Signals
- **50ms Strobe Loop**: Indicates `USBHost.begin(0)` failed. Usually means the library was compiled in Device mode or is looking for a MAX3421 chip.
- **1s Solid Pulse**: Indicates successful Host stack initialization. The Pico is now searching for the USB device.
- **Rapid 25ms Toggle**: Indicates active data transfer between GBA and Printer.

## 5. Buffer & Command Limits (Updated)
- **CMD_LEN**: 256 bytes (Supports long coordinates and complex purge moves).
- **QUEUE_SIZE**: 256 slots (Optimized to fit within RP2040 RAM while maintaining safety).
- **Printer Buffer**: 256 bytes with sliding window overflow protection.
