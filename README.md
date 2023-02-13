# BLE STEPPER MOTOR ANALYZER

<p align="center">
  The analyzer device<br>
  <img src="./www/device_overall.jpg" style="width: 350px;" />
</p>

<p align="center">
  The analyzer's app.
  <img src="./www/app_screenshot.jpg"  />
</p>


**A short video** of the app is available here https://vimeo.com/798169140

---
- [BLE STEPPER MOTOR ANALYZER](#ble-stepper-motor-analyzer)
  - [Description](#description)
  - [Design Highlights](#design-highlights)
  - [How does it work?](#how-does-it-work)
  - [Specification](#specification)
  - [Installing the device in your 3D printer](#installing-the-device-in-your-3d-printer)
  - [Using the Analyzer app.](#using-the-analyzer-app)
    - [Running the app using a provided binary](#running-the-app-using-a-provided-binary)
    - [Running the app using the Python code](#running-the-app-using-the-python-code)
    - [Understanding the app screen](#understanding-the-app-screen)
    - [App buttons](#app-buttons)
    - [Hidden app functionality](#hidden-app-functionality)
    - [App command line flags](#app-command-line-flags)
  - [Flashing a firmware update](#flashing-a-firmware-update)
  - [Building your own device](#building-your-own-device)
  - [Antenna selection](#antenna-selection)
  - [3D Models](#3d-models)
  - [Firmware development](#firmware-development)
  - [App development](#app-development)
  - [FAQ](#faq)

---

## Description

The BLE Stepper Motor Analyzer ('the analyzer') is a low-cost, open source system that analyzes stepper motor signals and display the the data in real time on a computer screen. The system includes two components.

1. **The Stepper Motor Probe** ('the device'). This is a small electronic board that monitors the currents through the stepper motor wires, extracts information such as steps, and speed, and transmits the data via Bluetooth BLE.

2. **The Analyzer App** ('the app'). This is a Python program that runs on a Windows, MaC OSX, or Linux PC, and displays the stepper information in real time in a graphical view.

<p align="center">
 System Block diagram<br><br>
  <img src="./www/system_block_diagram.svg" style="width: 400px;" />
</p>

## Design Highlights
* The device is passive and doesn't not interfere with the operation of the stepper motor, regardless if the device is in use or turned off.
* Embedding the device in a 3D printer is simple. It has a small footprint, operates on 7-30 VDC, and uses a small external sticker antenna that overcomes shielding by the printer case.
* The design is in the public domain (Creative Commons CC0) and can be used commercially.
* The hardware design is low cost and devices can be ordered fully assembled from JLCPCB or similar services.
* The device firmware can be updated by users using a standard USB cable, no other tools are required.
* Each device has its own factory assigned address such that multiple devices can be used independently (e.g. one for each stepper motor in your printer).
* Open BLE and Python APIs allows to write custom monitoring apps, including for mobile devices.

## How does it work?
The device contains two galvanic isolated current sensors that sense the currents through the two stepper coils. These values are measured 40,000 times a second and analysis firmware on the device extracts information such as step count, and motion speed. A Bluetooth BLE radio then transmits the data to the app which displays it in a graphical form.

 <p align="center">
 Device Block diagram<br>
  <img src="./www/device_block_diagram.svg" style="width: 400px;" />
</p>



## Specification

Item | Specification
:------------ | :-------------
Power consumption | 7-30 VDC, 1W.
Firmware update connector | USB Type C
SOIC Module | ESP32-WROOM-32U-N4 
Current measurement | +/-2.5A per coil.
PCB | 39mm x 43mm, two layers
Antenna  | 2.4Ghz external antenna IPX IPEX connector.
Current sensors | CC6920BSO-5A
Zero calibration | Using onboard button.
Count direction | User selected, 
Sampling rate | 40Khz per channel.
Sampling resolution | 12bits
Current accuracy | estimated at +/- 2%
Max step rate | 5K full steps/sec.
Step resolution | 1/100th of a full step.
Firmware programming language | C++
Firmware programming IDE | VSCode, Platformio, esdidf framework.
Firmware debugging | Via optional 10 pins JTAG connector.
Electronic design tool | Kicad.
Mechanical design tool | Onshape.
Open source license | Creative Commons CC0.



## Installing the device in your 3D printer

1. 3D print a device carrier (STL models provided here) and attach the device to the carrier using two pieces of 3M VHB 1mm sticky tape or similar.
2. Attach the device carrier to the 3D printer using and additional piece of 3M VHB tape.
3. Connect the external antenna and attach the antenna to the outside of the printer.
4. Connect the motor driver and stepper motor to the two connectors as shown in the wiring diagram below. (It's OK to swap between the two stepper motor connectors, the device will work just the same).
5. Connect 7-30VDC power to the VIN connector of the board and pay attention to the polarity. (Yes, dhe device does contain reverse polarity protection). 
6. Leave the USB connector unconnected. The USB connector is not used during  normal operation of the device. It is intended for firmware flashing only.
7. Your device installation is complete. You can use your 3D printer just as you did before, regardless if the analyzer app is active or not.

&nbsp;

<p align="center">
  Wiring Diagram<br>
  <img src="./www/wiring_diagram.svg" style="height: 400px;"/>
</p>

## Using the Analyzer app.

### Running the app using a provided binary
The simplest way to run the analyzer app on your computer is to look for an  executable file for your system under the 'release' directory of this repository.  

> **_NOTE:_**  The single binary files are created from the app's Python code using the standard Pyinstaller tool. 

### Running the app using the Python code
If a single app binary is not available for your system, or if you prefer to run the Python code directly, you will first need to do a few preperation to satisfy the prerequisites of the Python code.

1. Install Python on your computer. Prefer the latest stable version.
2. Install the required python modules by running this command the 'app' directory:
```
pip install -r requirements.txt
```
3. Make sure that your analyzer device is powered on and start the app using the command

```
python analyzer.py
```
4. The app will start and try to find the device, and if found, will connect to it and display it's data on the screen. 

> **_NOTE:_**  When the device is connected to an app, it will not be visible to other apps that start. The device will be available to a new connection only after the previous connection terminated and its heatbeat LED blinks slowly again once a second.

### Understanding the app screen

<p align="center">
  <img src="./www/annotated_app_screenshot.jpg"  />
</p>

The apps screen contains the following graphs

* [A]. A rolling chat with distance in steps. Updated 50 times a second.
* [B]. A rolling chart with speed in steps/sec. Updated 50 times a second.
* [C]. A rolling chart with stepper absolute current in Amps. Updated 50 times a second. The absolute current of a stepper is sqrt(A^2 + B^2) where A and B are the momentary currents of coils A, B respectively.
* [D] A histogram of current vs speed, that shows how well the driver is able to maintain the stepper current at various speeds.
* [E] A histogram that shows the time spent in each speed range.
* [F] A histogram that shows the distance done in each speed range.
* [G] A phase (Lissajous curve) graph that show the cleanliness of the sine/cosine signals the driver sends to the motor.
* [H] An oscilloscope graph that shows current samples from the two coils. During stepper movement, they should look like sine/cosine waves. 

### App buttons
The app contain the button listed below. Note that the buttons affect the app only and do not interfere with the operation of the motor.

* **Toggle Dir** - Click to change the direction of step counting.
* **Reset Data** - Click to clear the histograms and step count.
* **Time Scale X** - Click to toggle through time ranges of the Phase and oscilloscope graphs [G] and [H]. Adjust it to have at least one full cycle.
* **Pause** - Click to pause/resume the display. Useful to capture and examine data.

### Hidden app functionality
The app is implemented using pyqggraph and as such provides additional functionality that is available by right-clicking the mouse over the graphs, or by interactive with the graphs using the mouse. For example, the right-click functionality can be used to export data or zoom the graphs.

### App command line flags

<i>--scan</i> The analyzer will scan for BLE devices for a few seconds, print a list with the devices it found and will exit. Note that this will print all BLE devices detected, not just analyzer's devices. You can identify analyzer devices by their names which look like <si>STP-0C8B95F2B436</i>.

```
python analyzer.py --scan
```


<i>--device</i> Instructs the analyzer to not search for available devices on startup and instead, connect to the given device directly.

```
python analyzer.py --device "STP-0C8B95F2B436"

python analyzer.py --device "0C:8B:95:F2:B4:36"
```

<i>--device_nick_name</i> Specify a user friendly name of the stepper motor being monitor. This name is displayed at the top of the analyzer's app. 
```
python analyzer.py --device_nick_name "X Axis"
```

<i>--max-amps</i> If you monitor a low current stepper motor such as a 600ma extruder motor, you can use this flag to limit the range of the current display in the Phase and Oscilloscope graphs.
```
python analyzer.py --max_amps 0.8
```

<i>--units</i> The default units for stepper distance or rotation are full steps. You can use this flag to display other units such as mm, degrees, or mm^3. 
```
python analyzer.py --units "mm"

python analyzer.py --units "degrees"

python analyzer.py --units "mm^3"
```

<i>--steps_per_unit</i> If you are using the <i>--units</i> flag, use this flag to specify the number of full motor steps that correspond to one unit. For example, if your extruder esteps is 540 micro steps per mm, with 16 microsteps per full step, specify the flags below:
```
python analyzer.py --units=mm --steps_per_unit 33.75
```




## Flashing a firmware update

**TBD** (using the esptool or ESP Web Tools, and the firmware provided here)

## Building your own device

1. Order assembled boards (recommended), or just order bare PCBs and assemble the components yourself. Production files for JLCPCB PCB and SMT service are included in this repository.
2. Connect the device to a computer via a USB cable and notice that one LED turn on solid to indicate 3.3V power.
3. Flash the device with firmware and make sure that the second LED blinks at 1 sec intervals to indicate the device is ready for connection from the app.
3. Calibrate the current sensors for zero level  by long pressing the switch  until the third LED will blink 3 times to indicate that the calibration values were saved in the non volatile memory.
4. Connect an antenna, start the analyzer app and verify that it connects successfully to the device. The LED on the device will blink faster to indicate an active connection.
5. The device is now ready for installation in the 3D printer or for shipping to your customers.

* **Schematic**: https://github.com/zapta/ble_stepper_motor_analyzer/blob/main/hardware/stepper_monitor.pdf
* **JLCPCB production files**: https://github.com/zapta/ble_stepper_motor_analyzer/tree/main/hardware/JLCPCB



> **_NOTE:_**  Resistor R16 acts as a jumper and allows to disconnect the 3.3V supply from the rest of the electronics. This is useful when testing a new batch of boards or when diagnosing faulty boards. 

## Antenna selection

The external antenna needs to satisfy these requirements:
* Specified by the Bluetooth band of 2.4GHZ.
* Having a connector that match the ESP32 WROOM module. The connector has several names including IPEX, U.FL, or MHF1, but it's easier to distinguish it by the wider body.

<p align="center">
  <img src="./www/connector.jpg" style="width: 200px;" />
</p>

## 3D Models

3D Models of the carrier and a cover for shipping are available at https://github.com/zapta/ble_stepper_motor_analyzer/tree/main/3D

<p align="center">
  <img src="./www/carrier.jpg" style="width: 200px;" />
</p>

## Firmware development

This list below outlines the steps to set up your own firmware development environment.

1. Install Visual Studio Code (VSC).
2. In Visual Studio Code, enable the <i>platformio</i> extension.
3. Download or clone this git repository on your computer.
4. In VSC, select <i>File | Open Folder</i> and select the 'firmware' directory.
5. Connect a device to your computer using a USB cable. This will create a serial port on your compute.
6. Click on the platformio <i>Upload</i> icon at the bottom of the screen (right arrow icon) and platformio will automatically install all the dependencies, build the project, and upload it to your device via the serial port.  
7. For more information on how to use platformio, check http://platformio.org.

> **_NOTE:_**  For optional hardware debugging (breakpoints, single step, etc), you will need an Espressif ESP-Prog Development Board, and to solder on your device a 10 pins JTAG connector.

> **_NOTE:_** It's OK to connect a stepper motor to the device while you develope code since the stepper signals are galvanically isolated from the USB interface.

> **_NOTE:_** It is recommended to not connect VIN and the USB connectors to the device at the same time to avoid ground loop, in case both the VIN power supply and your computer are grounded independently.



## App development

The Analyzer's Python app is stored in the 'app' directory. It's available as source code so can be modified like any other Python program.  The app is based on Bleak (BLE client) PyQtGraph (a graphing library) and uses asyncio to satisfy Bleak's requirements. The list of requires modules is available in requirements.txt and can be installed using the command

```
pip install -r requirements.txt
```

## FAQ

**Q**: This is an exciting project, can I help?

A: Of course. Course, we are looking forward for contribution of code, documentation and any other technical contribution from the community. For example, a cleanup of this document, or a new app for mobile devices, or anything else you  can think of.

---

**Q**: I want to monitor multiple motors in my 3D printer. Is it possible?

**A**: Of course. Simply install a a device for each stepper motor you want to monitor.

---

**Q**: I am using multiple devices, how can I select which one I connect to? 

**A**: Each devices has a factory set unique address that looks like <i>0C:8B:95:F2:B4:36</i>. When you run the analyzer
app, you can specify the address of the device you want to monitor by adding a command line flag such as <i>-d 0C:8B:95:F2:B4:36</i>

---

**Q**: How far can my computer be away from my 3D printer? 

**A**: Bluetooth BLE radio communication is intended for short distances of a few yards. If the radio link will not be reliable, the analyzer program will detect data gaps and will report it in its console log.

---

**Q**: This system can be a great idea for a Crowd Compute campaign. Can I do that? 

**A**: Of course. The design is in public domain and commercial usages are encouraged. Attribution and sharing any changes you make are not required. 

---

**Q**: Do you sell boards? 

**A**: We may make a limited numbers of boards available from time to time, but believe that others can do a better job mass producing it and making it available to the community.

---

**Q**: Can the external antenna be eliminated to simplify installation and reduce cost? 

**A**: Yes. We believe that the ESP module ESP32-WROOM-32D-N4 which uses an internal antenna should be a drop-in replacement though we did not tested it.

---
