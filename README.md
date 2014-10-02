(this readme is preliminary. expect errors. this will be updated in the coming week))

Intro
==========
* This project aims to create an open source, scalable Battery management system for use in just about anything with more than 4 lithium batteries.
* It aims to be as open source as possible all software should ideally be free and unlimited. All PCB designs are done in KiCad and all programming is done in .....


Boards
===============
There are 3 boards in this system

* 1. The master board. it contains the mcu (The same microcontroller used in the teensy 3.1) The CAN transceiver and a couple other odds and ends such as a contactor driver.
* 2. The daughter cell management board. It contains the voltage measuring ic and balancing resistors.
* 3. The bus termination board. this one does precious little except ties the datapins of the top slave board together and allow activation of the system with an optoisolator.




Master Board Features / goals
===============
* Teensy 3.1 / Arduino Micro-controller (mk20dx256)
* Low Voltage Protection
* High Voltage Protection
* USB programmable and readable - possibility of usb otg.
* CAN capable for connections to commercial motor controllers and other CAN enabled systems.
* Connectors for an LCD Display and Current Sensor
* implement Coulomb Counting?
* mosfet contactor driver. also driver for a buck power supply for onboard 12V wen vehicle is turned on.

Daughter Board Features / goals
===========

* Measures 4-6 Li-Ion Cells in Series 
* Stackable Architecture Enables >400V Systems (Up to 16 6s modules)). - Master modules can be isolated and daisy chained as well for even higher voltages (opto serial to base module)
* 0.25% Maximum Total Measurement Error (12 bit adc)
* extremely low power goal is under 25ua when asleep.
* Cell Balancing:
* balancing achieved with 10 ohm power resistors. This means ~1.8 watts power dissipation per cell. heatsinking is needed at this level.
* crude pwm'ing of cell balance resistors for heat management. 25-100% duty cycle in increments of 25%
* Two Thermistor Inputs - one connected to an On-Board Temperature Sensor
* temperature management. software will start to pwm balance resistors of board gets too hot.

Mechanical Goals
===========
* robust 
* Simple to Assemble
* scalable (modules daisy-chainable)
* Operating Temperature -55oC to +85oC

software Goals
===========
* make a pc based cross playform gui. initially working on either windows or linux.
* have a readout of all cell voltages
* manually control system outputs
* debug can connection
* ladder logic programming?
  
  
Photos
===========
![board](https://github.com/teslafly/openBMS/photos/master/Photos/Board.png?raw=true)


TODO:
===============
* Actually write code for the thing
* finish and post the cpu board
* make a separate folder for all the kicad libraries and 3d models used in the project
* include a bom with digikey / mouser part#'s 
* write a blog post on teslafly.wordpress.com detailing this project.
* make this description more descriptive and concise. 



License
===========
TBA