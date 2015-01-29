
/*
This program is meant as a proof of concept test for the ATA6870N bms chip. 
as of now it just reads and prints out cell voltages for the specified amount of modules, and 
has a bunch of other junk laying aroung for not quite implemented features. 

funcions that need to be implemented:
Near future:
-- fix zero indexing of arrays and cells **DONE**
--cell voltage measurement - sort of working. *-burst mode working, voltage calclations are not.
--balancing algorigthim
--pwm for thermal management and overtemp shutdown
--implement ATA6870N checksum to improve reliability - implemented but not working
--charger control / low voltage cutoff / alarms.
--serial parsing and communication *-in progress. serial capture complete.

Barely more than a thought expierement:
--low power sleep modes
--contactor activation and precharging
--load resistor drain time counting for detecting weak cells
--current monitoring and watt-hour tracking & columb counting
--cell internal resistance estimation with current sensor
--cumulative mah drawn/added from pack and stored in eeprom
--realtime data streaming & control over bluetooth to phone app / computer. - BLE serial link. ( http://www.adafruit.com/product/1697 )

  Goal for the program as of now:
   - calculating voltages from data
   - implementing dumb balancing (if it's over a voltage turn in the balance resistor and toggle an i/o pin) - balancing algorithims will be able to be selected in configuration.h when more are added.
   - cleaning up diagnostic data and touching up documentation / style.

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
*/

#include "Configuration.h"
#include <SPI.h>

// variables ///////////////////////////////
// serial communication stuff
String SerialBuffer = "";         // a string to hold incoming data
boolean SerialComplete = false;  // whether the string is complete

uint16_t cellVoltages[(BALANCERCOUNT * 6)];// array to store all the cellvoltages.
uint16_t cellCalVals[(BALANCERCOUNT * 6)];// array to store all calibration values for cells.
uint16_t pcbTemps [BALANCERCOUNT ]; // array for storing system temperatures. 
uint16_t extTemps [BALANCERCOUNT ]; // array for storing external temperature measurements.

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
  // reserve bytes for incoming commands:
  SerialBuffer.reserve(SERIALBUFF); //make space for serial commands
  
  #ifdef TESTMODE
  Serial.print("Board is running in Test Mode. AtA6870N communication checks disabled");
  #endif
  
  ATA68_initialize(BALANCERCOUNT);

    
  // get system status
  uint8_t Ecode = ATA68_StartupInfo(DEBUG_INFO); //scan system and output useful data. Returns an error code if anything is wrong. 
  if(Ecode > 0) // print error code if error detected.
  { 
    Serial.print("Error: ");
    Serial.println(Ecode);
    mode = 2; // set system to error mode.
    Serial.print("mode set to: mode =");
    Serial.println(mode);
  }
    
    
  // test balance resistors
  for(int i=0; i < BALANCERCOUNT ; i++) // iterate through connected boards. 
  {
    ATA68_ResistorControl(i, B00111111); // turn on all cells for that board.
    delay(300); // allow time to observe tap resistor activating.
    ATA68_ResistorControl(i, B00000000); // turn off all cells
  }
    
   ATA68_GetOpStatus(0); // debuggging
    
   delay(1000);
}






// read and report cell voltages
void loop(){
  
 // parseComms(); 
  
   
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
    
    
     
  Serial.println("bulkread in progress");
  int bkrdError = ATA68_bulkRead(&cellVoltages[0], &pcbTemps[0], BALANCERCOUNT, 1, 1); // bulk read command tester
  Serial.println("bulkread error = ");
  Serial.println(bkrdError);
  
  ATA68_bulkRead(&cellCalVals[0], &extTemps[0], BALANCERCOUNT, 0, 0); // calibration read tester
  

  
   // transmit all adc readings
   Serial.println("calibration offsets");
   for(int i=0; i < (BALANCERCOUNT * 6); i++)
  {
    Serial.print("cell");
    Serial.print(i + 1); // turn zero index into 1 indexed.
    Serial.print(" = ");
    Serial.println(cellCalVals[i]);
  }
   
   Serial.println("cell adc readings");
  for(int i=0; i < (BALANCERCOUNT * 6); i++)
  {
    Serial.print("cell");
    Serial.print(i + 1); // turn zero index into 1 indexed.
    Serial.print(" = ");
    Serial.println(cellVoltages[i]);
  }
  
  for(int i=0; i < BALANCERCOUNT ; i++)
  {
    Serial.print("PcbTempSense");
    Serial.print(i + 1); // turn zero index into 1 indexed.
    Serial.print(" = ");
    Serial.println(pcbTemps[i]);
  }
  
  for(int i=0; i < BALANCERCOUNT ; i++) // external readings
  {
    Serial.print("extTempSense");
    Serial.print(i + 1); // turn zero index into 1 indexed.
    Serial.print(" = ");
    Serial.println(extTemps[i]);
  }
  
  
  
 // Serial.println("debug info");
  Serial.print("irqStore      = ");
  Serial.println(irqStore, BIN);
  irqStore = 0X0000; // empty irqsotore to make room for new data. Nothing else is done with this for now.
  
  Serial.println();
  //Serial.print("other debug stuff that may be of use goes here");
  
  
  delay(1000);   
}
