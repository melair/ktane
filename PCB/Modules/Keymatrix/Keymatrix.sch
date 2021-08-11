EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "KTANE Physical Edition - Keymatrix Module"
Date "2021-04-17"
Rev "1"
Comp "Designed by Melair"
Comment1 "4x4 Column to Row Keymatrix"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Wire Wire Line
	1050 1500 950  1500
Wire Wire Line
	950  1500 950  1600
Wire Wire Line
	1050 1600 950  1600
Connection ~ 950  1600
Wire Wire Line
	950  1600 950  1700
Wire Wire Line
	1050 1700 950  1700
Connection ~ 950  1700
Wire Wire Line
	950  1700 950  1800
Wire Wire Line
	1050 1800 950  1800
Connection ~ 950  1800
Wire Wire Line
	950  1800 950  1900
$Comp
L power:GND #PWR03
U 1 1 60A02DAA
P 950 1900
F 0 "#PWR03" H 950 1650 50  0001 C CNN
F 1 "GND" H 955 1727 50  0000 C CNN
F 2 "" H 950 1900 50  0001 C CNN
F 3 "" H 950 1900 50  0001 C CNN
	1    950  1900
	-1   0    0    -1  
$EndComp
Wire Wire Line
	1050 1400 950  1400
Wire Wire Line
	950  1400 950  1300
Wire Wire Line
	1050 1300 950  1300
Connection ~ 950  1300
Wire Wire Line
	950  1300 950  1200
Wire Wire Line
	1050 1200 950  1200
Connection ~ 950  1200
Wire Wire Line
	950  1200 950  1100
Wire Wire Line
	1050 1100 950  1100
Connection ~ 950  1100
Wire Wire Line
	950  1100 950  1000
$Comp
L power:+5V #PWR02
U 1 1 60A53DC0
P 950 1000
F 0 "#PWR02" H 950 850 50  0001 C CNN
F 1 "+5V" H 965 1173 50  0000 C CNN
F 2 "" H 950 1000 50  0001 C CNN
F 3 "" H 950 1000 50  0001 C CNN
	1    950  1000
	-1   0    0    -1  
$EndComp
Wire Notes Line
	650  650  2100 650 
Text Notes 700  750  0    50   ~ 0
Module Connectors
$Comp
L Connector_Generic:Conn_02x08_Odd_Even J3
U 1 1 607ED442
P 1250 1500
F 0 "J3" H 1300 2017 50  0000 C CNN
F 1 "In" H 1300 1926 50  0000 C CNN
F 2 "Custom:PinHeader_2x08_P2.54mm_Vertical-Centered" H 1250 1500 50  0001 C CNN
F 3 "~" H 1250 1500 50  0001 C CNN
	1    1250 1500
	1    0    0    1   
$EndComp
Wire Notes Line
	650  2200 650  650 
Wire Notes Line
	2100 2200 650  2200
Wire Notes Line
	2100 650  2100 2200
$Comp
L power:+5V #PWR01
U 1 1 609EA3B7
P 3200 900
F 0 "#PWR01" H 3200 750 50  0001 C CNN
F 1 "+5V" H 3215 1073 50  0000 C CNN
F 2 "" H 3200 900 50  0001 C CNN
F 3 "" H 3200 900 50  0001 C CNN
	1    3200 900 
	-1   0    0    -1  
$EndComp
$Comp
L power:GND #PWR05
U 1 1 609EA98F
P 3200 1900
F 0 "#PWR05" H 3200 1650 50  0001 C CNN
F 1 "GND" H 3205 1727 50  0000 C CNN
F 2 "" H 3200 1900 50  0001 C CNN
F 3 "" H 3200 1900 50  0001 C CNN
	1    3200 1900
	-1   0    0    -1  
$EndComp
Wire Notes Line
	2300 650  2300 2200
Wire Notes Line
	2300 2200 4000 2200
Wire Notes Line
	4000 2200 4000 650 
Wire Notes Line
	4000 650  2300 650 
$Comp
L KTANE:SP720ABTG U1
U 1 1 609E8D3C
P 3200 1400
F 0 "U1" H 3000 2000 50  0000 C CNN
F 1 "SP720ABTG" H 3450 2000 50  0000 C CNN
F 2 "Package_SO:SOIC-16_3.9x9.9mm_P1.27mm" H 2250 2450 50  0001 C CNN
F 3 "" H 3100 2000 50  0001 C CNN
	1    3200 1400
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR04
U 1 1 609C2EA4
P 2450 1900
F 0 "#PWR04" H 2450 1650 50  0001 C CNN
F 1 "GND" H 2455 1727 50  0000 C CNN
F 2 "" H 2450 1900 50  0001 C CNN
F 3 "" H 2450 1900 50  0001 C CNN
	1    2450 1900
	-1   0    0    -1  
$EndComp
Text Notes 2350 750  0    50   ~ 0
ESD Protection
Text GLabel 1650 1500 2    50   Input ~ 0
ROW0
Text GLabel 1650 1600 2    50   Input ~ 0
ROW1
Text GLabel 1650 1700 2    50   Input ~ 0
ROW2
Text GLabel 1650 1800 2    50   Input ~ 0
ROW3
Text GLabel 1650 1100 2    50   Input ~ 0
COL0
Text GLabel 1650 1200 2    50   Input ~ 0
COL1
Text GLabel 1650 1300 2    50   Input ~ 0
COL2
Text GLabel 1650 1400 2    50   Input ~ 0
COL3
$Comp
L Connector_Generic:Conn_02x08_Odd_Even J1
U 1 1 61152B26
P 5300 1500
F 0 "J1" H 5350 2017 50  0000 C CNN
F 1 "Matrix1" H 5350 1926 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x08_P2.54mm_Vertical" H 5300 1500 50  0001 C CNN
F 3 "~" H 5300 1500 50  0001 C CNN
	1    5300 1500
	1    0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_02x08_Odd_Even J2
U 1 1 6115417D
P 7050 1500
F 0 "J2" H 7100 2017 50  0000 C CNN
F 1 "Matrix2" H 7100 1926 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x08_P2.54mm_Vertical" H 7050 1500 50  0001 C CNN
F 3 "~" H 7050 1500 50  0001 C CNN
	1    7050 1500
	1    0    0    1   
$EndComp
Text GLabel 5700 1100 2    50   Input ~ 0
COL0
Text GLabel 5700 1200 2    50   Input ~ 0
COL0
Text GLabel 5700 1300 2    50   Input ~ 0
COL0
Text GLabel 5700 1400 2    50   Input ~ 0
COL0
Text GLabel 5700 1500 2    50   Input ~ 0
COL1
Text GLabel 5700 1600 2    50   Input ~ 0
COL1
Text GLabel 5700 1700 2    50   Input ~ 0
COL1
Text GLabel 5700 1800 2    50   Input ~ 0
COL1
Text GLabel 7450 1100 2    50   Input ~ 0
COL2
Text GLabel 7450 1200 2    50   Input ~ 0
COL2
Text GLabel 7450 1300 2    50   Input ~ 0
COL2
Text GLabel 7450 1400 2    50   Input ~ 0
COL2
Text GLabel 7450 1500 2    50   Input ~ 0
COL3
Text GLabel 7450 1600 2    50   Input ~ 0
COL3
Text GLabel 7450 1700 2    50   Input ~ 0
COL3
Text GLabel 7450 1800 2    50   Input ~ 0
COL3
Wire Wire Line
	5600 1100 5700 1100
Wire Wire Line
	5700 1200 5600 1200
Wire Wire Line
	5600 1300 5700 1300
Wire Wire Line
	5700 1400 5600 1400
Wire Wire Line
	5600 1500 5700 1500
Wire Wire Line
	5700 1600 5600 1600
Wire Wire Line
	5600 1700 5700 1700
Wire Wire Line
	5700 1800 5600 1800
Wire Wire Line
	7350 1800 7450 1800
Wire Wire Line
	7450 1700 7350 1700
Wire Wire Line
	7350 1600 7450 1600
Wire Wire Line
	7450 1500 7350 1500
Wire Wire Line
	7350 1400 7450 1400
Wire Wire Line
	7450 1300 7350 1300
Wire Wire Line
	7450 1200 7350 1200
Wire Wire Line
	7350 1100 7450 1100
$Comp
L Diode:1N4148 D1
U 1 1 61170E1E
P 4850 1100
F 0 "D1" H 4850 1317 50  0000 C CNN
F 1 "1N4148" H 4850 1226 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 4850 925 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 4850 1100 50  0001 C CNN
	1    4850 1100
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D3
U 1 1 61171769
P 4850 1200
F 0 "D3" H 4850 1417 50  0000 C CNN
F 1 "1N4148" H 4850 1326 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 4850 1025 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 4850 1200 50  0001 C CNN
	1    4850 1200
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D5
U 1 1 61171B52
P 4850 1300
F 0 "D5" H 4850 1517 50  0000 C CNN
F 1 "1N4148" H 4850 1426 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 4850 1125 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 4850 1300 50  0001 C CNN
	1    4850 1300
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D7
U 1 1 61171FAD
P 4850 1400
F 0 "D7" H 4850 1617 50  0000 C CNN
F 1 "1N4148" H 4850 1526 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 4850 1225 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 4850 1400 50  0001 C CNN
	1    4850 1400
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D9
U 1 1 6117227E
P 4850 1500
F 0 "D9" H 4850 1717 50  0000 C CNN
F 1 "1N4148" H 4850 1626 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 4850 1325 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 4850 1500 50  0001 C CNN
	1    4850 1500
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D11
U 1 1 6117258D
P 4850 1600
F 0 "D11" H 4850 1817 50  0000 C CNN
F 1 "1N4148" H 4850 1726 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 4850 1425 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 4850 1600 50  0001 C CNN
	1    4850 1600
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D13
U 1 1 61172C8D
P 4850 1700
F 0 "D13" H 4850 1917 50  0000 C CNN
F 1 "1N4148" H 4850 1826 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 4850 1525 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 4850 1700 50  0001 C CNN
	1    4850 1700
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D15
U 1 1 611730AA
P 4850 1800
F 0 "D15" H 4850 2017 50  0000 C CNN
F 1 "1N4148" H 4850 1926 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 4850 1625 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 4850 1800 50  0001 C CNN
	1    4850 1800
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D2
U 1 1 6117349D
P 6600 1100
F 0 "D2" H 6600 1317 50  0000 C CNN
F 1 "1N4148" H 6600 1226 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 6600 925 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 6600 1100 50  0001 C CNN
	1    6600 1100
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D4
U 1 1 61174093
P 6600 1200
F 0 "D4" H 6600 1417 50  0000 C CNN
F 1 "1N4148" H 6600 1326 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 6600 1025 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 6600 1200 50  0001 C CNN
	1    6600 1200
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D6
U 1 1 6117435A
P 6600 1300
F 0 "D6" H 6600 1517 50  0000 C CNN
F 1 "1N4148" H 6600 1426 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 6600 1125 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 6600 1300 50  0001 C CNN
	1    6600 1300
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D8
U 1 1 611746DB
P 6600 1400
F 0 "D8" H 6600 1617 50  0000 C CNN
F 1 "1N4148" H 6600 1526 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 6600 1225 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 6600 1400 50  0001 C CNN
	1    6600 1400
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D10
U 1 1 611753D0
P 6600 1500
F 0 "D10" H 6600 1717 50  0000 C CNN
F 1 "1N4148" H 6600 1626 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 6600 1325 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 6600 1500 50  0001 C CNN
	1    6600 1500
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D12
U 1 1 61175939
P 6600 1600
F 0 "D12" H 6600 1817 50  0000 C CNN
F 1 "1N4148" H 6600 1726 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 6600 1425 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 6600 1600 50  0001 C CNN
	1    6600 1600
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D14
U 1 1 61175DA3
P 6600 1700
F 0 "D14" H 6600 1917 50  0000 C CNN
F 1 "1N4148" H 6600 1826 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 6600 1525 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 6600 1700 50  0001 C CNN
	1    6600 1700
	1    0    0    -1  
$EndComp
$Comp
L Diode:1N4148 D16
U 1 1 6117638D
P 6600 1800
F 0 "D16" H 6600 2017 50  0000 C CNN
F 1 "1N4148" H 6600 1926 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123F" H 6600 1625 50  0001 C CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/1N4148_1N4448.pdf" H 6600 1800 50  0001 C CNN
	1    6600 1800
	1    0    0    -1  
$EndComp
Wire Wire Line
	5000 1100 5100 1100
Wire Wire Line
	5100 1200 5000 1200
Wire Wire Line
	5000 1300 5100 1300
Wire Wire Line
	5100 1400 5000 1400
Wire Wire Line
	5000 1500 5100 1500
Wire Wire Line
	5000 1600 5100 1600
Wire Wire Line
	5000 1700 5100 1700
Wire Wire Line
	5000 1800 5100 1800
Wire Wire Line
	6750 1100 6850 1100
Wire Wire Line
	6750 1200 6850 1200
Wire Wire Line
	6750 1300 6850 1300
Wire Wire Line
	6750 1400 6850 1400
Wire Wire Line
	6750 1500 6850 1500
Wire Wire Line
	6750 1600 6850 1600
Wire Wire Line
	6750 1700 6850 1700
Wire Wire Line
	6750 1800 6850 1800
Text GLabel 4600 1100 0    50   Input ~ 0
ROW0
Text GLabel 4600 1500 0    50   Input ~ 0
ROW0
Text GLabel 6350 1100 0    50   Input ~ 0
ROW0
Text GLabel 6350 1500 0    50   Input ~ 0
ROW0
Text GLabel 4600 1200 0    50   Input ~ 0
ROW1
Text GLabel 4600 1600 0    50   Input ~ 0
ROW1
Text GLabel 6350 1200 0    50   Input ~ 0
ROW1
Text GLabel 6350 1600 0    50   Input ~ 0
ROW1
Text GLabel 4600 1300 0    50   Input ~ 0
ROW2
Text GLabel 4600 1700 0    50   Input ~ 0
ROW2
Text GLabel 6350 1300 0    50   Input ~ 0
ROW2
Text GLabel 6350 1700 0    50   Input ~ 0
ROW2
Text GLabel 4600 1400 0    50   Input ~ 0
ROW3
Text GLabel 4600 1800 0    50   Input ~ 0
ROW3
Text GLabel 6350 1800 0    50   Input ~ 0
ROW3
Text GLabel 6350 1400 0    50   Input ~ 0
ROW3
Wire Wire Line
	6350 1100 6450 1100
Wire Wire Line
	6450 1200 6350 1200
Wire Wire Line
	6350 1300 6450 1300
Wire Wire Line
	6450 1400 6350 1400
Wire Wire Line
	6350 1500 6450 1500
Wire Wire Line
	6450 1600 6350 1600
Wire Wire Line
	6350 1700 6450 1700
Wire Wire Line
	6350 1800 6450 1800
Wire Wire Line
	4600 1100 4700 1100
Wire Wire Line
	4600 1200 4700 1200
Wire Wire Line
	4600 1300 4700 1300
Wire Wire Line
	4600 1400 4700 1400
Wire Wire Line
	4600 1500 4700 1500
Wire Wire Line
	4600 1600 4700 1600
Wire Wire Line
	4600 1700 4700 1700
Wire Wire Line
	4600 1800 4700 1800
Wire Notes Line
	4200 650  4200 2200
Wire Notes Line
	4200 2200 7850 2200
Wire Notes Line
	7850 2200 7850 650 
Wire Notes Line
	7850 650  4200 650 
Text Notes 4250 750  0    50   ~ 0
Keyboard Matrix
Wire Wire Line
	1550 1100 1650 1100
Wire Wire Line
	1650 1200 1550 1200
Wire Wire Line
	1650 1300 1550 1300
Wire Wire Line
	1550 1400 1650 1400
Wire Wire Line
	1650 1500 1550 1500
Wire Wire Line
	1550 1600 1650 1600
Wire Wire Line
	1550 1700 1650 1700
Wire Wire Line
	1550 1800 1650 1800
Text GLabel 2750 1400 0    50   Input ~ 0
ROW0
Text GLabel 2750 1500 0    50   Input ~ 0
ROW1
Text GLabel 2750 1600 0    50   Input ~ 0
ROW2
Text GLabel 2750 1700 0    50   Input ~ 0
ROW3
Text GLabel 3650 1100 2    50   Input ~ 0
COL0
Text GLabel 3650 1200 2    50   Input ~ 0
COL1
Text GLabel 3650 1300 2    50   Input ~ 0
COL2
Text GLabel 3650 1400 2    50   Input ~ 0
COL3
$Comp
L power:GND #PWR0101
U 1 1 6128F654
P 3850 1850
F 0 "#PWR0101" H 3850 1600 50  0001 C CNN
F 1 "GND" H 3855 1677 50  0000 C CNN
F 2 "" H 3850 1850 50  0001 C CNN
F 3 "" H 3850 1850 50  0001 C CNN
	1    3850 1850
	-1   0    0    -1  
$EndComp
Wire Wire Line
	3650 1700 3850 1700
Wire Wire Line
	3850 1700 3850 1850
Wire Wire Line
	3650 1600 3850 1600
Wire Wire Line
	3850 1600 3850 1700
Connection ~ 3850 1700
Wire Wire Line
	3650 1500 3850 1500
Wire Wire Line
	3850 1500 3850 1600
Connection ~ 3850 1600
Wire Wire Line
	2450 1900 2450 1300
Wire Wire Line
	2450 1300 2750 1300
Wire Wire Line
	2450 1300 2450 1200
Wire Wire Line
	2450 1200 2750 1200
Connection ~ 2450 1300
Wire Wire Line
	2450 1200 2450 1100
Wire Wire Line
	2450 1100 2750 1100
Connection ~ 2450 1200
$EndSCHEMATC
