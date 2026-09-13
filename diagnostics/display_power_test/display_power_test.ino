// Diagnostic test: keep the e-paper physically connected.
// This sketch deliberately does not initialize SPI or touch any display pin.

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("POWER TEST STARTED");
}

void loop() {
  static uint32_t counter = 0;
  Serial.printf("alive %lu, 3V3 must remain stable\n",
                static_cast<unsigned long>(counter++));
  delay(1000);
}
