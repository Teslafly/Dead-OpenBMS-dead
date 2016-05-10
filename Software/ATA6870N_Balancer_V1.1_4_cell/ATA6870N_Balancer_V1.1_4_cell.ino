
/*
This program is meant as a proof of concept test for the ATA6870N bms chip. 
as of now it just reads and prints out cell voltages for the specified amount of modules, and 
has a bunch of other junk laying aroung for not quite implemented features. 

This program was built on arduino 1.0.6

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

#include <Arduino.h>
#include "Configuration.h"
#include <SPI.h>
//#include <MemoryFree.h>

// variables ///////////////////////////////
// serial communication stuff
String SerialBuffer = "";         // a string to hold incoming data
boolean SerialComplete = false;  // whether the string is complete

// yes, I know these are overindexed by 1. leaving out for memory overflow troubleshooting stuff
uint16_t cellReadings[(BALANCERCOUNT * 6)];// array to store all the cellReadings.
uint16_t cellCalVals[(BALANCERCOUNT * 6)];// array to store all calibration values for cells
uint16_t pcbTemps [BALANCERCOUNT ]; // array for storing system temperatures. 
uint16_t extTemps [BALANCERCOUNT ]; // array for storing external temperature measurements.

float cellVoltages [(BALANCERCOUNT * 6)]; // decimal cell voltages.



uint16_t irqStore; // global variable to store irq results. make this not be overwritten each spi transfer, but rather added to until it is read and cleared. (if interrupts are used at all)
uint8_t mode = 0;// system modes. modes listed below // not used right now.
// 0 == waiting. system is starting up and not ready for user input/output.
// 1 == ready. system is ready for user input
// 2 == error. an error has been detected and the system has halted.- make sure the system can override faults and continue working for safety reasons.
// 3 == balalcing. system has detected that the battery is charging and that cells have passed a balancing threshold.
// 4 ==
// 5 == sleep. system is asleep and will wakeup once an interval timer has elapsed.

// variables for balance functions
 byte balanceStates [BALANCERCOUNT]; // states of all balance loads
 int completeCells = 0;
 
 // highest and lowest voltages of cells in the stack.
 float maxvoltage;
 float minvoltage;
 
 boolean balEn = 1;
 
 


// set up devices //////////////////////////
void setup(){
 
  SerialBuffer.reserve(SERIALBUFF);  // reserve bytes for incoming communication
  
  Serial.begin(BAUDRATE);
  Serial.println(F("ATA6870N balancer V1.0")); // splashscreen
 
  
  
  ATA68_initialize(BALANCERCOUNT);

    #ifdef DEBUG_INFO
      const boolean debugSerial = 1;
    #endif
    
  // get system status
  uint8_t Ecode = ATA68_StartupInfo(debugSerial); //scan system and output useful data. Returns an error code if anything is wrong. 
  if(Ecode > 0) // print error code if error detected.
  { 
    Serial.print(F("Error: "));
    Serial.println(Ecode);
    mode = 2; // set system to error mode.
    Serial.print(F("mode set to mode#"));
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
  
   
   
 
  balance(balanceStates, UNCONNECTED_CELLS, 0); // turn off cells for voltage read
  
  delay(10); //let voltages stabilize
  
     
  // read all cells and calibration values
  int bkrdError = ATA68_bulkRead(&cellReadings[0], &pcbTemps[0], BALANCERCOUNT, 1, 1); // bulk read command tester
  Serial.print(F("bulkread error = "));
  Serial.println(bkrdError);
  
  bkrdError = ATA68_bulkRead(&cellCalVals[0], &extTemps[0], BALANCERCOUNT, 0, 0); // calibration read tester
  Serial.print(F("cal bulkread error = "));
  Serial.println(bkrdError);

  
  balance(balanceStates, UNCONNECTED_CELLS, 0); // turn cells back on

  CalcDeciVolts(cellVoltages, cellReadings, cellCalVals ); // calculate floating point voltages
 
  
  // Calculate temperatures in Celsius
    for(int i=0; i < BALANCERCOUNT ; i++) // external readings
  {
   //extTemps[i]
  }
  

   
   Serial.println(F("cell voltages"));
  for(int i=0; i < (BALANCERCOUNT * 6); i++)
  {
    Serial.print(F("cell"));
    Serial.print(i + 1); // turn zero index into 1 indexed.
    Serial.print(F(" = "));
    Serial.println(cellVoltages[i], 3);
  }
  
  for(int i=0; i < BALANCERCOUNT ; i++)
  {
    Serial.print(F("PcbTempSense"));
    Serial.print(i + 1); // turn zero index into 1 indexed.
    Serial.print(F(" = "));
    Serial.println(pcbTemps[i]);
  }
  
  for(int i=0; i < BALANCERCOUNT ; i++) // external readings
  {
    Serial.print(F("extTempSense"));
    Serial.print(i + 1); // turn zero index into 1 indexed.
    Serial.print(F(" = "));
    Serial.println(extTemps[i]);
  }
  
  Serial.println();
 
  
  Serial.print(F("Pack voltage = "));
  Serial.println(CalcTotalVolts(cellVoltages));
  
  
  CalcExtremes(cellVoltages);
  
  
  if (minvoltage +  BAL_TOLERANCE >= maxvoltage) // if all cells are balanced and if the highest cell is close enough to the lowest cell.
  { 
    balEn = 0; // disable balance resistors
    Serial.println(F("Balancing finished!"));
  }
  else
  {
    if( maxvoltage > BAL_STARTV) //only balance at top of charge. only for testing.
    {
     balEn = 1; // enable balance resistors
    completeCells = CalcBalanceCells(cellVoltages, balanceStates, UNCONNECTED_CELLS, (4.14 + BAL_TOLERANCE)); //calculate cells to be balanced.
    }
    else
    {
      Serial.println(F("voltage too low, please charge battery")); 
    }
  }
  
  
  for(int deviceNum = 0; deviceNum < (BALANCERCOUNT); deviceNum++)
  {
    Serial.print(F("board #"));
    Serial.print(deviceNum);
    Serial.print(F(" is balancing cells "));
    Serial.println(balanceStates[deviceNum], BIN);
  }
  
  //Serial.print(F("# of complete cells = "));
  //Serial.println(completeCells); // this value is garbage for some reason.
  
  for(int i=0; i < BALANCERCOUNT ; i++) // external readings
  {
    if(pcbTemps[i] < 670) // check if boards are oevetemp. value completley arbraraty and guessed for testing.
    {
      Serial.println(F("board overtemp, loads disabled. "));
      Serial.print("temp at ");
      Serial.println(pcbTemps[i]);
      balEn = 0; // disable balance resistors
    }
  } 
  
  balance(balanceStates, UNCONNECTED_CELLS, balEn); // takes balanceStates and writes them to the balancer boards
  
  
  
 // Serial.println("debug info");
  Serial.print(F("irqStore      = "));
  Serial.println(irqStore, BIN);
  irqStore = 0X0000; // empty irqsotore to make room for new data. Nothing else is done with this for now.
  
  Serial.println();
  //Serial.print("other debug stuff that may be of use goes here");
  
  
  delay(10000);   
}
