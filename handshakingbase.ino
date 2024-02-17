void setup() {
  Serial.begin(9600);
  // Wait a bit for the serial port to establish connection with the PC
  delay(2000);
}

void loop() {
  Serial.println("HANDSHAKE");  // Send the handshake message
  while(Serial.available() == 0) {
    ; // Do nothing until we get a response
  }
  String response = Serial.readStringUntil('\n');
  if(response.startsWith("ACK")) {
  }
  delay(5000);
}
