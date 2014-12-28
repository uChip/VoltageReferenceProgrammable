Programmable Voltage Reference  
==============================

<img src="https://raw.githubusercontent.com/uChip/VoltageReferenceProgrammable/master/4000mV.jpg" alt="Programmable Voltage Reference with 1 mV resolution and 100 uV accuracy" height="260" width="390">  
This repository contains the design files and write-up for a Programmable Voltage Reference.  The reference has a range of 1 mV to 4.095 volts in 1 mV steps.  The accuracy is plus or minus 100 uV for any set value.  The reference value is displayed on a 4 digit LED display and can be set manually using the rotary encoder or remotely over serial (FTDI port).

The software folder contains an Arduino sketch that is source code for the voltage reference firmware.

See the file PVR.pdf for a more detailed explanation of the project design. 

If there is sufficient interest I will build and calibrate a batch of these to sell.  If you would like one, please contact me. 

## Order PCBs  

You can order this PCB directly from OSH Park.  Click on the following link.  
  * Programmable Voltage Reference - https://oshpark.com/shared_projects/aFUNW4O5 

<img src="https://raw.githubusercontent.com/uChip/VoltageReferenceProgrammable/master/RevDtop.png" alt="PCB Top" height="287" width="550">

<img src="https://raw.githubusercontent.com/uChip/VoltageReferenceProgrammable/master/RevDbottom.png" alt="PCB Bottom" height="287" width="550">

See the Bill of Materials (BOM) file in the repo Hardware folder for a parts list.  

## Status  
  * Rev-D PCB has been tested to be functional.  

## File Formats  

Hardware design files are in "CadSoft EAGLE PCB Design Software" .brd and .sch formats.  A free version of the software can be downloaded from www.cadsoftusa.com. 

The example code is in Arduino .ino format (text).  A free version of the Arduino software can be downloaded from www.arduino.cc.  

## Distribution License  

License:
<a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-sa/4.0/88x31.png" /></a><br /><span xmlns:dct="http://purl.org/dc/terms/" property="dct:title">Programmable Voltage Reference</span> by <a xmlns:cc="http://creativecommons.org/ns#" href="https://github.com/uChip/VoltageReferenceProgrammable" property="cc:attributionName" rel="cc:attributionURL">C.Schnarel</a> is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/">Creative Commons Attribution-ShareAlike 4.0 International License</a>.
  


