This project aims to create an open source, scalable Battery management system for use in just about anything with more than 4 lithium batteries.

There are 3 boards in this system

1. the cpu board. it contains the mcu (The same microcontroller used in the teensy 3.1) The CAN transceiver and a couple other odds and ends such as a contactor driver.
2. The slave cell management board. It contains the voltage measuring ic and balancing resistors.
3. The bus termination board. this one does precious little except ties the datapins of the top slave board together and allow activation of the system with an optoisolator.


TODO:
- actually write code for the thing
- finish and post the cpu board
- make a separate folder for all the kicad libraries and 3d models used in the project
- include a bom with digikey / mouser part#'s
- write a blog post on teslafly.wordpress.com detailing this project.
- make this description more descriptive and concise. 
