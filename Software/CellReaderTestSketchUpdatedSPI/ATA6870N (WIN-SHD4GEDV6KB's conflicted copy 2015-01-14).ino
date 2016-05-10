/*
where all the magic happens with the ATA6870N. 
this file handles all of the communicationn, parsing, and management of the ATA6870N BMS IC.
*/
// spi communication specifics
// ata68 communicates msbf (most significant byte first)
// spi speed is set to lowest speed. (for now) For reliability and reducing potential problems resulting from high clock speeds while testing.
// chip is selected when cs line is held LOW.
// clock specifics are: 
// CPOL = 1?
// CPHA = 1?


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

// ata6870n register mapping
const byte RevID = 0X00;             // Revision ID/value Mfirst, pow_on [-R 8bit]
const byte Ctrl = 0X01;              // control register [-RW 8bit]
const byte OpReq = 0X02;         // operation request [-RW 8bit]
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

uint8_t buffer[15]; // spi databuffer. 1 byte for control, 14 for recieved / transmitted data.

typedef struct BurstDataType{
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
 
 tBooleanCellData UDVstatus[BOARDCOUNT-1]; // variable for storing cell undervoltage status's
 tBooleanCellData DrainLoadStatus[BOARDCOUNT-1]; // variable for storing status of cell balance resistors.
 
 
 // set up the SPI speed, mode and endianness for ATA6870N
SPISettings ATA68_SPIconfig(SPI_CLOCK_DIV128, MSBFIRST, SPI_MODE3); 


 // status register bits.
 //bit 1 = dataRdy    -> Conversion finished
 //bit 2 = LFTdone    -> Low frequency timer elapsed
 //bit 3 = commError  -> Bad SPI command detected (wrong length)
 //bit 4 = udv        -> Undervoltage detected
 //bit 5 = chkError   -> Error on checksum check
 //bit 6 = Por        -> Power on reset detected
 //bit 7 = TFMdeOn    -> Test mode on
 
 /*
 register bits
 
 
 */






//const byte crcTable[] = {}; // actually do somthing with this




uint8_t ATA68_StartupInfo(boolean sendInfo){// reads chip id, scans the bus, gets useful chip settings, and checks if everything is ok. outputs error code if not.
    uint8_t errorCode = 0; // store any generated error codes.
    
  for(uint8_t i = 0; i <= 15; i++){// get chip id's of all chips on bus. scan whole 16 possible chips.
  
    uint8_t buffer = (ATA68_READ(i, RevID, 1))[0]; // read data
    
    // error checking goes here.
    // check that only the first chip in the bus has the master bit set
    // check that there are the correct # of chips connected.
    // check hardware inputs for right configuration.
    
      if(sendInfo == 1) { // send useful data about chips
      Serial.print("Address#");
      Serial.print(i);
      Serial.print(" --");
      Serial.println(buffer);
      }
    
  }// end of for loop
  
  uint16_t info[20];// id's of all the chips, settings, etc.
  
  // get the rest of the useful settings
  // parse out useful settings and store then in the array.
  
  // always outputs at least 0. This is normal and means nothing is wrong. If the output is not 0 something is definately wrong.
  return errorCode;
}

void selectTempSensor(boolean sensor, uint8_t board){
  // useage
  // cell -- cell number, 0 to 5. 6 if you want to read temperature, 7 if you want to read the lft 
  // board -- board number, 0 to 15
  
}
  
  /* ATA68_WRITE(uint8_t board, OpReq, data);
  
} */





uint16_t ATA68_readCell(uint8_t cell, uint8_t board){ // reads individual cell - this works!
  // useage
  // cell -- cell number, 0 to 5. 6 if you want to read temperature, 7 if you want to read the lft 
  // board -- board number, 0 to 15
 
  uint16_t voltage;
 
 if((cell >=0) && (cell < 8)){ // error checking. The first 5 msb's must remain 0.
  ATA68_WRITE(board, ChannelReadSel, cell); // get the board to collect data
  
  delay(10); //allow the chip to collect data. this value is not fine tuned and is only meant to allow this function to work.

  uint8_t *buffer = ATA68_READ(board, DataRd16, 2);
  
  voltage = (buffer[0] * 256) + buffer[1]; // convert the 2 buffer bytes into one 16 bit voltage value.
 }else{
  voltage = 0; // if error checking fails, return somthing clearly out of range.
 }
   return voltage; 
}


//////////////////////////////
// read all cell voltages
//////////////////////////////
void ATA68_readAllvoltages(uint16_t cellcount){
  uint16_t cellvoltages[cellcount]; // store raw cell adc values here.
  char buffer[2];
  
  for(uint8_t i = 0; i <= BOARDCOUNT; i++){ //run once for every board
  //ATA68_READ(i, ); 
  }
}

uint8_t ATA68_bulkRead(uint8_t board){

  //BurstRx
//ATA68_WRITE(); // get the board to collect data

uint8_t *buffer = ATA68_READ(board, DataRd16Burst, 14);

tBurstDataType* pBatteryData = (tBurstDataType*) buffer;
Serial.print(pBatteryData->temperature);

//BurstRx
}


////////////////////////////////////////
// function to compare the voltages of the cells and balance them
////////////////////////////////////////
void ATA68_balance(uint16_t balanceVoltage, uint16_t maxDutyCycle){ // still figuring out how I want to do balancing.
  
}

/////////////////////////////////////
// function to turn on the balance resistors with a timer
/////////////////////////////////////
void ATA68_discharge(uint8_t device, uint8_t cells, uint8_t timer){
  // device = device number. device 0-15. 
  // cells = cells to drain. this is 8 bits with the lsb being cell 1 and bit 6 being cell 6. 1 for load on, 0 for load off)
  // timer = amount of time the balance resistors will be on for. useful for pwm as you can set it for 1 second, then wait 4 seconds to refresh.
  
  
}







void ATA68_ReadControlData ()
{
 /*
 Reads control data from registers and populates a bitfeild. should also create a write function at some point.
 registers read
 const byte RevID = 0X00;             // Revision ID/value Mfirst, pow_on [-R 8bit]
const byte Ctrl = 0X01;              // control register [-RW 8bit]
const byte OpReq = 0X02;         // operation request [-RW 8bit]
const byte Opstatus = 0X03;          // operation status [-R 8bit]
const byte Rstr = 0X04;              // software reset [-W 8bit]
const byte IrqMask = 0X05;           // mask interrupt sources [-RW 8bit]
const byte statusReg = 0X06;         // status interrupt sources [-R 8bit]
 
 */
}


/*
ATA68_status(){
  // add stuff here to read the opstatus register. not sure how I want to pass returned variables yet, although this needs to be checked before reading data / starting processess.
}
*/

void ATA68_SelectTempSensor(boolean TempBit) // select temparature sensor. 1 or 0
{
  
}


//////////////////////////////////////////////////////////////////////////
// base hardware call functions
//////////////////////////////////////////////////////////////////////////




void ATA68_IRQroutine() // attached to irq pin via interrupt and runs when pin triggers. Still figuring out what I want to do with this.
{ 
Serial.print("irqTrigger");

}


/////////////////////////////////////////////
// order of commands to ata68. data is sent from top to bottom.
//[chipId - returns irq. 16 bits]
//[control - returns nothing. 8 bits]
//[data - returns data if being read. returns nothing if being written. 8-112bits read. 8-16 bits write.]
//[checksum. optional. always sent by microcontroller when active]


// higher level functions that achieve base functionality.
void ATA68_initialize(uint16_t expectedBoardCount){
  pinMode(ATA_CS, OUTPUT);
  digitalWrite(ATA_CS, HIGH); // end spi transfer by deselecting chip
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128); // slowest clock possible, 62.5khz with an 8 mhz avr on 3.3v. chip clock is 500khz and spi clock must be at least half that. and lower the more chips are added to the bus.
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(3);
  
  // different way of inaiatalising spi. not used for now.  SPCR = ((1<<SPE)|(1<<MSTR)|(0<<SPR1)|(1<<SPR0)|(1<<CPOL)|(1<<CPHA));  // SPI enable, MSB first, Master, f/16, SPI Mode 1
  
  #ifdef CHECKSUM_ENABLED
  ATA68_WRITE(0, Ctrl, 0x20); // set control register with chechsum bit enabled (1)
  #else 
  ATA68_WRITE(0, Ctrl, 0x00); // set control register with checksum bit disabled (0)
  #endif
  
  //attachInterrupt( IRQ_INT, ATA68_IRQroutine, RISING); // attach function to interrupt. not needed at this point
  
  // should set the undervoltage threshold here too.
}


//////////////////////////////////////////////
// function for pushing data to the ata6870n. Only accepts 8 bit values.
 void ATA68_WRITE(uint8_t device, uint8_t address, uint8_t data)
 {
  // device = device number. device 0-15. will return an error if input is not within this value.
  // control = adress of the register we want to write/read data from. The last bit is the read/write control. (0 for read)
  // data = the data we want to send to the ata68.

  uint16_t stackAddress = 0x0001 << device; // Address of selected chip. First byte is shifted left once for each chip increment
  const int Length = 2;
  uint8_t SPIdata[Length]; // data to store all spi communication data
  SPIdata[1] = (address << 1)| 1; // shift register address one over to make room for read/write bit. [1 for write, 0 for read]
  //control = control | (accessDir << 0); // set read write bit. not sure if needed or if this can be done in last problem

  SPI.transfer(0x00);// somthing that pulses out 4+ clock ticks ptobably needs to go here according to the datasheet.
  digitalWrite(ATA_CS, LOW);  // start spi transfer by selecting chip

    //select device#, recieve irq data, and add activated irq bits to the irqStore "list"
    irqStore = irqStore | (SPI.transfer( stackAddress >> 8) * 256); // transcieve first address byte
    irqStore = irqStore | SPI.transfer(stackAddress & 0xFF); // trancieve second adress byte. 
    
    for (int i = 0; i <= Length; i++) {  //recieve actual data
      SPI.transfer(SPIdata[i]);
    }
    
    #ifdef CHECKSUM_ENABLED
    SPI.transfer(ATA68_genLFSR(&SPIdata, Length)); // send back checksum. currently not working, do not enable.
    #endif
  
  digitalWrite(ATA_CS, HIGH); // end spi transfer by deselecting chip
  SPI.transfer(0x00);// somthing that pulses out 4+ clock ticks ptobably needs to go here according to the datasheet.
  
  #ifdef SLOWCOMMS
  delay(2000); // debugging stuff. makes datasniffing easier.
  #endif
}


void ATA68_Select (uint8_t StringAddress, uint8_t RegAddress, boolean RW) // selects device in string, transfers address command and gets irq.
{
  // StringAddress = device number. device 0-15. will skip communication of data is not within this value.
  // RegAddress = adress of the register we want to write/read data from. 
  // RW = read / write bit. 0 for read, 1 for write.

  uint16_t stackAddress = 0x0001 << StringAddress; // Address of selected chip. First byte is shifted left once for each chip increment
  uint8_t control = (RegAddress << 1)| RW; // shift register address one over to make room for read/write bit.

  //select device#, recieve irq data, and add activated irq bits to the irqStore "list"
  irqStore = irqStore | (SPI.transfer(highByte(stackAddress)) * 256); // transcieve first address byte
  irqStore = irqStore | SPI.transfer(lowByte(stackAddress)); // trancieve second adress byte. 
  SPI.transfer(control); //select register and set r/w bit 
}



///////////////////////////////////////////////////////////
// function for getting data out of the ata6870n
uint8_t *ATA68_READ (uint8_t device, uint8_t address, uint8_t Length )
{
  // device = device number. device 0-15. will return an error if input is not within this value.
  // address = adress of the register we want to write/read data from. The last bit is the read/write control. (0 for read)
  // recievedLength = If I am recieving data how many bytes should I recieve? Also automatically adds padding to push bits out.
 // check if length is zero.
  


  SPI.beginTransaction(ATA68_SPIconfig); 
  SPI.transfer(0X00);//pulse out 4+ clock ticks before communication
  digitalWrite(ATA_CS, LOW);  // start spi transfer by selecting chip

    ATA68_Select(device, address, 0);

    for (int i = 0; i <= Length; i++) {  //recieve actual data
      buffer[i] = SPI.transfer(0x00);
    }
    
    #ifdef CHECKSUM_ENABLED
   //SPI.transfer(ATA68_genLFSR(control, &buffer)); // send back checksum. currently not working, do not enable.
    #endif
    
    
  
  digitalWrite(ATA_CS, HIGH); // end spi transfer by deselecting chip
  SPI.transfer(0X00);//pulse out 4+ clock ticks after communication to "complete" the transaction.
  SPI.endTransaction();


  #ifdef SLOWCOMMS
  delay(2000); // debugging stuff. makes datasniffing easier.
  #endif

  return buffer; //return recieved data array
}



#ifdef CHECKSUM_ENABLED
uint8_t ATA68_genLFSR(uint8_t *DataIn, uint8_t length) // Generate the lfsr based checksum
{ 
  // use
  // address = adress sent to ata68. becomes forst bit sent to lfsr.
  // DATA = input data for LFSR conversion. ranges from 1-15 bytes (&pointer)
  // output = 8 bit LFSR checksum

  // x^8+x^2+x+1
  // bitstream in MSBF - xor - DR - xor - DR - xor - DR - DR - DR - DR - DR - feedback to xor
/*
byte data = 170; //value to transmit, binary 10101010
byte mask = 1; //our bitmask
  
  for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
    if (data & mask) // if bitwise AND resolves to true
       { LFSR = LFSR | 0x80; } // set incoming bit to 1. bit is otherwise left at 0
  }
  
  for (const char * p = "Fab" ; c = *p; p++)
    SPI.transfer (c);
  
  struct spiaccess{  // one datatype for whole spi access. should it just be one byte array with a variable amount of bytes?
    uint8_t address;
    uint8_t data[14]:
  }
    
*/

   uint8_t data; //element to combine DataIn and address into one bitstream

    //byte length; //length of data in bits.
    uint8_t mask = data | 1; // make sure to clear out data to just leave size full of zeros
    uint8_t LFSR = 0x00; // byte to store & minipulate the lfsr output
 
    //for(uint8_t i=0; i<length; i++)
    for (mask = 00000001; mask>0; mask <<= 1) //iterate through bit mask until bit is pushed out the end.
    { //This function is loosely based on example code provided by wikipedia. http://en.wikipedia.org/wiki/Linear_feedback_shift_register
        boolean lsb = LFSR & 1; // Get LSB / the output bit
        LFSR >> 1;              // shift
        
        if (data & mask)        // if selected bit is true...
           { LFSR = LFSR | 0x80; } // set incoming bit to 1. bit is otherwise left at 0
           
        if (lsb == 1)           // apply toggle mask if output bit is 1. leave it alone if bit is zero
           { LFSR ^= 0xE0; }       // Apply toggle mask: x^8+x^2+x+1 or b11100000  
    } 

  return LFSR;
}
#endif

