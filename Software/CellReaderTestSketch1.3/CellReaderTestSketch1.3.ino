
/*
This program is meant as a proof of concept test for the ATA6870N bms chip. 
as of now it just reads and prints out cell voltages for the specified amount of modules, and 
has a bunch of other junk laying aroung for not quite implemented features. 

funcions that need to be implemented:
Near future:
-- fix zero indexing of arrays and cells
--ATA6870N communication implemented as a "sudo" library **DONE**
--initalisation error detection (detect missing modules, etc.)*-sort of there, but only prints out diagnostics.
--cell voltage measurement - sort of working. *-burst mode in  progress
--load resistor control
--pwm for thermal management and overtemp shutdown
--implement ATA6870N checksum to improve reliability *- Working?
--charger control / low voltage cutoff / alarms.
--serial parsing and communication *-in progress. serial capture complete.

Barely more than a thought expierement:
--low power sleep modes
--contactor activation and precharging
--load resistor drain time counting for detecting weak cells
--current monitoring and watt-hour tracking & columb counting
--cell internal resistance estimation with current sensor
--cumulative mah drawn/added from pack and stored in eeprom
--sd card data logging - probably not happening. realtime streaming over bluetooth is a more viable option given the limited power budget (~20 mA)

goal for the program as of now is to add control for cell balance resistors and add a communication module backend. And also probably cleaning up the code in preperation for more complete functions.

Code created by: Marshall Scholz (aka, "Teslafly") 
and contributed to by: (insert yout name here) - nobody yet.


  CellReaderTestSketch. The beginning test software and library for communicating with the ATA6870N BMS IC
    Copyright (C) 2014  Marshall Scholz

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

(Licence added 12/30/2014.)


formatting
-- all defined variables are in caps
*/

#include "Configuration.h"
#include <SPI.h>

// variables ///////////////////////////////
// serial communication stuff
String SerialBuffer = "";         // a string to hold incoming data
boolean SerialComplete = false;  // whether the string is complete

uint16_t cellVoltages[(BALANCERCOUNT * 6) - 1];// array to store all the cellvoltages.
uint16_t cellCalVals[(BALANCERCOUNT * 6) - 1];// array to store all calibration values for cells.
uint16_t pcbTemps [BALANCERCOUNT - 1]; // array for storing system temperatures. 
uint16_t extTemps [BALANCERCOUNT - 1]; // array for storing external temperature measurements.

uint16_t irqStore; // global variable to store irq results. make this not be overwritten each spi transfer, but rather added to until it is read and cleared. (if interrupts are used at all)
uint8_t mode = 0;// system modes. modes listed below // not used right now.
// 0 == waiting. system is starting up and not ready for user input/output.
// 1 == ready. system is ready for user input
// 2 == error. an error has been detected and the system has halted.- make sure the system can override faults and continue working for safety reasons.
// 3 == balalcing. system has detected that the battery is charging and that cells have passed a balancing threshold.
// 4 ==
// 5 == sleep. system is asleep and will wakeup once an interval timer has elapsed.


// set up devices //////////////////////////
void setup(){
  Serial.begin(BAUDRATE);
  Serial.println("ATA6870N test sketch. initializing");
  // reserve 200 bytes for incoming commands:
  SerialBuffer.reserve(SERIALBUFF); //make space for serial commands
  
  #ifdef TESTMODE
  Serial.print("Board is running in Test Mode. AtA6870N communication disabled");
  #else
  
  delay (500);
  
    pinMode(IRQ_PIN, INPUT);
    pinMode(ATA_CS,  OUTPUT);
  
    ATA68_initialize(BALANCERCOUNT);
  
  delay(100);
  
    
    // get system status
    uint8_t Ecode = ATA68_StartupInfo(DEBUG_INFO); //scan system and output useful data. Returns an error code if anything is wrong. 
    if(Ecode > 0){ // print error code if error detected.
      Serial.print("Error: ");
      Serial.print(Ecode);
      mode = 2; // set system to error mode.
      Serial.print("mode set to: mode =");
      Serial.print(mode);
    }
    delay(100);
    
    // test balance resistors
     for(int i=0; i < BALANCERCOUNT ; i++) // iterate through connected boards. 
    {
       ATA68_ResistorControl(i, B00111111); // turn on all cells for that board.
       delay(300); // allow time to observe tap resistor activating.
       ATA68_ResistorControl(i, B00000000); // turn off all cells
    }// */
    
    ATA68_GetOpStatus(0); // debuggging
  #endif

}


// read and report cell voltages
void loop(){
  
    parseComms(); 
  /*
    for(int i=0; i < BALANCERCOUNT ; i++) 
    { // read all cell voltages. (yes I know this is horribly innefficent, but it will have to suffice until I get burstmode working.)
    for(int c=0; c<6; c++){
    cellVoltages[6*i+c] = ATA68_readCell(i,c);
    }
  }
  */
  
  delay(10);
  
   /*
   // test cell balance loads.
   for(int i=0; i < BALANCERCOUNT ; i++) // iterate through connected boards. 
    {
      for (uint8_t drain = 0x01; drain <= 64; drain <<= 1) // iterate through cells by bitshifting active cell until the "1" reaches the 6th bit.
      {
        ATA68_ResistorControl(i, drain); // send command
        
        delay(300); // allow time to observe tap resistor activating.
      }
      ATA68_ResistorControl(i, 0x00); // turn off all cells
    }
     */
  Serial.println("bulkread in progress");
  ATA68_bulkRead(&cellVoltages[0], &pcbTemps[0], BALANCERCOUNT, 1, 1); // bulk read command tester
  
   // transmit all cell voltages over serial ports. voltages are not scaled/modified in any way and may look like garbage.
  for(int i=0; i <= (BALANCERCOUNT * 6); i++)
  {
    Serial.print("cell");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(cellVoltages[i]);
  }
  
  
 // Serial.println("debug info");
  ATA68_GetOpStatus(0);
  Serial.print("irqStore      = ");
  Serial.println(irqStore, BIN);
  irqStore = 0X0000; // empty irqsotore to make room for new data. Nothing else is done with this for now.
  
  Serial.println();
  //Serial.print("other debug stuff that may be of use goes here");
  
  
  delay(5000);   
}
