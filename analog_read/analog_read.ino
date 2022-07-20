void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  
  //set the resolution to 12 bits (0-4096)
  analogReadResolution(10);
}

void loop() {
  // read the analog / millivolts value for pin 2:
  int analogValue0 = analogRead(2);
  int analogValue1 = analogRead(15);
  
  // print out the values you read:
  Serial.printf("Soil Moisture Value = %d\n",analogValue0);
  Serial.printf("Smoke Value = %d\n",analogValue1);

  
  delay(1000);  // delay in between reads for clear read from serial
}
