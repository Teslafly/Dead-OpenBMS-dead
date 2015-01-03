
/*
This program is meant as a proof of concept test for the ATA6870N bms chip. 
as of now it just reads and prints out cell voltages for the specified amount of modules, and 
has a bunch of other junk laying aroung for not quite implemented features. 

funcionns  that are planned to be implemented are,
--ATA6870N communication implemented as a sudo library in other tab
--cell voltage measurement - sort of working. burst mode next.
--load resistor activation
--load resistor thermal management pwm and thermal shutdown
--load resistor activated time counting and reporting for detecting weak cells
--implement crc checking to improve reliability
--low power sleep modes
--serial parsing and communication
--contactor activation and precharging
--current monitoring and watt/hour tracking with columb counting
--sd card data logging
--charger control / low voltage cutoff/alarms.
--cell internal resistance estimation with current sensor?
--cumulative mah drawn/added from pack and #of charge cycles stored in eeprom(not including motor regen-- hopefully)
--add detection for when one of the ata68 chips becomes unconnected and is not the number of cells this program is made for.
--add some sort of initalisation error detection.

goal for program as of now is to get communication working where I can read all the cell voltages/temeratures and control the cell discharge resistors on one ata68. (the first one, for loops to access all of them will come later.)

Code created by Marshall Scholz (aka, "Teslafly") and lisence is (somthing I haven't decided yet)
-- also, the author of this program apparently can't spell. if you find any spelling errors please let him know on github. username "teslafly"


formatting
-- all defined variables are in caps
*/

#include "Configuration.h"
#include <SPI.h>

// variables ///////////////////////////////
int cellvoltage[CELLCOUNT];// array to store all the cellvoltages.
int irqStore; // global variable to store irq results. make this not be overwritten each spi transfer, but rather added to until it is read and cleared. (if interrupts are used at all)
byte mode = 0;// system modes. modes listed below // not used right now.
// 0 == waiting. system is starting up and not ready for user input/output.
// 1 == ready. system is ready for user input
// 2 == error. an error has been detected and the system has halted.- make sure the system can override faults and continue working for safety reasons.
// 3 == balalcing. system has detected that the battery is charging and that cells have passed a balancing threshold.
// 4 ==
// 5 == sleep. system is asleep and will wakeup once an interval timer has elapsed.


// set up devices //////////////////////////
void setup(){
  Serial.begin(baudrate);
  Serial.print("ATA6870N test sketch. initalising");
  
  pinMode(IRQ_PIN, INPUT);
  pinMode(ATA_CS,  OUTPUT);
  
  ATA68_initialize(BOARDCOUNT);
  
  // get system status
  byte Ecode = ATA68_StartupInfo(DEBUG_INFO); //scan system and output useful data. Returns an error code if anything is wrong. 
  if(Ecode > 0){ // print error code if error detected.
    Serial.print("Error: ");
    Serial.print(Ecode);
    
    mode = 2; // set system to error mode.
    Serial.print("mode set to: mode =");
    Serial.print(mode);
  }
    

}


// read and report cell voltages
void loop(){
   
    for(int i=0; i<= BOARDCOUNT; i++) { // read all cell voltages
    
    for(int c=0; c<6; c++){
    cellvoltage[6*i+c] = ATA68_readCell(c,i);
    }
  }
  
  
  // transmit all cell voltages over serial ports. voltages are not scaled/modified in any way and may look like garbage.
  
  for(int i=0; i <= CELLCOUNT; i++){
  
  Serial.print("cell");
  Serial.print(i);
  Serial.print(" = ");
  Serial.println(cellvoltage[i]);
  }
  
  Serial.println("debug info");
  Serial.print("irqStore = ");
  Serial.println(irqStore);
  irqStore = 0X0000; // set irqsotore to 0X0000 to make room for new data. Nothing else is done with this for now.
  
  //Serial.print("other debug stuff that may be of use goes here");
  
  
  delay(1000);
}
