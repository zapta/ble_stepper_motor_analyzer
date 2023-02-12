# ESP32 STEPPER MOTOR ANALYZER

TODO: Add a picture of the device
TODO: Add a screen shot of the desktop app

## Description

The ESP32 Stepper Motor Analyzer ('the analyzer') is a low-cost, open source system that allows to analyze the behavior of stepper motor in systems such as 3D printers, and is made of two parts:

1. The Stepper Motor Probe ('the device'). This is a small electronic board that monitors the currents through the stepper motor wires, extracts information such as step count, and speed, and transmits them in real time to an app via Bluetooth BLE.
2. The Analyzer App ('the app'). This is a Python program that runs on a Windows, MaC OSX, or Linux PC, connects via Bluetooth BLE to the device and displays the stepper information in real time in a graphical view.

## Highlight
* The device is passive and doesn't not interfere with the operation of the stepper, regardless if the device is in use or turned off.
* Embedding the device in a 3D printer is easy. It is small, operates on 7-30 VDC, 1W, and uses a small external sticker antenna.
* The entire design is in the public domain and can be used commercially with no attribution or open source requirements.
* The electronic design is based on common components such that the device can be ordered fully assembled from JLCPCB SMT service (about $12/unit in quantities of 30, as of Feb 2023)
* The device firmware can be updated by users using a standard USB cable, no other tools required.
* Each device has its own unique address such that multiple devices can be used in parallel to monitor multiple stepper motors.
* An open Python API allows to write custom monitoring apps.

## How does it work?
The device contains two galvanic isolated current sensors that measure the current through the two stepper motor coils 40,000 times a second, a firmware that analyze the current readings and tracks information such as step count, and speed, and a Bluetooth BLE radio unit which transmits this information to the app which displays it with visual graphs.  



## Specification

Item | Specification
:------------ | :-------------
Power consumption | 7-30 VDC, 1W.
Firmware update connector | USB Type C
SOIC Module | ESP32-WROOM-32U-N4 
Current measurement | +/-2.5A per coil.
PCB | 39mm x 43mm, two layers
Antenna  | 2.4Ghz external antenna IPX IPEX connector.
Sensor isolation | See CC6920BSO-5A datasheet
Zero calibration | Using onboard button.
Count direction | User selected, 
Sampling rate | 40Khz per channel.
Sampling resolution | 12bits
Current accuracy | estimated at +/- 2%
Max step rate | 5K full steps/sec.
Step resolution | 1/10th of a full step.
Firmware programming language | C++
Firmware programming IDE | VSCode, Platformio, esdidf framework.
Firmware debugging | Via optional 10 pins JTAG connector.
Electronic design software | Kicad.
Mechanical design software | Onshape.
Open source license | Creative Commons CC0.

## Building your own

1. Order assembled boards (recommended) or just the PCBs and assembled the components yourself. Production files for JLCPCB PCB and SMT services are included.
2. Connect to a compute via a USB cable and flash the firmware.
3. The the stepper motor not connected and the devices powered, press the button for 5 second until the LED will flash three time. This calibrate and store the zero level of the sensors.
4. Run the analyzer app and verify that it connects to the device and displays its data.

## Installing the device in a 3D printer

1. 3D print a device carrier (STL models provided) and attach the device to the carrier using two pieces of 3M VHB tape or similar.
2. Attach the device carrier to the 3D printer using 3M VHB tape.
3. Connect the external antenna and stick it to the outside of the 3D printer. 
4. Disconnect the 4 wires of the stepper motor through the two connectors on the board. The two connectors implement a pass through path through the current sensors.
5. Connect the power input of the device to a power source (7-30VDC). Pay attention to the polarity though reverse polarity should not damage the device. 
6. Turn on the 3D printer and notice that device has one LED solid on and another blinks every second or so. You can now use the 3D normally, and connect and monitor the stepper from the app, any time you want.

## Operating the Analyzer app.











