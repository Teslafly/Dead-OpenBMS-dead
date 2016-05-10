/* 
configuration file. 
this file contains pinouts, battery chemistry specific settings, and other important configurations.
*/

// system configuration ////////////////////////////
#define MOTHERBOARD
#define BAUDRATE 19200 // serial baudrate. default: 9600
#define BLE_ENABLED // enables nRF8001 BLE module for mobile device communication.

#define DEBUG_INFO // enables printing of debug info 
//#ifdef DEBUG_INFO
 
//#endif  
  
//#define CHECKSUM_ENABLED  // enables chcksum on spi communication with ata68. currently broken.
//#define SAFETY_NOSHOUTDOWN // disables system shutdown in case of a fault. You would use this for safety critical applications where the battery system can NOT be shut down.

// battery specific settings ///////////////////////
#define BALANCERCOUNT 2 // # of ATA6870N attached in series
#define EN_BALANCING // Enables balance function. board otherwise operates in a voltage sense only mode. - not currently checked.
#define BAL_TOLERANCE 0.004 // balance tolerance. if cells are within this value of the lowest cell they will be considered "balanced".
#define BAL_STARTV 4.15 // default 4.07 - balance start voltage. if cell voltage is above this it will start balancing
#define MAXTEMP 670 // maximum tempereture. adc is inverted so maxtemp is the minimum pcbtemp adc value.
#define BRKN_WIRE_DETECT // broken wire detection. measures voltage, turns on cell and measures voltage again. The difference should not exceed VOLTAGEDROP. 
#define VOLTAGEDROP 10 // voltage difference in millivolts for before / after broken wire check readings.
#define UDV_TRIP 3.1 // undervoltage trip value.
#define SYS_VOLTAGE 10 // minimum system voltage until it will go into an error state and disable some functions until proper voltage to aux is restored.
// minvoltage(per cell)
// maxvoltage(per cell)
// balancevoltage
// BAL_START 4.0 // voltage that cells will start getting balanced at. not currently used for lack of algorthim implementation.
// BAL_END 4.1 // voltage that balance will bring each cell down to.
// CHARGE_END 4.15 // voltage that when reached by 1 cell, the charger will shut off at.


// If a cell is unconnected set it to "0". connected cells are "1". 
// each board is one byte with the 6 cells defaulted to "1" or connected. 
// This array probably has more values your board stack. It is unnessasary 
// to set board bytes to all "0"s if they do not exist in your stack and you have set 
// "BALANCERCOUNT" to the correct value. (Although it won't hurt to shrink it to the correct size)

// unconnected cells/inputs. - unconnected cells cannot be the highest or lowest cells, and unconnected inputs must be shorted to the lowest cell.
byte UNCONNECTED_CELLS[] = 
  { B00111111, // board 0
    B00111111, // board 1
    B00111111, // board 2
    B00111111, // board 3
    B00111111, // board 4
    B00111111, // board 5
    B00111111, // board 6
    B00111111, // board 7
    B00111111, // board 8
    B00111111, // board 9
    B00111111, // board 10
    B00111111, // board 11
    B00111111, // board 12
    B00111111, // board 13
    B00111111, // board 14
    B00111111  // board 15
  }; 
  //unconnected cells will turn their balance resistors on. if the pack is not balancing, and no balancing LEDs are lit, you have not defined any connected cells as unconnected.
  #define UNCONNECTED_UDV 100 // maximum voltage for unconnected cells in millivolts for unconnected cells. Will report an error for unconnected cells under this amount.
  //UdvThreshData
  
 


///////////////////////////////////////////////////////////////////
// pinouts
///////////////////////////////////////////////////////////////////
#if defined(__AVR_ATmega328P__) // select pinouts for avr chips 

// digital
#define IRQ_PIN 2 // irq interrupt pin from ata6870n chip.
#define IRQ_INT 0
#define ATA_CS 6 // ATA6870N chip select pin.

// pwm
#define DRIVER1 5  // npn mosfet driven output #1
#define DRIVER2 6 // npn mosfet driven output #2

// Analog
#define SYS_VOLTAGE 10 // minimum system voltage until it will go into an error state and disable some functions until proper voltage is restored.
#define SYS_VOLTPIN A7// analog pin 1 for onboard voltage. usually 12-24v

// bluetooth stuff
// Connect CLK/MISO/MOSI to hardware SPI
// e.g. On UNO & compatible: CLK = 13, MISO = 12, MOSI = 11
#define ADAFRUITBLE_REQ 8
#define ADAFRUITBLE_RDY 2     // This should be an interrupt pin, on Uno thats #2 or #3
#define ADAFRUITBLE_RST 5



#elif defined(__arm__) && defined(CORE_TEENSY) // pinouts for teensy 3.1 based boards

// digital
#define IRQ_PIN 2 // irq interrupt pin from ata6870n chip.
#define IRQ_INT IRQ_PIN // interrupt is the same as the above pin number.
#define ATA_CS 7 // ATA6870N chip select pin.
// ATA_CLK = pin ? // 500 khz hardware clock not yet implemented

// pwm
#define DRIVER1 -1  // npn mosfet driven output #1
#define DRIVER2 -1 // npn mosfet driven output #2

// Analog
#define SYS_VOLTPIN 4 // analog pin 1 for onboard voltage. usually 12-24v


// bluetooth stuff
// Connect CLK/MISO/MOSI to hardware SPI
// e.g. On UNO & compatible: CLK = 13, MISO = 12, MOSI = 11
#define ADAFRUITBLE_REQ 10
#define ADAFRUITBLE_RDY 2     // This should be an interrupt pin, on Uno thats #2 or #3
#define ADAFRUITBLE_RST 9



#endif
