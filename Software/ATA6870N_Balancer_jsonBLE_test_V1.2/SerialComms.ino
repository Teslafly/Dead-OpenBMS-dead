/*
communication portion of sketch. bluetooth & serial connection switching, parsing and setup all takes place here.
*/
// goals
// switch between ble and serial comms depending on what's connected.
//automatically route commands / json


// a little bit of config stuff.
#define SERIALBUFF 10 // buffer size for serial commands. .


Adafruit_BLE_UART BTLEserial = Adafruit_BLE_UART(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);


// functions
void BeginComms(int baudRate)// start ble and serial stuff
{
  SerialBuffer.reserve(SERIALBUFF);  // reserve bytes for incoming communication
  
  Serial.begin(baudRate);
  while(!Serial);
 
 #ifdef BLE_ENABLED
    BTLEserial.setDeviceName("OSBMSv1"); // name of bluetooth device /* 7 characters max! */
    BTLEserial.begin(); // reminder that bluetooth link can only transfer 30 bytes each read / write..
    delay(10);
    parseComms ();
 #endif
}


void parseComms () { // should be called as often as possible
  
 #ifdef BLE_ENABLED
 // Tell the nRF8001 to do whatever it should be working on.
  BTLEserial.pollACI(); // poll as often as possible.
  
  // eventually slim this down.
  // what's our current status?
  aci_evt_opcode_t status = BTLEserial.getState();
  // If the status changed....
  if (status != laststatus) {
    // print it out!
    if (status == ACI_EVT_DEVICE_STARTED) {
        Serial.println(F("* Advertising started"));
    }
    if (status == ACI_EVT_CONNECTED) {
        Serial.println(F("* Connected!"));
    }
    if (status == ACI_EVT_DISCONNECTED) {
        Serial.println(F("* Disconnected or timed out"));
    }
    // OK set the last status change to this one
    laststatus = status;
  }
  
  
  if (status == ACI_EVT_CONNECTED) {
    // see if there's any data for us
    if (BTLEserial.available()) {
      Serial.print("* "); 
      Serial.print(BTLEserial.available()); 
      Serial.println(F(" BLE bytes available"));
    }
    // OK while we still have something to read, get a character and print it out
    while (BTLEserial.available()) {
      char c = BTLEserial.read();
      Serial.print(c);
    }

    // Next up, see if we have any data to get from the Serial console


    if(0)
    { // (Serial.available()) 
      // Read a line from Serial
      Serial.setTimeout(100); // 100 millisecond timeout
      String s = Serial.readString();

      // We need to convert the line to bytes, no more than 20 at this time
      uint8_t sendbuffer[20];
      s.getBytes(sendbuffer, 20);
      char sendbuffersize = min(20, s.length());

      Serial.print(F("\n* Sending -> \"")); Serial.print((char *)sendbuffer); Serial.println("\"");

      // write the data
      BTLEserial.write(sendbuffer, sendbuffersize);
    }
  }
  
 
 #endif
 
 
 
 
  // print the string when a newline arrives:
  if (SerialComplete) {
    // Serial.println(SerialBuffer); // debugging recieved commands.
   
   // parsing goes here what data structure will be used? 
    
    // clear the string:
    SerialBuffer = "";
    SerialComplete = false;
  }
}


void serialSemd(String msg)
{
      parseComms ();
      
      
      // We need to convert the line to bytes, no more than 20 at this time
      uint8_t sendbuffer[20];
      msg.getBytes(sendbuffer, 20);
      char sendbuffersize = min(20, msg.length());

      Serial.print(F("\n* Sending -> \"")); Serial.print((char *)sendbuffer); Serial.println("\"");

      // write the data
      BTLEserial.write(sendbuffer, sendbuffersize); 
}


/*
// get serial when new bytes arrive.
void serialEvent() {
  //while (Serial.available()) 
  if(0)
  {
    static byte CharCount; // count to store character count so command buffer doesn't overflow
    // get the new byte:
    char inChar = (char)Serial.read();

    
    // add it to the inputString:
    if (CharCount <= SERIALBUFF && SerialComplete == false)  
    SerialBuffer += inChar;
    else                            
    Serial.print("BigCmd");
    
    CharCount++; // increment the count of charecters in buffer
    
    // set a flag if the serial command is "complete"
    if (inChar == '\n') {
      SerialComplete = true;
      CharCount = 0;
    } 
  }
}
*/
