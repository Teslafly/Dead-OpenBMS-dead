
///////////////////////////////////////////////////////////
// function for getting data out of the ata6870n
uint8_t *ATA68_Transfer (uint8_t deviceNum, uint8_t regAddress, uint8_t *SPIbuffer, boolean RW, uint8_t length )
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
 
      checksum = ATA68_calcLFSR(checksum, regaddress); //run calculation of register adress
    
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
  // StringAddress = device number. device 0-15. will skip communication of data is not within this value.

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
