/* 
configuration file. 
this file contains pinouts, battery chemistry specific settings, and other important configurations.
*/

// system configuration ////////////////////////////
#define MOTHERBOARD
#define BAUDRATE 19200 // serial baudrate. default: 9600
#define BLE_ENABLED 0 // enables nRF8001 BLE module for mobile device communication.
#define SERIALBUFF 30 // buffer size for serial commands. shouldn't be more than 30 for upcoming bluetooth module.
#define DEBUG_INFO 1 // prints debug info if set to "1". stops transmissoon of boot up matadata if set to "0".
//#define TESTMODE //
//#define CHECKSUM_ENABLED  // enables chcksum on spi communication with ata68. May consume slightly more power and slow down spi bus.
//#define SAFETY_NOSHOUTDOWN // disables system shutdown in case of a fault. You would use this for safety critical applications where the battery system can NOT be shut down.

// battery specific settings ///////////////////////
#define BALANCERCOUNT 2 // # of ATA6870N attached in series
#define EN_BALANCING // Enables balance function. board otherwise operates in a voltage sense only mode.
#define BRKN_WIRE_DETECT // broken wire detection. measures voltage, turns on cell and measures voltage again. The difference should not exceed VOLTAGEDROP. 
#define VOLTAGEDROP 10 // voltage difference in millivolts for before / after broken wire check readings.
#define UDV_TRIP 3.1 // undervoltage trip value. 
// minvoltage(per cell)
// maxvoltage(per cell
// balancevoltage
// BAL_START 4.0 // voltage that cells will start getting balanced at. not currently used for lack of algorthim implementation.
// BAL_END 4.1 // voltage that balance will bring each cell down to.
// CHARGE_END 4.15 // voltage that when reached by 1 cell, the charger will shut off at.


// if a cell is unconnected set it to "0". connected cells are "1". 
// each board is one byte with the 6 cells defaulted to "1" or connected. 
// this array will probably be longer than your board stack. it is unnessasary 
// to set board bytes to all "0"s if they do not exist in your stack and you have set 
// "BALANCERCOUNT" to the correct value.

// unconnected (shorted) cells/inputs.
/*uint8_t UdvThreshData[2] = 
  { B00111111, // board 0
    B00111111, // board 1
    B00111111, //...
    B00111111, 
    B00111111, 
    B00111111, 
    B00111111,
    B00111111, 
    B00111111, 
    B00111111,
    B00111111, 
    B00111111,
    B00111111,
    B00111111,
    B00111111,
    B00111111
  }; */
  //unconnected cells will turn their balance resistors on. if the pack is not balancing, and no balancing LEDs are lit, you have not defined any connected cells as unconnected.
  #define UNCONNECTED_UDV 100 // maximum voltage for unconnected cells in millivolts for unconnected cells. Will report an error for unconnected cells under this amount.
  
  
 

// maxcurrent


// pinouts for avr chips ///////////////////////////////////

#if defined(__AVR_ATmega328P__) // select pinouts for avr chips 

// digital
#define IRQ_PIN 2 // irq interrupt pin from ata6870n chip.
#define IRQ_INT 0
#define ATA_CS 8 // ATA6870N chip select pin.

// pwm
#define DRIVER1 5  // npn mosfet driven output #1
#define DRIVER2 6 // npn mosfet driven output #2

// Analog
#define SYS_VOLTAGE 10 // minimum system voltage until it will go into an error state and disable some functions until proper voltage is restored.
#define SYS_VOLTPIN A7// analog pin 1 for onboard voltage. usually 12-24v




#elif defined(__arm__) && defined(CORE_TEENSY) // settings for teensy

// pinouts for teensy 3.1 //

// digital
#define IRQ_PIN 2 // irq interrupt pin from ata6870n chip.
#define IRQ_INT IRQ_PIN // interrupt is the same as the above pin number.
#define ATA_CS 7 // ATA6870N chip select pin.
// ATA_CLK = pin 6

// pwm
#define DRIVER1 5  // npn mosfet driven output #1
#define DRIVER2 10 // npn mosfet driven output #2

// Analog
#define SYS_VOLTAGE 10 // minimum system voltage until it will go into an error state and disable some functions until proper voltage is restored.
#define SYS_VOLTPIN 4 // analog pin 1 for onboard voltage. usually 12-24v


#endif
