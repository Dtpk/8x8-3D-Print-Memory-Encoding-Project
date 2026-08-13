#include <Adafruit_TinyUSB.h>
#include <ctype.h>

Adafruit_USBH_Host USBHost;
Adafruit_USBH_CDC PrinterUSB;

const int LED_PIN = 25;
const int RX_PIN = 5;   // GBA Pin 3 (SI) -> Pico GP5 (RX)
const int TX_PIN = 4;   // Pico GP4 (TX) -> GBA Pin 5 (SO)

char gba_buf[256];
size_t gba_buf_pos = 0;

char printer_buf[256];
size_t printer_buf_pos = 0;

bool waiting_for_marlin_ok = false;
uint32_t activity_timer = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW);  delay(100);
  }

  pinMode(RX_PIN, INPUT_PULLUP);
  Serial2.setTX(TX_PIN);
  Serial2.setRX(RX_PIN);
  Serial2.setInvertRX(false);
  Serial2.begin(1200); 

  if (!USBHost.begin(0)) {
    while (1) {
      digitalWrite(LED_PIN, HIGH); delay(50);
      digitalWrite(LED_PIN, LOW);  delay(50);
    }
  }

  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  USBHost.task();

  if (PrinterUSB.mounted()) {

    // -------------------------------------------------------------
    // 1. INGEST: GBA (Serial2) -> Forward Direct to Printer
    // -------------------------------------------------------------
    while (Serial2.available()) {
      char c = (char)Serial2.read();
      activity_timer = millis();

      if (c == '\r' || c == '\n') {
        if (gba_buf_pos > 0) {
          gba_buf[gba_buf_pos] = '\0';
          
          // Send line straight to printer
          PrinterUSB.println(gba_buf);
          PrinterUSB.flush();
          
          waiting_for_marlin_ok = true;
          gba_buf_pos = 0;
        }
      } else if (gba_buf_pos < sizeof(gba_buf) - 1) {
        gba_buf[gba_buf_pos++] = c;
      }
    }

    // -------------------------------------------------------------
    // 2. READ PRINTER: Catch 'ok' and ACK back to GBA
    // -------------------------------------------------------------
    while (PrinterUSB.available()) {
      uint8_t c = PrinterUSB.read(); 
      activity_timer = millis();

      if (c == '\r' || c == '\n') {
        if (printer_buf_pos > 0) {
          printer_buf[printer_buf_pos] = '\0';

          for (size_t i = 0; i < printer_buf_pos; i++) {
            printer_buf[i] = tolower((unsigned char)printer_buf[i]);
          }

          // Strict check for Marlin line start 'ok'
          if (strncmp(printer_buf, "ok", 2) == 0 && 
             (printer_buf[2] == '\0' || printer_buf[2] == ' ' || printer_buf[2] == '\t')) {
            
            if (waiting_for_marlin_ok) {
              waiting_for_marlin_ok = false;
              
              // Direct 1:1 ACK back to GBA
              Serial2.write('.');
            }
          }
          printer_buf_pos = 0;
        }
      } else {
        if (printer_buf_pos < sizeof(printer_buf) - 1) {
          printer_buf[printer_buf_pos++] = c;
        }
      }
    }

    // LED Activity Flicker
    if (millis() - activity_timer < 50) {
      digitalWrite(LED_PIN, (millis() / 25) % 2);
    } else {
      digitalWrite(LED_PIN, HIGH);
    }

  } else {
    // Unconnected LED heartbeat
    static uint32_t last_heartbeat = 0;
    static bool led_state = false;
    if (millis() - last_heartbeat > 1000) {
      last_heartbeat = millis();
      led_state = !led_state;
      digitalWrite(LED_PIN, led_state);
    }
  }
}

void tuh_cdc_mount_cb(uint8_t idx) {
  PrinterUSB.mount(idx);
  PrinterUSB.begin(115200);
  PrinterUSB.setDtrRts(true, true);
  waiting_for_marlin_ok = false;
  printer_buf_pos = 0;
}

void tuh_cdc_umount_cb(uint8_t idx) {
  PrinterUSB.umount(idx);
  waiting_for_marlin_ok = false;
}
