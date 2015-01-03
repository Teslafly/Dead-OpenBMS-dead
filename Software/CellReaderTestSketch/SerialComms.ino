

void parseComms () {
  // print the string when a newline arrives:
  if (SerialComplete) {
    // Serial.println(SerialBuffer); // debugging recieved commands.
   
   // parsing goes here
    
    // clear the string:
    SerialBuffer = "";
    SerialComplete = false;
  }
}

// get serial when new bytes arrive.
void serialEvent() {
  while (Serial.available()) {
    static byte CharCount; // count to store charecter count so command buffer doesn't overflow
    // get the new byte:
    char inChar = (char)Serial.read();

    
    // add it to the inputString:
    if (CharCount <= SERIALBUFF && SerialComplete == false)  
    SerialBuffer += inChar;
    else                            
    Serial.print("Cmd to large");
    
    CharCount++; // increment the count of charecters in buffer
    
    // set a flag if the serial command is "complete"
    if (inChar == '\n') {
      SerialComplete = true;
      CharCount = 0;
    } 
  }
}
