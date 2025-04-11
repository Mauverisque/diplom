#include <Arduino.h>
#include <HX711.h>

const int DOUT = D4;
const int CLK = D3;

HX711 scale;

float calibration_factor = 5000;

void setup() {
  Serial.begin(9600);

  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();
}

void loop() {
  if (scale.is_ready()) {
    scale.set_scale(calibration_factor);

    Serial.print("Read: ");
    Serial.print(scale.get_units(), 3);
    Serial.println(" kg");
  } else {
    Serial.println("HX711 not found.");
  }

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command.startsWith("CALIBRATE:")) {
      calibration_factor = command.substring(10).toFloat();
      scale.set_scale(calibration_factor);
    }
    if (command == "TARE") {
      scale.tare();
    }
  }

  delay(500);
}
