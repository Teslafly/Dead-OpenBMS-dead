

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void Serial etup() {
  // reserve 200 bytes for the inputString:
  inputString.reserve(100);
}

void parseSerail {
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.println(inputString);
   
   // parsing goes here
    
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }
}
