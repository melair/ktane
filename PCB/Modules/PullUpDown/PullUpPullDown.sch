EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 5 9
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text HLabel 1000 1600 0    50   Input ~ 0
Input
Text HLabel 3250 1600 2    50   Input ~ 0
Output
$Comp
L Device:Q_PMOS_GSD Q1
U 1 1 60A26012
P 2100 1350
AR Path="/60A09CDF/60A26012" Ref="Q1"  Part="1" 
AR Path="/60A5E738/60A26012" Ref="Q3"  Part="1" 
AR Path="/60A5F967/60A26012" Ref="Q5"  Part="1" 
AR Path="/60A60D5D/60A26012" Ref="Q7"  Part="1" 
AR Path="/60A62D0F/60A26012" Ref="Q9"  Part="1" 
AR Path="/60A62D17/60A26012" Ref="Q11"  Part="1" 
AR Path="/60A62D1F/60A26012" Ref="Q13"  Part="1" 
AR Path="/60A62D27/60A26012" Ref="Q15"  Part="1" 
F 0 "Q7" H 2305 1396 50  0000 L CNN
F 1 "Q_PMOS_GSD" H 2305 1305 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 2300 1450 50  0001 C CNN
F 3 "~" H 2100 1350 50  0001 C CNN
	1    2100 1350
	1    0    0    1   
$EndComp
$Comp
L Device:Q_NMOS_GSD Q2
U 1 1 60A27C3F
P 2100 1850
AR Path="/60A09CDF/60A27C3F" Ref="Q2"  Part="1" 
AR Path="/60A5E738/60A27C3F" Ref="Q4"  Part="1" 
AR Path="/60A5F967/60A27C3F" Ref="Q6"  Part="1" 
AR Path="/60A60D5D/60A27C3F" Ref="Q8"  Part="1" 
AR Path="/60A62D0F/60A27C3F" Ref="Q10"  Part="1" 
AR Path="/60A62D17/60A27C3F" Ref="Q12"  Part="1" 
AR Path="/60A62D1F/60A27C3F" Ref="Q14"  Part="1" 
AR Path="/60A62D27/60A27C3F" Ref="Q16"  Part="1" 
F 0 "Q8" H 2304 1896 50  0000 L CNN
F 1 "Q_NMOS_GSD" H 2304 1805 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 2300 1950 50  0001 C CNN
F 3 "~" H 2100 1850 50  0001 C CNN
	1    2100 1850
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR09
U 1 1 60A28B1A
P 2200 1100
AR Path="/60A09CDF/60A28B1A" Ref="#PWR09"  Part="1" 
AR Path="/60A5E738/60A28B1A" Ref="#PWR013"  Part="1" 
AR Path="/60A5F967/60A28B1A" Ref="#PWR017"  Part="1" 
AR Path="/60A60D5D/60A28B1A" Ref="#PWR021"  Part="1" 
AR Path="/60A62D0F/60A28B1A" Ref="#PWR025"  Part="1" 
AR Path="/60A62D17/60A28B1A" Ref="#PWR029"  Part="1" 
AR Path="/60A62D1F/60A28B1A" Ref="#PWR033"  Part="1" 
AR Path="/60A62D27/60A28B1A" Ref="#PWR037"  Part="1" 
F 0 "#PWR021" H 2200 950 50  0001 C CNN
F 1 "+5V" H 2215 1273 50  0000 C CNN
F 2 "" H 2200 1100 50  0001 C CNN
F 3 "" H 2200 1100 50  0001 C CNN
	1    2200 1100
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR08
U 1 1 60A2934C
P 1100 1100
AR Path="/60A09CDF/60A2934C" Ref="#PWR08"  Part="1" 
AR Path="/60A5E738/60A2934C" Ref="#PWR012"  Part="1" 
AR Path="/60A5F967/60A2934C" Ref="#PWR016"  Part="1" 
AR Path="/60A60D5D/60A2934C" Ref="#PWR020"  Part="1" 
AR Path="/60A62D0F/60A2934C" Ref="#PWR024"  Part="1" 
AR Path="/60A62D17/60A2934C" Ref="#PWR028"  Part="1" 
AR Path="/60A62D1F/60A2934C" Ref="#PWR032"  Part="1" 
AR Path="/60A62D27/60A2934C" Ref="#PWR036"  Part="1" 
F 0 "#PWR020" H 1100 950 50  0001 C CNN
F 1 "+5V" H 1115 1273 50  0000 C CNN
F 2 "" H 1100 1100 50  0001 C CNN
F 3 "" H 1100 1100 50  0001 C CNN
	1    1100 1100
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR011
U 1 1 60A29802
P 2200 2100
AR Path="/60A09CDF/60A29802" Ref="#PWR011"  Part="1" 
AR Path="/60A5E738/60A29802" Ref="#PWR015"  Part="1" 
AR Path="/60A5F967/60A29802" Ref="#PWR019"  Part="1" 
AR Path="/60A60D5D/60A29802" Ref="#PWR023"  Part="1" 
AR Path="/60A62D0F/60A29802" Ref="#PWR027"  Part="1" 
AR Path="/60A62D17/60A29802" Ref="#PWR031"  Part="1" 
AR Path="/60A62D1F/60A29802" Ref="#PWR035"  Part="1" 
AR Path="/60A62D27/60A29802" Ref="#PWR039"  Part="1" 
F 0 "#PWR023" H 2200 1850 50  0001 C CNN
F 1 "GND" H 2205 1927 50  0000 C CNN
F 2 "" H 2200 2100 50  0001 C CNN
F 3 "" H 2200 2100 50  0001 C CNN
	1    2200 2100
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR010
U 1 1 60A29FF8
P 1100 2100
AR Path="/60A09CDF/60A29FF8" Ref="#PWR010"  Part="1" 
AR Path="/60A5E738/60A29FF8" Ref="#PWR014"  Part="1" 
AR Path="/60A5F967/60A29FF8" Ref="#PWR018"  Part="1" 
AR Path="/60A60D5D/60A29FF8" Ref="#PWR022"  Part="1" 
AR Path="/60A62D0F/60A29FF8" Ref="#PWR026"  Part="1" 
AR Path="/60A62D17/60A29FF8" Ref="#PWR030"  Part="1" 
AR Path="/60A62D1F/60A29FF8" Ref="#PWR034"  Part="1" 
AR Path="/60A62D27/60A29FF8" Ref="#PWR038"  Part="1" 
F 0 "#PWR022" H 1100 1850 50  0001 C CNN
F 1 "GND" H 1105 1927 50  0000 C CNN
F 2 "" H 1100 2100 50  0001 C CNN
F 3 "" H 1100 2100 50  0001 C CNN
	1    1100 2100
	1    0    0    -1  
$EndComp
$Comp
L Device:R_Small R5
U 1 1 60A438CD
P 2200 2600
AR Path="/60A09CDF/60A438CD" Ref="R5"  Part="1" 
AR Path="/60A5E738/60A438CD" Ref="R10"  Part="1" 
AR Path="/60A5F967/60A438CD" Ref="R15"  Part="1" 
AR Path="/60A60D5D/60A438CD" Ref="R20"  Part="1" 
AR Path="/60A62D0F/60A438CD" Ref="R25"  Part="1" 
AR Path="/60A62D17/60A438CD" Ref="R30"  Part="1" 
AR Path="/60A62D1F/60A438CD" Ref="R35"  Part="1" 
AR Path="/60A62D27/60A438CD" Ref="R40"  Part="1" 
F 0 "R20" V 2004 2600 50  0000 C CNN
F 1 "0" V 2095 2600 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" H 2200 2600 50  0001 C CNN
F 3 "~" H 2200 2600 50  0001 C CNN
	1    2200 2600
	0    1    1    0   
$EndComp
$Comp
L Device:R_Small R1
U 1 1 60A4449E
P 1100 1450
AR Path="/60A09CDF/60A4449E" Ref="R1"  Part="1" 
AR Path="/60A5E738/60A4449E" Ref="R6"  Part="1" 
AR Path="/60A5F967/60A4449E" Ref="R11"  Part="1" 
AR Path="/60A60D5D/60A4449E" Ref="R16"  Part="1" 
AR Path="/60A62D0F/60A4449E" Ref="R21"  Part="1" 
AR Path="/60A62D17/60A4449E" Ref="R26"  Part="1" 
AR Path="/60A62D1F/60A4449E" Ref="R31"  Part="1" 
AR Path="/60A62D27/60A4449E" Ref="R36"  Part="1" 
F 0 "R16" H 1041 1404 50  0000 R CNN
F 1 "10k" H 1041 1495 50  0000 R CNN
F 2 "Resistor_SMD:R_0805_2012Metric" H 1100 1450 50  0001 C CNN
F 3 "~" H 1100 1450 50  0001 C CNN
	1    1100 1450
	-1   0    0    1   
$EndComp
$Comp
L Device:R_Small R4
U 1 1 60A449EE
P 1100 1750
AR Path="/60A09CDF/60A449EE" Ref="R4"  Part="1" 
AR Path="/60A5E738/60A449EE" Ref="R9"  Part="1" 
AR Path="/60A5F967/60A449EE" Ref="R14"  Part="1" 
AR Path="/60A60D5D/60A449EE" Ref="R19"  Part="1" 
AR Path="/60A62D0F/60A449EE" Ref="R24"  Part="1" 
AR Path="/60A62D17/60A449EE" Ref="R29"  Part="1" 
AR Path="/60A62D1F/60A449EE" Ref="R34"  Part="1" 
AR Path="/60A62D27/60A449EE" Ref="R39"  Part="1" 
F 0 "R19" H 1041 1704 50  0000 R CNN
F 1 "10k" H 1041 1795 50  0000 R CNN
F 2 "Resistor_SMD:R_0805_2012Metric" H 1100 1750 50  0001 C CNN
F 3 "~" H 1100 1750 50  0001 C CNN
	1    1100 1750
	-1   0    0    1   
$EndComp
Wire Wire Line
	1000 1600 1100 1600
Wire Wire Line
	1100 1600 1100 1550
Wire Wire Line
	1100 1600 1100 1650
Connection ~ 1100 1600
Wire Wire Line
	1100 1850 1100 2100
Wire Wire Line
	1100 1350 1100 1100
Wire Wire Line
	2200 1100 2200 1150
Wire Wire Line
	2200 2100 2200 2050
Wire Wire Line
	2950 1600 2700 1600
Wire Wire Line
	2200 1600 2200 1550
Wire Wire Line
	2200 1650 2200 1600
Connection ~ 2200 1600
Wire Wire Line
	1900 1350 1850 1350
Wire Wire Line
	1850 1350 1850 1600
Wire Wire Line
	1850 1850 1900 1850
Wire Wire Line
	1850 1600 1700 1600
Connection ~ 1850 1600
Wire Wire Line
	1850 1600 1850 1850
$Comp
L Device:R_Small R2
U 1 1 60A49FB6
P 1600 1600
AR Path="/60A09CDF/60A49FB6" Ref="R2"  Part="1" 
AR Path="/60A5E738/60A49FB6" Ref="R7"  Part="1" 
AR Path="/60A5F967/60A49FB6" Ref="R12"  Part="1" 
AR Path="/60A60D5D/60A49FB6" Ref="R17"  Part="1" 
AR Path="/60A62D0F/60A49FB6" Ref="R22"  Part="1" 
AR Path="/60A62D17/60A49FB6" Ref="R27"  Part="1" 
AR Path="/60A62D1F/60A49FB6" Ref="R32"  Part="1" 
AR Path="/60A62D27/60A49FB6" Ref="R37"  Part="1" 
F 0 "R17" V 1796 1600 50  0000 C CNN
F 1 "100" V 1705 1600 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" H 1600 1600 50  0001 C CNN
F 3 "~" H 1600 1600 50  0001 C CNN
	1    1600 1600
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1500 1600 1350 1600
Wire Wire Line
	2300 2600 2700 2600
Wire Wire Line
	2700 2600 2700 1600
Connection ~ 2700 1600
Wire Wire Line
	2700 1600 2200 1600
Wire Wire Line
	2100 2600 1350 2600
Wire Wire Line
	1350 2600 1350 1600
Connection ~ 1350 1600
Wire Wire Line
	1350 1600 1100 1600
Wire Notes Line
	1000 1550 1000 850 
Wire Notes Line
	1000 850  2850 850 
Wire Notes Line
	2850 850  2850 1550
Wire Notes Line
	2850 1550 1000 1550
Wire Notes Line
	2850 1650 1000 1650
Wire Notes Line
	1000 1650 1000 2350
Wire Notes Line
	1000 2350 2850 2350
Wire Notes Line
	2850 2350 2850 1650
Wire Notes Line
	1000 2400 2850 2400
Wire Notes Line
	2850 2400 2850 2700
Wire Notes Line
	2850 2700 1000 2700
Wire Notes Line
	1000 2700 1000 2400
Wire Notes Line
	1400 1300 1400 1750
Wire Notes Line
	1400 1750 1800 1750
Wire Notes Line
	1800 1750 1800 1300
Wire Notes Line
	1800 1300 1400 1300
Text Notes 1400 950  0    50   ~ 0
High Side Switch
Text Notes 1400 2300 0    50   ~ 0
Low Side Switch
Text Notes 1400 1250 0    50   ~ 0
Common
Text Notes 1050 2500 0    50   ~ 0
Bypass Or Voltage Divider R1
$Comp
L Device:R_Small R3
U 1 1 609A97B3
P 3050 1600
AR Path="/60A09CDF/609A97B3" Ref="R3"  Part="1" 
AR Path="/60A5E738/609A97B3" Ref="R8"  Part="1" 
AR Path="/60A5F967/609A97B3" Ref="R13"  Part="1" 
AR Path="/60A60D5D/609A97B3" Ref="R18"  Part="1" 
AR Path="/60A62D0F/609A97B3" Ref="R23"  Part="1" 
AR Path="/60A62D17/609A97B3" Ref="R28"  Part="1" 
AR Path="/60A62D1F/609A97B3" Ref="R33"  Part="1" 
AR Path="/60A62D27/609A97B3" Ref="R38"  Part="1" 
F 0 "R18" V 2854 1600 50  0000 C CNN
F 1 "0" V 2945 1600 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" H 3050 1600 50  0001 C CNN
F 3 "~" H 3050 1600 50  0001 C CNN
	1    3050 1600
	0    1    1    0   
$EndComp
Wire Wire Line
	3250 1600 3150 1600
Wire Notes Line
	2900 1250 2900 1800
Wire Notes Line
	2900 1800 3200 1800
Wire Notes Line
	3200 1800 3200 1250
Wire Notes Line
	3200 1250 2900 1250
Text Notes 2950 1350 0    50   ~ 0
Limit
$EndSCHEMATC
