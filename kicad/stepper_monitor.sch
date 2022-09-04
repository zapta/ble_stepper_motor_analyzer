EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L RF_Module:ESP32-WROOM-32 U1
U 1 1 6313F256
P 5575 3525
F 0 "U1" H 5725 5075 50  0000 C CNN
F 1 "ESP32-WROOM-32" H 6025 4975 50  0000 C CNN
F 2 "RF_Module:ESP32-WROOM-32" H 5575 2025 50  0001 C CNN
F 3 "https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf" H 5275 3575 50  0001 C CNN
	1    5575 3525
	1    0    0    -1  
$EndComp
$Comp
L power:GNDD #PWR?
U 1 1 631407E6
P 4650 5150
F 0 "#PWR?" H 4650 4900 50  0001 C CNN
F 1 "GNDD" H 4654 4995 50  0000 C CNN
F 2 "" H 4650 5150 50  0001 C CNN
F 3 "" H 4650 5150 50  0001 C CNN
	1    4650 5150
	1    0    0    -1  
$EndComp
$Comp
L power:+3V3 #PWR?
U 1 1 63140DE6
P 5225 1800
F 0 "#PWR?" H 5225 1650 50  0001 C CNN
F 1 "+3V3" H 5240 1973 50  0000 C CNN
F 2 "" H 5225 1800 50  0001 C CNN
F 3 "" H 5225 1800 50  0001 C CNN
	1    5225 1800
	1    0    0    -1  
$EndComp
$EndSCHEMATC
