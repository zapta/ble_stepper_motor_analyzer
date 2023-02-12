# BLE STEPPER MOTOR ANALYZER

TODO: Add a picture of the device


<p align="center">
  <img src="./www/app_screenshot.jpg"  />
</p>

## Description

The BLE Stepper Motor Analyzer ('the analyzer') is a low-cost, open source system that analyzes stepper motor signals and display the the data in real time on a computer screen. The system includes two components.

1. The Stepper Motor Probe ('the device'). This is a small electronic board that monitors the currents through the stepper motor wires, extracts information such as steps, and speed, and transmits the data via Bluetooth BLE.

2. The Analyzer App ('the app'). This is a Python program that runs on a Windows, MaC OSX, or Linux PC, and displays the stepper information in real time in a graphical view.

<p align="center">
 System Block diagram<br><br>
  <img src="./www/system_block_diagram.svg" style="width: 400px;" />
</p>

## Product Highlights
* The device is passive and doesn't not interfere with the operation of the stepper, regardless if the device is in use or turned off.
* Embedding the device in a 3D printer is simple. It has a small footprint, operates on 7-30 VDC, and uses a small external sticker antenna.
* The entire design is in the public domain and can be used commercially with no attribution or open source requirements.
* The electronic board uses common and low cost components and can be ordered fully assembled from JLCPCB or similar services.
* The device firmware can be updated by users using a standard USB cable, no other tools are required.
* Each device has its own factory-set unique address such that multiple devices can be used independently to monitor multiple stepper motors.
* Open BLE and Python API allows to write custom monitoring apps, including on mobile devices.

## How does it work?
The device contains two galvanic isolated current sensors that sense the currents through each of the two stepper coils. These two values are measured 40,000 times a second and a firmware analyzes the signals and tracks information such as step count, and motion speed speed. A Bluetooth BLE radio is then transmits that data to the app once it connects to it, and the app displays the data in a graphical form.

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

## Building your own device

1. Order assembled boards (recommended), or just the just bare PCB and assembled the components yourself. Production files for JLCPCB PCB and SMT service are included here.
2. Connect the device to a computer via a USB cable and notice that one LED turn on solid to indicate 3.3V power.
3. Flash the firmware to the device and notice that the second LED blinks every second or so, This indicates that the firmware is running and that the device is not connected to the app.
3. Calibrate the zero level of the current sensor by pressing on the switch a few seconds until the third LED will blink 3 times, indicating that the zero calibration was saved.
4. Connect an antenna and run the analyzer app to verify that it can connect to the device.
5. The device is not ready for installation in the 3D printer.

> **_NOTE:_**  Resistor R16 acts as a jumper and allow the 3.3V supply from the rest of the electronics. This is useful when bringing up boards or diagnosing faulty boards. 

## Installing the device in your 3D printer

1. 3D print a device carrier (STL models provided here) and attach the device to the carrier using two pieces of 3M VHB 1mm sticky tape or similar.
2. Attach the device carrier to the 3D printer using and additional piece of 3M VHB tape.
3. Connect the external antenna connector and attach the antenna to the outside of the printer.
4. Connect the driver and motor wires per the wiring diagram below. The two connectors are pass-through such that the signals from the driver are connected internally to their respective motor signals. (It's OK to swap between the two stepper motor connectors, the device will work just the same).
5. Connect the power input of the device to a power source (7-30VDC). Pay attention to the polarity.  (The device contains reverse polarity protection). 
6. Leave the USB connector unconnected. It is not used in normal operation of the device and should be used for firmware flashing only.
7. Your device installation is complete. You can use your 3D printer just the same, regardless if you use the analyzer app.

&nbsp;

<p align="center">
  Wiring Diagram<br>
  <img src="./www/wiring_diagram.svg" style="height: 400px;"/>
</p>

## Using the Analyzer app.

### Running the app using a provided binary
To simplify the app installation, we prepare sometimes prepare stand alone binary files that includes the app and everything it needs, including the python system and module perquisite. These binary can be found under the 'release' directory.

> **_NOTE:_**  The single binary files are created using the standard Pyinstaller tool. 

### Running the app using the Python code
If a single app binary is not available for your system, you can run the Python code directly. It is stored under the 'app' directory to run it on your system follow these steps:

1. Install Python on your computer. Prefer the latest stable version.
2. Install necessary python modules by running within the 'app' directory:
```
pip install -r requirements.txt
```
3. Make sure that your analyzer device is power on and start the app using the command

```
python analyzer.py
```
4. The app will start, try to find the device, and if found, will connect to it and display it's graphical screen. On the device side, the heatbeat LED will blink faster to indicate that a connection is active.

> **_NOTE:_**  When the device is connected to an app, it will not be visible to other apps that start. The device will be available to a new connection only after the previous connection terminated and its heatbeat LED blinks slowly again once a second.

### Understanding the app screen

<p align="center">
  <img src="./www/annotated_app_screenshot.jpg"  />
</p>

The apps screen contains the following graphs

[A]. A scrolling graph of distance in steps. Updated 50 times a second.
[B]. A scrolling graph of speed in steps/sec. Updated 50 times a second.
[C]. A scrolling graph stepper absolute current in Amps. Updated 50 times a second. The absolute current of a stepper is sqrt(A^2 + B^2) where A and B are the momentary currents of coils A, B respectively.
[D] An histogram of current vs speed, that shows how well the driver is able to maintain the stepper current at various speeds.
[E] An histogram that shows the time spent in each speed range.
[F] An histogram that shows the distance done in each speed range.
[G] A phase graph that show the cleanliness of the sine/cosine signals the driver sends to the motor. 
[H] An oscilloscope like graph that shows current samples from the two coils. During stepper movement, they should look like sine/cosine waves. 

### App buttons
The app contain the button listed below. Note that the buttons affect the app only and do not interfere with the operation of the motor.

**Toggle Dir** - Click to change the direction of step counting.
**Reset Data** - Click to clear the histograms and step count.
**Time Scale X** - Click to toggle through time ranges of the Phase and oscilloscope graphs [G] and [H]. Adjust it to have at least one full cycle.
**Pause*** - Click to pause/resume the display. Useful to capture and examine data.

### Hidden app functionality
The app is implemented using pyqggraph and as such provides additional functionality that is available by right-clicking the mouse over the graphs. It can be used for example to export data, zoom the graphs in and out, and so on.

### App command line flags

<i>--scan</i> instead of connecting to the device, just look for available BLE devices and exit.

```
python analyzer.py --scan
```


<i>--device</i> do not look for devices to connect to and connect directly  to the device with the given address. 
```
python analyzer.py --device ???
```


## Flashing a firmware update

TBD (using the esptool or ESP Web Tools, and the firmware provided here)

## Firmware development

This list outlines the steps to set up your own firmware developement environment.

1. Install Visual Studio Code (VSC).
2. In Visual Studio Code, add the platformio extension.
3. Clone this git repository on your compute (download, clone, or first fork and then clone).
4. In VSC, select File | Open Folder and open the 'firmware' directory in this repository.
5. Connect a device to your computer using a USB cable. This will create a serial port on your compute.
6. Click on the <i>Upload</i> icon at the bottom of the screen (right arrow icon) and platformio will install all the dependencies, build the project and upload it to your device via the serial port.  
7. For more information on how to use platformio, check http://platformio.org.

> **_NOTE:_**  For optional hardware debugging (breakpoints, single step, etc), you will need an Espressif ESP-Prog Development Board, and to solder a 10 pins JTAG connector to your analyzer device.

> **_NOTE:_** It's OK to connect a stepper motor to the device while you debug it through a the USB connector since they current sensors are galvanically isolated from the electronic. However, it's safer not to connect at the same time both a USB cable and the Vin connector to avoid potential ground loop between the two.


## Analyzer app development

The Analyzer's Python app is stored in the 'app' directory. It's available as source code so can be modified like any other Python program.  The app is based on Bleak for BLE functionality and PyQtGraph for the user interface. The list of prerequisite modules is available in requirements.txt and you can install them using the command

```
pip install -r requirements.txt
```

## FAQ

Q: This is an exciting project, can I help?

A: Of course. Course, we are looking forward for contribution of code, documentation and any other technical contribution from the community. For example, a cleanup of this document, or a new app for mobile devices, or anything else you  can think of.

---

Q: I want to monitor multiple motors in my 3D printer. Is it possible?

A: Of course. Simply install a a device for each stepper motor you want to monitor.

---

Q: I am using multiple devices, how can I select which one I connect to? 

A: Each devices has a factory set unique address that looks like <i>0C:8B:95:F2:B4:36</i>. When you run the analyzer
app, you can specify the address of the device you want to monitor by adding a command line flag such as <i>-d 0C:8B:95:F2:B4:36</i>

---

Q: How far can my computer be away from my 3D printer? 

A: Bluetooth BLE radio communication is intended for short distances of a few yards. If the radio link will not be reliable, the analyzer program will detect data gaps and will report it in its console log.

---

Q: This system can be a great idea for a Crowd Compute campaign. Can I do that? 

A: Of course. The design is in public domain and commercial usages are encouraged. Attribution and sharing any changes you make are not required. 

---

Q: Do you sell boards? 

A: We may make a limited numbers of boards available from time to time, but believe that others can do a better job mass producing it and making it available to the community.

---

Q: Can the external antenna be eliminated to simplify installation and reduce cost? 

A: Yes. We believe that the ESP module ESP32-WROOM-32D-N4 which uses an internal antenna should be a drop-in replacement though we did not tested it.

---














