/*
where all the magic happens with the ATA6870N. 
this file handles all of the communicationn, parsing, and management of the ATA6870N BMS IC.
*/
// spi communication specifics
// ata68 communicates msbf (most significant byte first)
// spi speed is set to lowest speed. (for now) For reliability and reducing potential problems resulting from high clock speeds while testing.
// chip is selected when cs line is held LOW.
// clock specifics are: 
// CPOL = 0
// CPHA = 1
// spi mode = 0


// potential gotchya's
/*
For internal synchronization, it is mandatory to keep CLK running during any SPI access; CLK must be set on 4 clock cycles
(at least) before SPI access starts, and must be kept on 4 clock
cycles (at least) after SPI access ends up. Keeping at least
4 CLK clock cycles between two consecutive SPI accesses is mandatory. If this is not the
case, the Atmel ATA6870Ns will detect an error in communication. The CommError bit will be set in the status register [0x06])


// check this in the schematic!
The test-mode pins DTST, ATST, PWTST (outputs) have to be kept open in the application. The test-mode pins
SCANMODE and CS_FUSE (inputs) have to be connected to VSSA. These inputs have an internal
pull-down resistor. The test-mode pin VDDFUSE is a supply pin. It must also be connected to VSSA


Error codes // not quite working as of yet.
  0 == all is good. system working as expected. (does not print error code on serial)
  1 == Communication error. check your wiring and master/slave jumpers.
  2 == main power undervoltage. please check power supply.
  3 == wrong number of devices on bus. please program with correct number of devices. or ensure correct wiring.
  4 ==
  5 == 
  6 ==
  7 == 
  8 == 
  9 ==
*/

// defines to make it easier to get the transfer direction right.
#define SEND 1
#define RECIEVE 0

// set up the SPI speed, mode and endianness for ATA6870N
SPISettings ATA68_SPIconfig(SPI_CLOCK_DIV128, MSBFIRST, SPI_MODE0); 

const byte expectedRevId = 0X02; // expected revision id to look for.

// ata6870n register mapping.   The datasheet is quite useful for this: http://www.atmel.com/Images/Atmel-9317-Li-Ion-Battery-Management-ATA6870N_Datasheet.pdf
const byte RevID = 0X00;             // Revision ID/value Mfirst, pow_on [-R 8bit]
const byte Ctrl = 0X01;              // control register [-RW 8bit]
const byte OpReq = 0X02;             // operation request [-RW 8bit]
const byte Opstatus = 0X03;          // operation status [-R 8bit]
const byte Rstr = 0X04;              // software reset [-W 8bit]
const byte IrqMask = 0X05;           // mask interrupt sources [-RW 8bit]
const byte statusReg = 0X06;         // status interrupt sources [-R 8bit]
const byte ChannelUdvStatus = 0X08;  // channel undervoltage status [-R 8bit]
const byte ChannelDischSel = 0X09;   // select channels to discharge [-RW 8bit]
const byte ChannelReadSel = 0X0A;    // select channel to read [-RW 8bit]
const byte LFTimer = 0X0B;           // Low frequency timer control [-RW 8bit}
const byte UdvThreshold = 0X10;      // undervoltage detection threshold [-RW 16 bit]
const byte DataRd16 = 0X11;          // single access to selected channel value (In channelReadSel register) [-R 16 bit]
const byte DataRd16Burst = 0X7F;     // burst access to all channels (6 voltage, 1 temperature) -R 112bit


// default register values.
const byte IrqMask_initVal = B00010110; // LFTdoneMask, chkdata, chksum are enabled. other masks are disabled and will cause interrupt.

#ifdef CHECKSUM_ENABLED
const byte Ctrl_initVal    = B00010000; // [Chksum_ena: on] [LFTimer_ena: off] [TFMODE_ena: off]
#else
const byte Ctrl_initVal    = B00000000; // [Chksum_ena: off] [LFTimer_ena: off] [TFMODE_ena: off]
#endif
       
 // status register bits.
 //bit 0 = dataRdy    -> Conversion finished
 //bit 1 = LFTdone    -> Low frequency timer elapsed
 //bit 2 = commError  -> Bad SPI command detected (wrong length)
 //bit 3 = udv        -> Undervoltage detected
 //bit 4 = chkError   -> Error on checksum check
 //bit 5 = Por        -> Power on reset detected
 //bit 6 = TFMdeOn    -> Test mode on
 //bit 7 = n/a



//////////////////////////////////////////////////////////////////////////////////////////
// higher level functions that achieve base functionality.
//////////////////////////////////////////////////////////////////////////////////////////
void ATA68_initialize(uint16_t expectedBoardCount) // set important spi settings and set control register for string.
{
  // set up pins
  pinMode(ATA_CS, OUTPUT);
  pinMode(IRQ_PIN, INPUT);
  digitalWrite(ATA_CS, HIGH); // end spi transfer by deselecting chip
  //attachInterrupt( IRQ_INT, ATA68_IRQroutine, RISING); // attach function to interrupt. not needed at this point
 
  ATA68_GenClk(1); // start 500khz clock for ATA6870N on pin 9
 
  SPI.begin(); 
  
  for(int deviceNum = 0; deviceNum < (BALANCERCOUNT); deviceNum++) // iterate through boards and set communication critical registers.
  {
     byte ctrlData = Ctrl_initVal;
     byte irqMaskData = IrqMask_initVal;
      
     ATA68_Transfer(deviceNum, Ctrl, &ctrlData, SEND, 1); //set control registers of entire string.
     ATA68_Transfer(deviceNum, IrqMask, &irqMaskData , SEND, 1); //set irqmask of entire string.
  }
  // should set the undervoltage threshold here too.
}




////////////////////////////////////////////////////////////////////////
uint8_t ATA68_StartupInfo(boolean sendInfo) // reads chip id, scans the bus, gets useful chip settings, and checks if everything is ok. outputs error code if not. - this is still has some check issues.
{
  uint8_t errorCode = 0; // store any generated error codes.
    
  for(uint8_t deviceNum = 0; deviceNum < 16; deviceNum++)// get chip id's of all chips on bus. scan whole 16 possible chips.
  {
  
    uint8_t RevID_Data = 0XFF;
    ATA68_Transfer(deviceNum,  RevID, &RevID_Data, RECIEVE, 1);
    
    if(sendInfo == 1) // send useful data about chips
      { 
        Serial.print(F("IC#"));
        Serial.print(deviceNum);
        Serial.print(F(" - "));
        Serial.print(RevID_Data, BIN);
      }
    
    if(deviceNum < BALANCERCOUNT)// check to make sure ic should be present.
    {
      if((((RevID_Data & B00001000) == B00001000) & (deviceNum == 0)) | (((RevID_Data & B00001000) != 0) & (deviceNum != 0)))  // check that only the first chip in the bus has the master bit set
      {
        errorCode = 1;
        Serial.print(F(" BadJumperSetting "));
      }
      else if((RevID_Data & B00000111) != expectedRevId)
      {
        errorCode = 2;
        Serial.print(F(" BadRevId "));
      }
      else if((RevID_Data & B00010000) == 0)  // check hardware inputs for right configuration.
      {
        errorCode = 3;
        Serial.print(F(" BadPowEn "));
      }
    } 
    else if(RevID_Data != 0)// check if an ic not defined reports back. default response for unconnected ic's is 0X00
    {
      Serial.print(F("bad balancer count"));
      errorCode = 4;
    }
      
    Serial.println();  
      
  }// end of for loop
  
  // always outputs at least 0. This is normal and means everything checked out. if this is not 0 you need to check your configuration.
  return errorCode;
}


 
////////////////////////////////////////////////////////////////////////////////////////
uint8_t ATA68_bulkRead(uint16_t *cellAdcData, uint16_t *tempAdcData, byte icCount, boolean voltMode, boolean tempBit) // reads all cell voltages and selected temperature senors into appropriate arrays.
{ // useage
  // volt mode = calibration or regular aquisition. 0 for calibration, 1 for regular.
  // tempBit = temperature sensor to select. 1 for internal, 0 for external.
  // Iccount = number of ic's in the string to iterate through.
  // returns error code. 0 = ok. 1-4 = not ok.
  
  uint8_t bulkReadError = 0;
 
 // start conversion by clearing out any exixting comands and issuing our own.
  for(byte deviceNum = 0; deviceNum < icCount; deviceNum++) // iterate through connected boards.
  {
    //Serial.print(F("reading device: "));
    //Serial.println(deviceNum);
    
    ATA68_getStatus(deviceNum); // clear any existing interrupts
    
    if (ATA68_GetOpStatus(deviceNum) != 0) // check and clear exixting commands
    {
       Serial.println("ATA6870 busy, clearing");
      
       byte OpClear = 0X00;
       ATA68_Transfer(deviceNum, OpReq, &OpClear, SEND, 1); //clear existing command
      
       delay(5);
      
       ATA68_getStatus(deviceNum);
       ATA68_GetOpStatus(deviceNum);
    }
    
    uint8_t OpReqData = (B00000001 | (voltMode << 1) | (tempBit << 3));
    //Serial.print("opreqdata: ");
    //Serial.println(OpReqData, BIN);
    ATA68_Transfer(deviceNum, OpReq, &OpReqData, SEND, 1); // start conversion  
  }
  
  
  
  
  
  // scan the bus for a dataready interrupt and read data into adc value arrays.
  for(byte deviceNum = 0; deviceNum < icCount; deviceNum++) // iterate through connected boards.
  {
    
    boolean nodata = 1;
    int exitCount = 0; // counts loops and exits when it gets stuck
    while (nodata) // check if dataready bit is set. should be replaced with an interrupt based dataready check.
    { 
          
      delay(10); //allow the chip to collect data. this value is not fine tuned and is only meant to allow this function to work.
      // 8.2ms conversion time according to datasheet.
      // should actually wait for interrupt in a future version of code. a delay like this could cause problems
    
      if(digitalRead(IRQ_PIN) == HIGH)
      { 
        nodata = 0;   
      }
      else if(ATA68_GetOpStatus(deviceNum) == 2) // optionally poll for data instead of looking for interrupt.
      {
        nodata = 0;
      } 

      
      if(exitCount > 20) // check if loop is stuck
      { 
        bulkReadError = 4; // exits with error code 4.
        nodata = 0; // attempt to read data anayway. (status bits will cancel read if the chip really isn't ready)
      }
      
      exitCount++;
    }//end while(nodata)
    
   
    if (ATA68_getStatus(deviceNum) & 0x01) // if dataready bit is set, read data.
    {
      byte BurstBuffer[14]; 
      for (int i; i <= 13; i++) 
      { BurstBuffer[i] = 0XFF; } // clear buffer
    
      ATA68_Transfer(deviceNum, DataRd16Burst, &BurstBuffer[0], RECIEVE, 14);
    
      if ((ATA68_getStatus(deviceNum) & B00010100) < 1) // Check that communication didn't cause a commError or chkError flag
      {
        for (int c = 0; c < 6; c++)
        {
         cellAdcData[(5 - c) + (deviceNum * 6)] = (BurstBuffer[c * 2] << 8) | BurstBuffer[(c * 2) + 1]; // combine two adc bytes into uint16_t and store to adc result array.
        }
       
        tempAdcData[deviceNum] =  (BurstBuffer[12] << 8) | BurstBuffer[12 + 1]; // convert and store temperature adc result.
       
         ATA68_GetOpStatus(deviceNum);
      } // end if-comm Ok ckeck
      else 
      { 
        bulkReadError = 2; 
        //Serial.println("invalid data");
      }
          
    }// end if-dataready 
    else
    {
      //Serial.println("data not ready"); 
      ATA68_GetOpStatus(deviceNum);
    } 
  
  }// end "for(byte deviceNum = 0;"
      
  return bulkReadError;
}// end bulkread function


/////////////////////////////
// utility functions
/////////////////////////////
void ATA68_SetUdvTrip(int LowVoltage) // set the low voltage trip point in the ATA6870N
{ // useage
  // binary value of low voltage trip point.
  
  uint8_t UdvThreshData[2] = {(LowVoltage & 0xff), (LowVoltage >> 8)} ; // turn the 16 bit int into 2 bytes.

  for(int deviceNum; deviceNum < BALANCERCOUNT; deviceNum++) // iterate through all modules in the string.
  {
    ATA68_Transfer(deviceNum, UdvThreshold, &UdvThreshData[0], SEND, 2);
  }
}


/////////////////////////////////////
// function to turn on the balance resistors
/////////////////////////////////////
void ATA68_ResistorControl(uint8_t deviceNum, uint8_t cells){
  // device = device number. device 0-15. 
  // cells = cells to drain. this is 8 bits with the lsb being cell 1 and bit 6 being cell 6. 1 for load on, 0 for load off)
  
  if (cells <= 63) // check that upper 2 bits are 0. will rework this function with bitfeild in the future.
  {
    ATA68_Transfer(deviceNum, ChannelDischSel, &cells, SEND, 1); 
  }
}

//////////////////////////////////////////////////
byte ATA68_GetOpStatus(uint8_t deviceNum)// get operation status
{ //useage
  // output:
  // 0 = no operation
  // 1 = ongoing operation
  // 2 = operation finished
  // 3 = operation failed/was cancled. result not available.
  
  uint8_t OpStatusData = 0XFF;
  ATA68_Transfer(deviceNum, Opstatus, &OpStatusData, RECIEVE, 1); 
  
  // debugging stuff
  //Serial.print(" OPstatus = ");
  //Serial.println(OpStatusData, HEX);
  
  return OpStatusData;
}



byte ATA68_getStatus(uint8_t deviceNum) // read the status register
{
  uint8_t statusRegData = 0XFF;
  ATA68_Transfer(deviceNum, statusReg, &statusRegData, RECIEVE, 1);
  
  //Serial.print("Status = ");
  //Serial.println(statusRegData, BIN);
  
  return statusRegData;
}


//////////////////////////////////////////////////////////////////////////
// base hardware call functions
//////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////
// order of commands to ata68. data is sent from top to bottom.
//[chipId - returns irq. 16 bits]
//[control - returns nothing. 8 bits]
//[data - returns data if being read. returns nothing if being written. 8-112bits read. 8-16 bits write.]
//[checksum. optional. always sent by microcontroller when active]


void ATA68_IRQroutine() // attached to irq pin via interrupt and runs when pin triggers. Still figuring out what I want to do with this.
{ 
Serial.print(F("irqTrigger"));

}



///////////////////////////////////////////////////////////
// function for transferring data to and from the ATA6870N
void ATA68_Transfer (uint8_t deviceNum, uint8_t regAddress, uint8_t *SPIbuffer, boolean RW, uint8_t length )
{
  // deviceNum = device number. Must be between 0-15 otherwise no device wll be selected.
  // RegAddress = adress of the register we want to write/read data from. The last bit is the read/write control. (0 for read)
  // recievedLength = If I am recieving data how many bytes should I recieve? Also automatically adds padding to push bits out.
  // *SPIbuffer = pointer to byte array to hold transmitted / recieved data. Should be cleared before reads.
  // RW = read / write bit. 
  // length = length in bytes of returned / transmitted data.
  
  regAddress = (regAddress << 1)| RW; // shift register address one over to make room for read/write bit 

  SPI.beginTransaction(ATA68_SPIconfig); 
  SPI.transfer(0XFF);//pulse out 4+ clock ticks before communication
  digitalWrite(ATA_CS, LOW);  // start spi transfer by selecting chip

    ATA68_Select(deviceNum);
    
    SPI.transfer(regAddress); //select register and set r/w bit 

    if(length != 0) // check that length is valid
    {
      for (int i = 0; i < (length); i++) {  //recieve actual data. length = length in bytes for message
        SPIbuffer[i] = SPI.transfer(SPIbuffer[i]); // recieve/transmit data.
      }
    }//else{error = 1;}
    
    #ifdef CHECKSUM_ENABLED
    // generate and send checksum
      uint8_t checksum = 0x00; // byte to store & minipulate the lfsr output
 
      checksum = ATA68_calcLFSR(checksum, regAddress); //run calculation of register adress
    
      for(uint8_t i=0; i<length; i++)
      {
         checksum = ATA68_calcLFSR(checksum, SPIbuffer[i]); // run calculation of data byte by byte. 
      }
      
      if(RW = 1)
      {
    
      #ifdef DEBUG_INFO
        Serial.print(F("sent checksum: "));
        Serial.println(checksum, BIN);
      #endif  
     
                
      SPI.transfer(checksum);
      // check for bad comm interrupt in status register!
      
      }
      else 
      { 
        byte ATAChecksum = SPI.transfer(0X00); // read checksum from ATA6870n and check against our computed checksum.
        
        if ( ATAChecksum == checksum)
        {
         // output somthing akin to "communication good"
        }
    #endif
  
  digitalWrite(ATA_CS, HIGH); // end spi transfer by deselecting chip
  SPI.transfer(0XFF);//pulse out 4+ clock ticks after communication to "complete" the transaction.
  SPI.endTransaction();

}


//////////////////////////////////////////////////////////////////////////////////////////
void ATA68_Select (uint8_t StringAddress) // selects device in string, transfers address command and gets irq.
{ // useage
  // StringAddress = device number. device 0-15. will skip communication of data is not within this value. (all 16 bits set to "0")



  uint16_t stackAddress = 0x0001 << StringAddress; // Address of selected chip. First byte is shifted left once for each chip increment
  
  //Serial.print("stackAddress= ");
  //Serial.println(stackAddress, BIN);
  
  // select device#, recieve irq data, and add activated irq bits to the irqStore "list"
  irqStore = irqStore | (SPI.transfer(stackAddress >> 8) << 8); // transcieve first address byte
  irqStore = irqStore | SPI.transfer(stackAddress & 0xFF); // trancieve second adress byte.

}


/////////////////////////////////////////////////////////////////////////////////////////
uint8_t ATA68_calcLFSR(uint8_t Lfsr, uint8_t data) // runs LSFSR chscksum on provided data
{ // useage
  // Lfsr = starting point for calculation. pass in the result of the last calculation to string calculations together.
  // data = data that will get calculated into the checksum
  // output = calculated checksum
  
  // x^8+x^2+x+1
  // bitstream in MSBF - xor - DR - xor - DR - xor - DR - DR - DR - DR - DR - feedback to xor
  
  for (uint8_t mask = 0x01; mask>0; mask <<= 1) // iterate through bit mask until bit is pushed out the end.
      { // This function is loosely based on example code provided by wikipedia. http://en.wikipedia.org/wiki/Linear_feedback_shift_register
          boolean lsb = Lfsr & 1; // Get LSB / the output bit
          Lfsr >> 1;              // shift
          
          if (data & mask)        // if selected bit is true...
             { Lfsr = Lfsr | 0x80; } // set incoming bit to 1. bit is otherwise left at 0
           
          if (lsb == 1)           // apply toggle mask if output bit is 1. leave it alone if bit is zero
             { Lfsr ^= 0xE0; }       // Apply toggle mask: x^8+x^2+x+1 or b11100000  
      } 
  return Lfsr;  
}

///////////////////////////////////////////////////////////////////////////////////////////
void ATA68_GenClk(boolean startStop) // controls internal timers to generate a 500khz clock on pin 9
{// StartStop = start or stop 500khz clock on pin 9. 1 = start. 0 = stop.
 // please note that other pins / functions that use timer 1 will not work and may impede the correct operation of this function.
  
  
  if(startStop)
  { // start clock
    
    // Set Timer 1 CTC mode with no prescaling.  OC1A toggles on compare match
    //
    // WGM12:0 = 010: CTC Mode, toggle OC 
    // WGM2 bits 1 and 0 are in TCCR1A,
    // WGM2 bit 2 and 3 are in TCCR1B
    // COM1A0 sets OC1A (arduino pin 9 on Arduino Micro) to toggle on compare match
   #if defined(__AVR_ATmega328P__)
    TCCR1A = ( (1 << COM1A0));
    TCCR1B = ((1 << WGM12) | (1 << CS10));
    
    TIMSK1 = 0; // Make sure Compare-match register A interrupt for timer1 is disabled
    
    OCR1A = 7; // set scalar for 500khz

    pinMode(9, OUTPUT);  // start output
    #endif
  }
  else
  { // stop clock and disable timers.
    // need to actually do somthing with this. will come when power optimisation is started.
  }
}

