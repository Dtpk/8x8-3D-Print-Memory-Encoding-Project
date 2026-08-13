/*
 * GBA Baud Rate Scanner & Diagnostic Bridge
 * -----------------------------------------
 * Measures the pulse width of the GBA signal to find the true baud rate.
 */

const int RX_PIN = 5;
const int LED_PIN = 25;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT_PULLUP);
  
  Serial.begin(115200);
  while(!Serial); // Wait for PC connection
  
  Serial.println("\n--- GBA BAUD RATE SCANNER ACTIVE ---");
  Serial.println("Waiting for GBA signal... (Press a button on GBA)");
}

void loop() {
  // 1. Measure the width of the first LOW pulse (Start Bit)
  // pulseIn returns duration in microseconds
  uint32_t duration = pulseIn(RX_PIN, LOW, 1000000); // 1s timeout
  
  if (duration > 0) {
    // A standard UART bit is 1/Baud. 
    // Duration is in microseconds, so Baud = 1,000,000 / duration.
    float detected_baud = 1000000.0 / (float)duration;
    
    Serial.print("\n[!] SIGNAL DETECTED!");
    Serial.print(" Pulse Width: "); Serial.print(duration); Serial.print("us");
    Serial.print(" -> Estimated Baud: "); Serial.println(detected_baud);
    
    // 2. Try to read a few bytes at this new baud rate
    // We'll use a rounded integer baud
    int baud = (int)detected_baud;
    if (baud < 300) baud = 300;
    
    Serial2.begin(baud);
    Serial.print("Starting bridge at "); Serial.print(baud); Serial.println(" baud...");
    
    uint32_t start_capture = millis();
    while (millis() - start_capture < 2000) { // Capture for 2 seconds
      if (Serial2.available()) {
        uint8_t c = Serial2.read();
        Serial.print("[");
        if (c < 16) Serial.print("0");
        Serial.print(c, HEX);
        Serial.print("]");
        if (c >= 32 && c <= 126) Serial.print((char)c);
      }
    }
    
    Serial2.end();
    Serial.println("\n--- Capture Finished. Waiting for next signal... ---");
  }
  
  // Heartbeat
  static uint32_t last_hb = 0;
  if (millis() - last_hb > 500) {
    last_hb = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}
