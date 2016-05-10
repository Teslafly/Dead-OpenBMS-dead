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
  1 == Communication error. check your wiring. 
  2 == main power undervoltage. please check power supply.
  3 == wrong number of devices on bus. please program with correct number of devices. or ensure correct wiring.
  4 == bit error. chip hardware not configured / working properly. (more than one board set as master)
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

// ata6870n register mapping
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

// default register values. the datasheet is quite useful for this: http://www.atmel.com/Images/Atmel-9317-Li-Ion-Battery-Management-ATA6870N_Datasheet.pdf
const byte Ctrl_initVal    = B00010000; // [Chksum_ena: on] [LFTimer_ena: off] [TFMODE_ena: off]
const byte IrqMask_initVal = B00000010; // LFTdoneMask is enabled. other masks are disabled.
const uint16_t udvTrip_initVal = 1000;  // actually define this and get function working



typedef struct BurstDataType{ // this needs to be flippped!
  uint16_t channel6;
  uint16_t channel5;
  uint16_t channel4;
  uint16_t channel3;
  uint16_t channel2;
  uint16_t channel1;
  uint16_t temperature;
 }tBurstDataType;
 
 tBurstDataType BurstRx;
 
 
 typedef struct BooleanCellData{
   byte : 2; // 2 padding bits for alignment.
   boolean cell[5]; // data for 6 cells. zero indexed.
 }tBooleanCellData;
 
 tBooleanCellData UDVstatus[BALANCERCOUNT-1]; // variable for storing cell undervoltage status's
 tBooleanCellData DrainLoadStatus[BALANCERCOUNT-1]; // variable for storing status of cell balance resistors.
 
 
 


 // status register bits.
 //bit 0 = dataRdy    -> Conversion finished
 //bit 1 = LFTdone    -> Low frequency timer elapsed
 //bit 2 = commError  -> Bad SPI command detected (wrong length)
 //bit 3 = udv        -> Undervoltage detected
 //bit 4 = chkError   -> Error on checksum check
 //bit 5 = Por        -> Power on reset detected
 //bit 6 = TFMdeOn    -> Test mode on
 //bit 7 = n/a
 /*
 register bits
 
 
 */



// higher level functions that achieve base functionality.
void ATA68_initialize(uint16_t expectedBoardCount)
{
  pinMode(ATA_CS, OUTPUT);
  digitalWrite(ATA_CS, HIGH); // end spi transfer by deselecting chip
  SPI.begin();
  //SPI.setClockDivider(SPI_CLOCK_DIV128); // slowest clock possible, 62.5khz with an 8 mhz avr on 3.3v. chip clock is 500khz and spi clock must be at least half that.
  //SPI.setBitOrder(MSBFIRST);
  //SPI.setDataMode(3);
  
   uint8_t CtrlData = 0x00; // control register data
  
  #ifdef CHECKSUM_ENABLED
   CtrlData ^= 0x10; // set checksum enable bit
  #endif
  
  for(int i=0; i <= (BALANCERCOUNT - 1); i++)
    {
    ATA68_Transfer(i, Ctrl, &CtrlData, SEND, 1); // write the control registers of the entire string.
    }

  //attachInterrupt( IRQ_INT, ATA68_IRQroutine, RISING); // attach function to interrupt. not needed at this point
  
  // should set the undervoltage threshold here too.
}



uint8_t ATA68_StartupInfo(boolean sendInfo) // reads chip id, scans the bus, gets useful chip settings, and checks if everything is ok. outputs error code if not.
{
    uint8_t errorCode = 0; // store any generated error codes.
    
  for(uint8_t i = 0; i <= 15; i++)
  {// get chip id's of all chips on bus. scan whole 16 possible chips.
  
    uint8_t RevID_Data;
    ATA68_Transfer(i,  RevID, &RevID_Data, RECIEVE, 1);
    
    // error checking goes here.
    // check that only the first chip in the bus has the master bit set
    // check that there are the correct # of chips connected.
    // check hardware inputs for right configuration.
    
      if(sendInfo == 1) { // send useful data about chips
      Serial.print("Address#");
      Serial.print(i);
      Serial.print(" -- B");
      Serial.println(RevID_Data, BIN);
      }
    
  }// end of for loop
  
  uint16_t info[20];// id's of all the chips, settings, etc.
  
  // get the rest of the useful settings
  // parse out useful settings and store then in the array.
  
  // always outputs at least 0. This is normal and means nothing is wrong. If the output is not 0 something is definately wrong.
  return errorCode;
}






/*
uint16_t ATA68_readCell(uint8_t deviceNum, uint8_t cell) // reads individual cells / temp sensors
{ // useage
  // cell -- cell number, 0 to 5. 6 if you want to read temperature, 7 if you want to read the lft 
  // board -- board number, 0 to 15
  // outputs adc value
 
  uint16_t voltage;
 
  if((cell >=0) && (cell < 8)){ // error checking. The first 5 msb's must remain 0.
 
    uint8_t OpReqData = (OpReq_Val | 1);
 
    ATA68_Transfer(deviceNum, ChannelReadSel, &cell, SEND, 1); // select adc to read
    ATA68_Transfer(deviceNum, OpReq, &OpReqData, SEND, 1); // set operation request bit to start conversion.
    
    
    uint8_t statusRegData;
   // while (statusRegData | 0x01) // check if dataready bit is set. should be replaced with an interrupt based dataready check.
    if(1)
    {
      ATA68_Transfer(deviceNum, statusReg, &statusRegData, RECIEVE, 1); 
      
      delay(20); //allow the chip to collect data. this value is not fine tuned and is only meant to allow this function to work.
      // 8.2ms conversion time according to datasheet.
      // should actually wait for interrupt in a future version of code. a delay like this could cause problems.
    }
     
    
    uint8_t Vbuffer[1];
    ATA68_Transfer(deviceNum, DataRd16, &Vbuffer[0], RECIEVE, 2); 
  
    voltage = (Vbuffer[0] * 256) + Vbuffer[1]; // convert the 2 buffer bytes into one 16 bit voltage value.
  }else{
    voltage = 0; // if error checking fails, return somthing clearly out of range.
  }
  return voltage; 
}
*/


//////////////////////////////
// read all cell voltages
//////////////////////////////
/*void ATA68_readAllvoltages(uint16_t cellcount){
  uint16_t cellvoltages[cellcount]; // store raw cell adc values here.
  char buffer[2];
  
  for(uint8_t i = 0; i <= BALANCERCOUNT; i++){ //run once for every board
  //ATA68_READ(i, ); 
  }
}*/



uint8_t ATA68_bulkRead(uint16_t *cellAdcData, uint16_t *tempAdcData, byte icCount, boolean voltMode, boolean tempBit) // reads all cell voltages and selected temperature senors into appropriate arrays.
{ // useage
  // volt mode = calibration or regular aquisition. 0 for calibration, 1 for regular.
  // tempBit = temperature sensor to select.
  
  uint8_t bulkReadError = 0;
  
  uint8_t OpReqData = (B00000001 | (voltMode << 1) | (tempBit << 3));
  
  Serial.print("opreqdata ");
  Serial.println(OpReqData, BIN);
  
  for(byte deviceNum = 0; deviceNum < icCount; deviceNum++) // iterate through connected boards.
  {
    Serial.print("reading device: ");
    Serial.println(deviceNum);
    
    
    if (ATA68_GetOpStatus(deviceNum) == 1) // check and clear exixting commands
    {
      Serial.println("opstatus not cleared, clearing");
      ATA68_GetOpStatus(deviceNum);
      
      byte OpClear = 0X00;
      
      ATA68_Transfer(deviceNum, OpReq, &OpClear, SEND, 1); //start conversion.
      
      delay(5);
      
      ATA68_getStatus(deviceNum);
      ATA68_GetOpStatus(deviceNum);
      
     }
    
    
      
       ATA68_Transfer(deviceNum, OpReq, &OpReqData, SEND, 1); //start conversion.
  
       //wait for interrupt. - should actually wait for interrupt
       byte whileopstatus = 0;
       while (whileopstatus != 2) // check if dataready bit is set. should be replaced with an interrupt based dataready check.
       { 
          whileopstatus = ATA68_GetOpStatus(deviceNum);
          
          Serial.print("opreqdata ");
          Serial.println(OpReqData, BIN);
          if (whileopstatus == 0){ATA68_Transfer(deviceNum, OpReq, &OpReqData, SEND, 1);} //start conversion.
          if (whileopstatus == 3){return 3;}
          
          delay(100); //allow the chip to collect data. this value is not fine tuned and is only meant to allow this function to work.
          // 8.2ms conversion time according to datasheet.
          // should actually wait for interrupt in a future version of code. a delay like this could cause problems  
       }
    
       delay(10); // allow chip to work further.
       //voltage = (Vbuffer[0] * 256) + Vbuffer[1];
   
       if (ATA68_getStatus(deviceNum) & 0x01) // if dataready bit is set, read data.
       {
          byte BurstBuffer[13]; 
          for (int i; i <= 13; i++) 
          { BurstBuffer[i] = 0; } // clear buffer
    
          ATA68_Transfer(deviceNum, DataRd16Burst, &BurstBuffer[0], RECIEVE, 14);
       
       for (int d; d <= 13; d++) // print out all recieved binary values for debugging.
             {
                Serial.print("burst data recieved: ");
                Serial.println(BurstBuffer[d], BIN);
             }
    
          if ((ATA68_getStatus(deviceNum) & B00010100) < 1) // Check that communication worked
          {
            delay(10);
        
           
       
            // convert bytes to readable ints. currently for big endian while avr is little endian.
            tBurstDataType* pBatteryData = (tBurstDataType*) BurstBuffer;
    
            Serial.print("Burst cell #1 = ");
            Serial.println(pBatteryData->channel6, BIN);
    
            Serial.print("Burst cell #2 = ");
            Serial.println(pBatteryData->channel5, BIN);
    
            Serial.print("Burst cell #3 = ");
            Serial.println(pBatteryData->channel4, BIN);
    
            Serial.print("Burst cell #4 = ");
            Serial.println(pBatteryData->channel3, BIN);
    
            Serial.print("Burst cell #5 = ");
            Serial.println(pBatteryData->channel2, BIN);
    
            Serial.print("Burst cell #6 = ");
            Serial.println(pBatteryData->channel1, BIN);
    
            Serial.print("Burst temp    = ");
            Serial.println(pBatteryData->temperature, BIN);
       
            ATA68_GetOpStatus(deviceNum);
          } 
          else 
          { 
          bulkReadError = 2; Serial.println("cummunication error, data not recorded");
          }
          
       }
       else
       {
         Serial.println("data not ready"); 
         ATA68_GetOpStatus(deviceNum);
       }
       
    
  }
  return bulkReadError;
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
  
  uint8_t OpStatusData;
  ATA68_Transfer(deviceNum, Opstatus, &OpStatusData, RECIEVE, 1); 
  
  // debugging stuff
  Serial.print("OPstatus = ");
  Serial.println(OpStatusData, HEX);
  
  return OpStatusData;
}



byte ATA68_getStatus(uint8_t deviceNum)
{
  uint8_t statusRegData;
  ATA68_Transfer(deviceNum, statusReg, &statusRegData, RECIEVE, 1);
  
  Serial.print("Status = ");
  Serial.println(statusRegData, BIN);
  
  return statusRegData;
}


void ATA68_ReadControlData ()
{
 /*
 Reads control data from registers and populates a bitfeild. should also create a write function at some point.
 registers read\
 
const byte RevID = 0X00;             // Revision ID/value Mfirst, pow_on [-R 8bit]
 
 //bit 0 = IC rev number.  bit1 revb = 0x02
 //bit 1 = IC rev number.  bit2
 //bit 2 = IC rev number.  bit3
 //bit 3 = Mfirst     -> status input pin MFIRST
 //bit 4 = pow_en     -> status input pin POW_EN
 //bit 5 = n/a
 //bit 6 = n/a
 //bit 7 = n/a

//////////////////////////////////
const byte Ctrl = 0X01;              // control register [-RW 8bit]



const byte OpReq = 0X02;         // operation request [-RW 8bit]
const byte Opstatus = 0X03;          // operation status [-R 8bit]
const byte Rstr = 0X04;              // software reset [-W 8bit]
const byte IrqMask = 0X05;           // mask interrupt sources [-RW 8bit]
const byte statusReg = 0X06;         // status interrupt sources [-R 8bit]
 
 */
}




void ATA68_SetUdvTrip(int LowVoltage) // set the low voltage trip point in the ATA6870N
{ // useage
  // binary value of low voltage trip point.
  
  uint8_t UdvThreshData[2] = {(LowVoltage & 0xff), (LowVoltage >> 8)} ; // turn the 16 bit int into 2 bytes.

  for(int deviceNum; deviceNum < BALANCERCOUNT; deviceNum++) // iterate through all modules in the string.
  {
    ATA68_Transfer(deviceNum, UdvThreshold, &UdvThreshData[0], SEND, 2);
  }
}

void ATA68_SelectTempSensor(uint8_t deviceNum, boolean TempBit) // select temparature sensor. 1 or 0
{ 
  // useage
  // board -- board number, 0 to 15
  // TempBit -- temperature sensor. 1 for external, 0 for internal. (check the wiring on this, could be backwards)
  
  
  //ATA68_Transfer(deviceNum, , , SEND, 1);
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
Serial.print("irqTrigger");

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
  SPI.transfer(0X00);//pulse out 4+ clock ticks before communication
  digitalWrite(ATA_CS, LOW);  // start spi transfer by selecting chip

    ATA68_Select(deviceNum);
    
    SPI.transfer(regAddress); //select register and set r/w bit 

    if(length != 0) // check that length is valid
    {
      for (int i = 0; i < (length); i++) {  //recieve actual data. length = length in bytes for message
        SPIbuffer[i] = SPI.transfer(SPIbuffer[i]); // recieve/transmit data.
      }
    }
    
    #ifdef CHECKSUM_ENABLED
    // generate and send checksum
      uint8_t checksum = 0x00; // byte to store & minipulate the lfsr output
 
      checksum = ATA68_calcLFSR(checksum, regAddress); //run calculation of register adress
    
      for(uint8_t i=0; i<length; i++)
      {
         checksum = ATA68_calcLFSR(checksum, SPIbuffer[i]); // run calculation of data byte by byte. 
      }
    
      SPI.transfer(checksum);
      // check for bad comm interrupt in status register!
    #endif
  
  digitalWrite(ATA_CS, HIGH); // end spi transfer by deselecting chip
  SPI.transfer(0X00);//pulse out 4+ clock ticks after communication to "complete" the transaction.
  SPI.endTransaction();

}


//////////////////////////////////////////////////////////////////////////////////////////
void ATA68_Select (uint8_t StringAddress) // selects device in string, transfers address command and gets irq.
{ // useage
  // StringAddress = device number. device 0-15. will skip communication of data is not within this value. (all 16 bits set to "0")

  uint16_t stackAddress = 0x0001 << StringAddress; // Address of selected chip. First byte is shifted left once for each chip increment
  
  // select device#, recieve irq data, and add activated irq bits to the irqStore "list"
  irqStore = irqStore | (SPI.transfer(stackAddress >> 8) * 256); // transcieve first address byte
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

