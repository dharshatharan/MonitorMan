# MonitorMan
Embedded programming using a MSP432-P401R board

## Overview
A monitoring application that communicates with the MSP432-P401R via the serial port (RS232) over a USB connection.
The application consists of two parts, one is the software that is installed in the MSP432 board and one that is installed in a PC. 
Both these parts communicate with each other to constuct and manupulate multiple states of the MSP432 board. The states can be changed 
either by interacting with the buttons on the board or using the GUI in the python part of the application.
