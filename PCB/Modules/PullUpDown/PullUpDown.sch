EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 9
Title "KTANE Physical Edition - Pull Up/Down Module"
Date "2021-04-17"
Rev "1"
Comp "Designed by Melair"
Comment1 "Port module for control board, with pull up, pull down, tvs and optional output resistance."
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
L power:GND #PWR04
U 1 1 60A02DAA
P 950 1900
F 0 "#PWR04" H 950 1650 50  0001 C CNN
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
Entry Wire Line
	1800 1700 1700 1800
Entry Wire Line
	1800 1600 1700 1700
Entry Wire Line
	1800 1500 1700 1600
Entry Wire Line
	1800 1400 1700 1500
Entry Wire Line
	1800 1300 1700 1400
Entry Wire Line
	1800 1200 1700 1300
Entry Wire Line
	1800 1100 1700 1200
Entry Wire Line
	1800 1000 1700 1100
Wire Wire Line
	1700 1800 1550 1800
Text GLabel 1800 950  0    50   Input ~ 0
In
Text Label 1600 1100 0    50   ~ 0
I0
Text Label 1600 1200 0    50   ~ 0
I1
Text Label 1600 1300 0    50   ~ 0
I2
Text Label 1600 1400 0    50   ~ 0
I3
Text Label 1600 1500 0    50   ~ 0
I4
Text Label 1600 1600 0    50   ~ 0
I5
Text Label 1600 1700 0    50   ~ 0
I6
Text Label 1600 1800 0    50   ~ 0
I7
Wire Notes Line
	650  650  3300 650 
Text Notes 700  750  0    50   ~ 0
Module Connectors
Wire Wire Line
	1550 1400 1700 1400
Wire Wire Line
	1700 1500 1550 1500
Wire Wire Line
	1550 1600 1700 1600
Wire Wire Line
	1700 1100 1550 1100
Wire Wire Line
	1700 1300 1550 1300
Wire Wire Line
	1700 1700 1550 1700
Wire Wire Line
	1550 1200 1700 1200
$Comp
L Connector_Generic:Conn_02x08_Odd_Even J2
U 1 1 607ED442
P 1250 1500
F 0 "J2" H 1300 2017 50  0000 C CNN
F 1 "In" H 1300 1926 50  0000 C CNN
F 2 "Custom:PinHeader_2x08_P2.54mm_Vertical-Centered" H 1250 1500 50  0001 C CNN
F 3 "~" H 1250 1500 50  0001 C CNN
	1    1250 1500
	1    0    0    1   
$EndComp
Wire Notes Line
	650  2200 650  650 
Wire Notes Line
	3300 2200 650  2200
Wire Notes Line
	3300 650  3300 2200
Wire Wire Line
	2250 1500 2150 1500
Wire Wire Line
	2150 1500 2150 1600
Wire Wire Line
	2250 1600 2150 1600
Connection ~ 2150 1600
Wire Wire Line
	2150 1600 2150 1700
Wire Wire Line
	2250 1700 2150 1700
Connection ~ 2150 1700
Wire Wire Line
	2150 1700 2150 1800
Wire Wire Line
	2250 1800 2150 1800
Connection ~ 2150 1800
Wire Wire Line
	2150 1800 2150 1900
$Comp
L power:GND #PWR05
U 1 1 609D0B02
P 2150 1900
F 0 "#PWR05" H 2150 1650 50  0001 C CNN
F 1 "GND" H 2155 1727 50  0000 C CNN
F 2 "" H 2150 1900 50  0001 C CNN
F 3 "" H 2150 1900 50  0001 C CNN
	1    2150 1900
	-1   0    0    -1  
$EndComp
Wire Wire Line
	2250 1400 2150 1400
Wire Wire Line
	2150 1400 2150 1300
Wire Wire Line
	2250 1300 2150 1300
Connection ~ 2150 1300
Wire Wire Line
	2150 1300 2150 1200
Wire Wire Line
	2250 1200 2150 1200
Connection ~ 2150 1200
Wire Wire Line
	2150 1200 2150 1100
Wire Wire Line
	2250 1100 2150 1100
Connection ~ 2150 1100
Wire Wire Line
	2150 1100 2150 1000
$Comp
L power:+5V #PWR03
U 1 1 609D0B13
P 2150 1000
F 0 "#PWR03" H 2150 850 50  0001 C CNN
F 1 "+5V" H 2165 1173 50  0000 C CNN
F 2 "" H 2150 1000 50  0001 C CNN
F 3 "" H 2150 1000 50  0001 C CNN
	1    2150 1000
	-1   0    0    -1  
$EndComp
Entry Wire Line
	3000 1700 2900 1800
Entry Wire Line
	3000 1600 2900 1700
Entry Wire Line
	3000 1500 2900 1600
Entry Wire Line
	3000 1400 2900 1500
Entry Wire Line
	3000 1300 2900 1400
Entry Wire Line
	3000 1200 2900 1300
Entry Wire Line
	3000 1100 2900 1200
Entry Wire Line
	3000 1000 2900 1100
Wire Wire Line
	2900 1800 2750 1800
Text GLabel 3000 950  0    50   Input ~ 0
Out
Text Label 2800 1100 0    50   ~ 0
O0
Text Label 2800 1200 0    50   ~ 0
O1
Text Label 2800 1300 0    50   ~ 0
O2
Text Label 2800 1400 0    50   ~ 0
O3
Text Label 2800 1500 0    50   ~ 0
O4
Text Label 2800 1600 0    50   ~ 0
O5
Text Label 2800 1700 0    50   ~ 0
O6
Text Label 2800 1800 0    50   ~ 0
O7
Wire Wire Line
	2750 1400 2900 1400
Wire Wire Line
	2900 1500 2750 1500
Wire Wire Line
	2750 1600 2900 1600
Wire Wire Line
	2900 1100 2750 1100
Wire Wire Line
	2900 1300 2750 1300
Wire Wire Line
	2900 1700 2750 1700
Wire Wire Line
	2750 1200 2900 1200
$Comp
L Connector_Generic:Conn_02x08_Odd_Even J1
U 1 1 609D0B32
P 2450 1400
F 0 "J1" H 2500 1917 50  0000 C CNN
F 1 "In" H 2500 1826 50  0000 C CNN
F 2 "Custom:PinHeader_2x08_P2.54mm_Vertical-Centered" H 2450 1400 50  0001 C CNN
F 3 "~" H 2450 1400 50  0001 C CNN
	1    2450 1400
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR01
U 1 1 609EA3B7
P 4250 900
F 0 "#PWR01" H 4250 750 50  0001 C CNN
F 1 "+5V" H 4265 1073 50  0000 C CNN
F 2 "" H 4250 900 50  0001 C CNN
F 3 "" H 4250 900 50  0001 C CNN
	1    4250 900 
	-1   0    0    -1  
$EndComp
$Comp
L power:GND #PWR06
U 1 1 609EA98F
P 4250 1900
F 0 "#PWR06" H 4250 1650 50  0001 C CNN
F 1 "GND" H 4255 1727 50  0000 C CNN
F 2 "" H 4250 1900 50  0001 C CNN
F 3 "" H 4250 1900 50  0001 C CNN
	1    4250 1900
	-1   0    0    -1  
$EndComp
Wire Notes Line
	3450 650  3450 2200
Wire Notes Line
	3450 2200 5050 2200
Wire Notes Line
	5050 2200 5050 650 
Wire Notes Line
	5050 650  3450 650 
Text GLabel 3800 1400 0    50   Input ~ 0
O0
Text GLabel 3800 1500 0    50   Input ~ 0
O1
Text GLabel 3800 1700 0    50   Input ~ 0
O3
Text GLabel 4700 1400 2    50   Input ~ 0
O4
Text GLabel 4700 1500 2    50   Input ~ 0
O5
Text GLabel 4700 1600 2    50   Input ~ 0
O6
Text GLabel 4700 1700 2    50   Input ~ 0
O7
Text GLabel 3800 1600 0    50   Input ~ 0
O2
$Comp
L KTANE:SP720ABTG U1
U 1 1 609E8D3C
P 4250 1400
F 0 "U1" H 4050 2000 50  0000 C CNN
F 1 "SP720ABTG" H 4500 2000 50  0000 C CNN
F 2 "Package_SO:SOIC-16_3.9x9.9mm_P1.27mm" H 3300 2450 50  0001 C CNN
F 3 "" H 4150 2000 50  0001 C CNN
	1    4250 1400
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR07
U 1 1 60A04BEA
P 4900 1900
F 0 "#PWR07" H 4900 1650 50  0001 C CNN
F 1 "GND" H 4905 1727 50  0000 C CNN
F 2 "" H 4900 1900 50  0001 C CNN
F 3 "" H 4900 1900 50  0001 C CNN
	1    4900 1900
	-1   0    0    -1  
$EndComp
$Sheet
S 1000 2600 900  200 
U 60A09CDF
F0 "PullUpPullDown" 50
F1 "PullUpPullDown.sch" 50
F2 "Input" I L 1000 2700 50 
F3 "Output" I R 1900 2700 50 
$EndSheet
Text GLabel 900  2700 0    50   Input ~ 0
I0
Text GLabel 2000 2700 2    50   Input ~ 0
O0
Wire Wire Line
	900  2700 1000 2700
Wire Wire Line
	1900 2700 2000 2700
$Sheet
S 1000 3050 900  200 
U 60A5E738
F0 "sheet60A5E734" 50
F1 "PullUpPullDown.sch" 50
F2 "Input" I L 1000 3150 50 
F3 "Output" I R 1900 3150 50 
$EndSheet
Text GLabel 900  3150 0    50   Input ~ 0
I1
Text GLabel 2000 3150 2    50   Input ~ 0
O1
Wire Wire Line
	900  3150 1000 3150
Wire Wire Line
	1900 3150 2000 3150
$Sheet
S 1000 3500 900  200 
U 60A5F967
F0 "sheet60A5F963" 50
F1 "PullUpPullDown.sch" 50
F2 "Input" I L 1000 3600 50 
F3 "Output" I R 1900 3600 50 
$EndSheet
Text GLabel 900  3600 0    50   Input ~ 0
I2
Text GLabel 2000 3600 2    50   Input ~ 0
O2
Wire Wire Line
	900  3600 1000 3600
Wire Wire Line
	1900 3600 2000 3600
$Sheet
S 1000 3950 900  200 
U 60A60D5D
F0 "sheet60A60D59" 50
F1 "PullUpPullDown.sch" 50
F2 "Input" I L 1000 4050 50 
F3 "Output" I R 1900 4050 50 
$EndSheet
Text GLabel 900  4050 0    50   Input ~ 0
I3
Text GLabel 2000 4050 2    50   Input ~ 0
O3
Wire Wire Line
	900  4050 1000 4050
Wire Wire Line
	1900 4050 2000 4050
$Sheet
S 2550 2600 900  200 
U 60A62D0F
F0 "sheet60A62D08" 50
F1 "PullUpPullDown.sch" 50
F2 "Input" I L 2550 2700 50 
F3 "Output" I R 3450 2700 50 
$EndSheet
Text GLabel 2450 2700 0    50   Input ~ 0
I4
Text GLabel 3550 2700 2    50   Input ~ 0
O4
Wire Wire Line
	2450 2700 2550 2700
Wire Wire Line
	3450 2700 3550 2700
$Sheet
S 2550 3050 900  200 
U 60A62D17
F0 "sheet60A62D09" 50
F1 "PullUpPullDown.sch" 50
F2 "Input" I L 2550 3150 50 
F3 "Output" I R 3450 3150 50 
$EndSheet
Text GLabel 2450 3150 0    50   Input ~ 0
I5
Text GLabel 3550 3150 2    50   Input ~ 0
O5
Wire Wire Line
	2450 3150 2550 3150
Wire Wire Line
	3450 3150 3550 3150
$Sheet
S 2550 3500 900  200 
U 60A62D1F
F0 "sheet60A62D0A" 50
F1 "PullUpPullDown.sch" 50
F2 "Input" I L 2550 3600 50 
F3 "Output" I R 3450 3600 50 
$EndSheet
Text GLabel 2450 3600 0    50   Input ~ 0
I6
Text GLabel 3550 3600 2    50   Input ~ 0
O6
Wire Wire Line
	2450 3600 2550 3600
Wire Wire Line
	3450 3600 3550 3600
$Sheet
S 2550 3950 900  200 
U 60A62D27
F0 "sheet60A62D0B" 50
F1 "PullUpPullDown.sch" 50
F2 "Input" I L 2550 4050 50 
F3 "Output" I R 3450 4050 50 
$EndSheet
Text GLabel 2450 4050 0    50   Input ~ 0
I7
Text GLabel 3550 4050 2    50   Input ~ 0
O7
Wire Wire Line
	2450 4050 2550 4050
Wire Wire Line
	3450 4050 3550 4050
Wire Notes Line
	650  2350 3850 2350
Wire Notes Line
	3850 2350 3850 4350
Wire Notes Line
	3850 4350 650  4350
Wire Notes Line
	650  4350 650  2350
Text Notes 700  2450 0    50   ~ 0
IO Handling
Wire Wire Line
	4700 1300 4900 1300
Wire Wire Line
	4700 1200 4900 1200
Wire Wire Line
	4900 1200 4900 1300
Connection ~ 4900 1300
Wire Wire Line
	4700 1100 4900 1100
Wire Wire Line
	4900 1100 4900 1200
Connection ~ 4900 1200
Wire Wire Line
	4900 1300 4900 1900
Wire Wire Line
	3800 1100 3600 1100
Wire Wire Line
	3600 1300 3800 1300
Wire Wire Line
	3600 1100 3600 1200
Connection ~ 3600 1300
Wire Wire Line
	3600 1300 3600 1900
Wire Wire Line
	3800 1200 3600 1200
Connection ~ 3600 1200
Wire Wire Line
	3600 1200 3600 1300
$Comp
L power:GND #PWR0101
U 1 1 609C2EA4
P 3600 1900
F 0 "#PWR0101" H 3600 1650 50  0001 C CNN
F 1 "GND" H 3605 1727 50  0000 C CNN
F 2 "" H 3600 1900 50  0001 C CNN
F 3 "" H 3600 1900 50  0001 C CNN
	1    3600 1900
	-1   0    0    -1  
$EndComp
Wire Bus Line
	3000 950  3000 1700
Wire Bus Line
	1800 950  1800 1700
$EndSCHEMATC
