// Standalone arithmetic regression test; no sensor or display needed.
// Uploading this sketch replaces the application. Re-upload main afterwards.
#include "../../main/rpm_calculator.cpp"

static unsigned failures = 0;
static void check(bool ok, const char* label) {
  Serial.printf("%s: %s\n", ok ? "PASS" : "FAIL", label);
  if (!ok) ++failures;
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  RpmCalculator rpm;
  rpm.reset();
  for (uint32_t t = 0; t <= 100000; t += 10000) rpm.acceptRisingEdge(t);
  check(rpm.peakRpm() == 6000 && rpm.averageRpm() == 6000 &&
        rpm.validPeriodCount() == 10, "steady 6000, first timestamp zero");
  check(!rpm.stopped(99900) && !rpm.stopped(100000) &&
        !rpm.stopped(349999) && rpm.stopped(350000),
        "future DMA timestamp cannot prematurely stop a launch");

  rpm.reset();
  uint32_t t = 1000;
  rpm.acceptRisingEdge(t);
  const uint32_t acceleration[] = {30000, 25000, 15000, 10000,
                                  6000, 6000, 6000, 6000, 6000};
  for (uint32_t period : acceleration) rpm.acceptRisingEdge(t += period);
  check(rpm.currentRpm() == 10000 && rpm.peakRpm() == 10000 &&
        rpm.rejectedPeriodCount() == 0, "acceleration does not lock filter");

  rpm.reset();
  t = 1000;
  rpm.acceptRisingEdge(t);
  const uint32_t isolatedSpike[] = {10000, 10000, 2500, 10000, 10000};
  for (uint32_t period : isolatedSpike) rpm.acceptRisingEdge(t += period);
  check(rpm.peakRpm() == 6000, "top-three median rejects one fast outlier");

  rpm.reset();
  rpm.acceptRisingEdge(1000);
  rpm.acceptRisingEdge(11000);
  rpm.acceptRisingEdge(12000); // Impossible early edge, must not shift origin.
  rpm.acceptRisingEdge(21000);
  check(rpm.validPeriodCount() == 2 && rpm.averageRpm() == 6000 &&
        rpm.lastEdgeUs() == 21000 && rpm.rejectedTooFastCount() == 1,
        "early glitch preserves next complete period");
  check(rpm.validLaunch(20), "two-period short pull remains valid");

  rpm.reset();
  rpm.acceptRisingEdge(UINT32_MAX - 19999);
  rpm.acceptRisingEdge(UINT32_MAX - 9999);
  rpm.acceptRisingEdge(0);
  check(rpm.averageRpm() == 6000 && rpm.stopped(Config::STOP_TIMEOUT_US),
        "micros rollover and stop when last edge is zero");

  rpm.reset();
  rpm.acceptRisingEdge(1000);
  rpm.acceptRisingEdge(11000);
  rpm.reset(); // Same operation used on an ADC discontinuity.
  rpm.acceptRisingEdge(900000);
  rpm.acceptRisingEdge(910000);
  check(!rpm.hasStarted() && rpm.validPeriodCount() == 0,
        "no period is counted across stream reset");

  rpm.reset();
  t = 1000;
  rpm.acceptRisingEdge(t);
  const uint32_t missingEdge[] = {10000, 10000, 20000, 10000, 10000};
  for (uint32_t period : missingEdge) rpm.acceptRisingEdge(t += period);
  check(rpm.peakRpm() == 6000 && rpm.averageRpm() == 5000,
        "missing edge does not invent a revolution");
  Serial.printf("DONE: %u failures\n", failures);
}

void loop() {}
