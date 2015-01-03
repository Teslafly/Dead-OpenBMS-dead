/* 
configuration file. 
this file contains pinouts, battery chemistry specific settings, and otherr things
*/

// system configuration ////////////////////////////
#define baudrate 9600  // serial baudrate. default: 9600 
#define DEBUG_INFO 1 // prints debug info if set to 1. stops transmissoon of boot up matadata if set to "0".
#define TESTMODE // puts the board into test mode and neglects all communication failures with ata68 chips. useful for testing other parts of the system.
//#define CHECKSUM_ENABLED  // enables chcksum on spi communication with ata68. not currently supported.
//#define SAFETY_NOSHOUTDOWN // disables system shutdown in case of a fault. You would use this for safety critical applications where the vehicle can NOT be shut down.

// battery specific settings ///////////////////////
#define BOARDCOUNT 1 // # of ATA6870N attached in series
#define CELLCOUNT BOARDCOUNT * 6 // gets recieved # of cells. 6 per each board. do not disable unconnected cells here. May differ from actual cellcount. 
// minvoltage(per cell)
// maxvoltage(per cell
// balancevoltage
// BAL_START 4.0 // voltage that cells will start getting balanced at. not currently used for lack of algorthim implementation.
// BAL_END 4.1 // voltage that balance will bring each cell down to.
// CHARGE_END 4.15 // voltage that when reached by 1 cell, the charger will shut off at.
// unconnected cells?
// maxcurrent


// pinouts for avr chips ///////////////////////////////////

#if defined(__AVR_ATmega328P__) // select pinouts for avr chips 

// digital
#define IRQ_PIN 2 // irq interrupt pin from ata6870n chip.
#define IRQ_INT 0
#define ATA_CS 3 // ATA6870N chip select pin.

// pwm
#define DRIVER1 9  // npn mosfet driven output #1
#define DRIVER2 10 // npn mosfet driven output #2

// Analog
#define SYS_VOLTAGE 10 // minimum system voltage until it will go into an error state and disable some functions until proper voltage is restored.
#define SYS_VOLTPIN // analog pin 1 for onboard voltage. usually 12-24v




#else // select pinouts for arm chips (not avr)

// pinouts for teensy 3.1 //

// digital
#define IRQ_PIN 2 // irq interrupt pin from ata6870n chip.
#define IRQ_INT IRQ_PIN // interrupt is the same as the above pin number.
#define ATA_CS 3 // ATA6870N chip select pin.

// pwm
#define DRIVER1 9  // npn mosfet driven output #1
#define DRIVER2 10 // npn mosfet driven output #2

// Analog
#define SYS_VOLTAGE 10 // minimum system voltage until it will go into an error state and disable some functions until proper voltage is restored.
#define SYS_VOLTPIN 4 // analog pin 1 for onboard voltage. usually 12-24v


#endif
