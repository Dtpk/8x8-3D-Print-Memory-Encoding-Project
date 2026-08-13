/*
 * GBA-to-Serial Bridge (Robust Line-Buffered Version)
 * -------------------------------------------------
 * Bridges GBA (1200 baud) to USB Serial (115200 baud).
 */

const int LED_PIN = 25; 
const int RX_PIN = 5;

// CHANGE THIS TO 'true' IF YOUR SIGNAL IS IDLE-LOW / INVERTED
const bool INVERT_SIGNAL = false; 

#define BUF_SIZE 256
char rx_buf[BUF_SIZE];
size_t buf_idx = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(RX_PIN, INVERT_SIGNAL ? INPUT_PULLDOWN : INPUT_PULLUP);

  // Configure Hardware UART2 on RP2040
  Serial2.setTX(4);
  Serial2.setRX(RX_PIN);
  Serial2.setInvertRX(INVERT_SIGNAL); 
  Serial2.begin(1200); 

  // PC USB Serial
  Serial.begin(115200);

  // Confirmation LED flashes
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW);  delay(100);
  }
  
  Serial.print("\n--- GBA BRIDGE ACTIVE (1200 BAUD / ");
  Serial.print(INVERT_SIGNAL ? "INVERTED" : "STANDARD");
  Serial.println(") ---");
}

void loop() {
  static uint32_t last_heartbeat = 0;
  static bool led_state = false;
  static uint32_t activity_timer = 0;

  // --- BRIDGE: Read from GBA & Line Buffer ---
  while (Serial2.available() > 0) {
    char c = (char)Serial2.read();
    activity_timer = millis();

    // Store character in buffer
    if (buf_idx < BUF_SIZE - 1) {
      rx_buf[buf_idx++] = c;
    }

    // Flush buffer to PC on Newline ('\n') or if buffer is full
    if (c == '\n' || buf_idx >= BUF_SIZE - 1) {
      rx_buf[buf_idx] = '\0'; // Null-terminate string
      Serial.print(rx_buf);   // Print entire line at once
      buf_idx = 0;            // Reset buffer
    }
  }

  // Timeout Flush: If line ends without '\n' and line sits idle > 100ms
  if (buf_idx > 0 && (millis() - activity_timer > 100)) {
    rx_buf[buf_idx] = '\0';
    Serial.print(rx_buf);
    buf_idx = 0;
  }

  // --- HEARTBEAT / ACTIVITY LED ---
  if (millis() - activity_timer > 50) {
    if (millis() - last_heartbeat > 1000) {
      last_heartbeat = millis();
      led_state = !led_state;
      digitalWrite(LED_PIN, led_state);
    }
  } else {
    digitalWrite(LED_PIN, (millis() / 25) % 2); // Rapid flash during activity
  }
}
