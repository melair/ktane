#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS

else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=modes/blank/blank.c modes/bootstrap/bootstrap.c modes/controller/controller.c peripherals/timer/segment.c peripherals/keymatrix.c peripherals/ports.c peripherals/pwmled.c main.c nvm.c argb.c buzzer.c can.c tick.c mode.c lcd.c protocol.c firmware.c protocol_module.c protocol_firmware.c status.c protocol_game.c interrupt.c module.c game.c rng.c edgework.c modes/needy.c modes/debug/debug.c modes/maze/maze.c modes/password/password.c modes/whosonfirst/whosonfirst.c modes/wires/wires.c modes/keys/keys.c modes/simon/simon.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/modes/blank/blank.p1 ${OBJECTDIR}/modes/bootstrap/bootstrap.p1 ${OBJECTDIR}/modes/controller/controller.p1 ${OBJECTDIR}/peripherals/timer/segment.p1 ${OBJECTDIR}/peripherals/keymatrix.p1 ${OBJECTDIR}/peripherals/ports.p1 ${OBJECTDIR}/peripherals/pwmled.p1 ${OBJECTDIR}/main.p1 ${OBJECTDIR}/nvm.p1 ${OBJECTDIR}/argb.p1 ${OBJECTDIR}/buzzer.p1 ${OBJECTDIR}/can.p1 ${OBJECTDIR}/tick.p1 ${OBJECTDIR}/mode.p1 ${OBJECTDIR}/lcd.p1 ${OBJECTDIR}/protocol.p1 ${OBJECTDIR}/firmware.p1 ${OBJECTDIR}/protocol_module.p1 ${OBJECTDIR}/protocol_firmware.p1 ${OBJECTDIR}/status.p1 ${OBJECTDIR}/protocol_game.p1 ${OBJECTDIR}/interrupt.p1 ${OBJECTDIR}/module.p1 ${OBJECTDIR}/game.p1 ${OBJECTDIR}/rng.p1 ${OBJECTDIR}/edgework.p1 ${OBJECTDIR}/modes/needy.p1 ${OBJECTDIR}/modes/debug/debug.p1 ${OBJECTDIR}/modes/maze/maze.p1 ${OBJECTDIR}/modes/password/password.p1 ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1 ${OBJECTDIR}/modes/wires/wires.p1 ${OBJECTDIR}/modes/keys/keys.p1 ${OBJECTDIR}/modes/simon/simon.p1
POSSIBLE_DEPFILES=${OBJECTDIR}/modes/blank/blank.p1.d ${OBJECTDIR}/modes/bootstrap/bootstrap.p1.d ${OBJECTDIR}/modes/controller/controller.p1.d ${OBJECTDIR}/peripherals/timer/segment.p1.d ${OBJECTDIR}/peripherals/keymatrix.p1.d ${OBJECTDIR}/peripherals/ports.p1.d ${OBJECTDIR}/peripherals/pwmled.p1.d ${OBJECTDIR}/main.p1.d ${OBJECTDIR}/nvm.p1.d ${OBJECTDIR}/argb.p1.d ${OBJECTDIR}/buzzer.p1.d ${OBJECTDIR}/can.p1.d ${OBJECTDIR}/tick.p1.d ${OBJECTDIR}/mode.p1.d ${OBJECTDIR}/lcd.p1.d ${OBJECTDIR}/protocol.p1.d ${OBJECTDIR}/firmware.p1.d ${OBJECTDIR}/protocol_module.p1.d ${OBJECTDIR}/protocol_firmware.p1.d ${OBJECTDIR}/status.p1.d ${OBJECTDIR}/protocol_game.p1.d ${OBJECTDIR}/interrupt.p1.d ${OBJECTDIR}/module.p1.d ${OBJECTDIR}/game.p1.d ${OBJECTDIR}/rng.p1.d ${OBJECTDIR}/edgework.p1.d ${OBJECTDIR}/modes/needy.p1.d ${OBJECTDIR}/modes/debug/debug.p1.d ${OBJECTDIR}/modes/maze/maze.p1.d ${OBJECTDIR}/modes/password/password.p1.d ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1.d ${OBJECTDIR}/modes/wires/wires.p1.d ${OBJECTDIR}/modes/keys/keys.p1.d ${OBJECTDIR}/modes/simon/simon.p1.d

# Object Files
OBJECTFILES=${OBJECTDIR}/modes/blank/blank.p1 ${OBJECTDIR}/modes/bootstrap/bootstrap.p1 ${OBJECTDIR}/modes/controller/controller.p1 ${OBJECTDIR}/peripherals/timer/segment.p1 ${OBJECTDIR}/peripherals/keymatrix.p1 ${OBJECTDIR}/peripherals/ports.p1 ${OBJECTDIR}/peripherals/pwmled.p1 ${OBJECTDIR}/main.p1 ${OBJECTDIR}/nvm.p1 ${OBJECTDIR}/argb.p1 ${OBJECTDIR}/buzzer.p1 ${OBJECTDIR}/can.p1 ${OBJECTDIR}/tick.p1 ${OBJECTDIR}/mode.p1 ${OBJECTDIR}/lcd.p1 ${OBJECTDIR}/protocol.p1 ${OBJECTDIR}/firmware.p1 ${OBJECTDIR}/protocol_module.p1 ${OBJECTDIR}/protocol_firmware.p1 ${OBJECTDIR}/status.p1 ${OBJECTDIR}/protocol_game.p1 ${OBJECTDIR}/interrupt.p1 ${OBJECTDIR}/module.p1 ${OBJECTDIR}/game.p1 ${OBJECTDIR}/rng.p1 ${OBJECTDIR}/edgework.p1 ${OBJECTDIR}/modes/needy.p1 ${OBJECTDIR}/modes/debug/debug.p1 ${OBJECTDIR}/modes/maze/maze.p1 ${OBJECTDIR}/modes/password/password.p1 ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1 ${OBJECTDIR}/modes/wires/wires.p1 ${OBJECTDIR}/modes/keys/keys.p1 ${OBJECTDIR}/modes/simon/simon.p1

# Source Files
SOURCEFILES=modes/blank/blank.c modes/bootstrap/bootstrap.c modes/controller/controller.c peripherals/timer/segment.c peripherals/keymatrix.c peripherals/ports.c peripherals/pwmled.c main.c nvm.c argb.c buzzer.c can.c tick.c mode.c lcd.c protocol.c firmware.c protocol_module.c protocol_firmware.c status.c protocol_game.c interrupt.c module.c game.c rng.c edgework.c modes/needy.c modes/debug/debug.c modes/maze/maze.c modes/password/password.c modes/whosonfirst/whosonfirst.c modes/wires/wires.c modes/keys/keys.c modes/simon/simon.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=18F57Q84
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/modes/blank/blank.p1: modes/blank/blank.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/blank" 
	@${RM} ${OBJECTDIR}/modes/blank/blank.p1.d 
	@${RM} ${OBJECTDIR}/modes/blank/blank.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/blank/blank.p1 modes/blank/blank.c 
	@-${MV} ${OBJECTDIR}/modes/blank/blank.d ${OBJECTDIR}/modes/blank/blank.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/blank/blank.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/bootstrap/bootstrap.p1: modes/bootstrap/bootstrap.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/bootstrap" 
	@${RM} ${OBJECTDIR}/modes/bootstrap/bootstrap.p1.d 
	@${RM} ${OBJECTDIR}/modes/bootstrap/bootstrap.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/bootstrap/bootstrap.p1 modes/bootstrap/bootstrap.c 
	@-${MV} ${OBJECTDIR}/modes/bootstrap/bootstrap.d ${OBJECTDIR}/modes/bootstrap/bootstrap.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/bootstrap/bootstrap.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/controller/controller.p1: modes/controller/controller.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/controller" 
	@${RM} ${OBJECTDIR}/modes/controller/controller.p1.d 
	@${RM} ${OBJECTDIR}/modes/controller/controller.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/controller/controller.p1 modes/controller/controller.c 
	@-${MV} ${OBJECTDIR}/modes/controller/controller.d ${OBJECTDIR}/modes/controller/controller.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/controller/controller.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/peripherals/timer/segment.p1: peripherals/timer/segment.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/peripherals/timer" 
	@${RM} ${OBJECTDIR}/peripherals/timer/segment.p1.d 
	@${RM} ${OBJECTDIR}/peripherals/timer/segment.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/peripherals/timer/segment.p1 peripherals/timer/segment.c 
	@-${MV} ${OBJECTDIR}/peripherals/timer/segment.d ${OBJECTDIR}/peripherals/timer/segment.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/peripherals/timer/segment.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/peripherals/keymatrix.p1: peripherals/keymatrix.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/peripherals" 
	@${RM} ${OBJECTDIR}/peripherals/keymatrix.p1.d 
	@${RM} ${OBJECTDIR}/peripherals/keymatrix.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/peripherals/keymatrix.p1 peripherals/keymatrix.c 
	@-${MV} ${OBJECTDIR}/peripherals/keymatrix.d ${OBJECTDIR}/peripherals/keymatrix.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/peripherals/keymatrix.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/peripherals/ports.p1: peripherals/ports.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/peripherals" 
	@${RM} ${OBJECTDIR}/peripherals/ports.p1.d 
	@${RM} ${OBJECTDIR}/peripherals/ports.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/peripherals/ports.p1 peripherals/ports.c 
	@-${MV} ${OBJECTDIR}/peripherals/ports.d ${OBJECTDIR}/peripherals/ports.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/peripherals/ports.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/peripherals/pwmled.p1: peripherals/pwmled.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/peripherals" 
	@${RM} ${OBJECTDIR}/peripherals/pwmled.p1.d 
	@${RM} ${OBJECTDIR}/peripherals/pwmled.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/peripherals/pwmled.p1 peripherals/pwmled.c 
	@-${MV} ${OBJECTDIR}/peripherals/pwmled.d ${OBJECTDIR}/peripherals/pwmled.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/peripherals/pwmled.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.p1.d 
	@${RM} ${OBJECTDIR}/main.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/main.p1 main.c 
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/nvm.p1: nvm.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/nvm.p1.d 
	@${RM} ${OBJECTDIR}/nvm.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/nvm.p1 nvm.c 
	@-${MV} ${OBJECTDIR}/nvm.d ${OBJECTDIR}/nvm.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/nvm.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/argb.p1: argb.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/argb.p1.d 
	@${RM} ${OBJECTDIR}/argb.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/argb.p1 argb.c 
	@-${MV} ${OBJECTDIR}/argb.d ${OBJECTDIR}/argb.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/argb.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/buzzer.p1: buzzer.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/buzzer.p1.d 
	@${RM} ${OBJECTDIR}/buzzer.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/buzzer.p1 buzzer.c 
	@-${MV} ${OBJECTDIR}/buzzer.d ${OBJECTDIR}/buzzer.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/buzzer.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/can.p1: can.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/can.p1.d 
	@${RM} ${OBJECTDIR}/can.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/can.p1 can.c 
	@-${MV} ${OBJECTDIR}/can.d ${OBJECTDIR}/can.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/can.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/tick.p1: tick.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/tick.p1.d 
	@${RM} ${OBJECTDIR}/tick.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/tick.p1 tick.c 
	@-${MV} ${OBJECTDIR}/tick.d ${OBJECTDIR}/tick.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/tick.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/mode.p1: mode.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/mode.p1.d 
	@${RM} ${OBJECTDIR}/mode.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/mode.p1 mode.c 
	@-${MV} ${OBJECTDIR}/mode.d ${OBJECTDIR}/mode.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/mode.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/lcd.p1: lcd.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/lcd.p1.d 
	@${RM} ${OBJECTDIR}/lcd.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/lcd.p1 lcd.c 
	@-${MV} ${OBJECTDIR}/lcd.d ${OBJECTDIR}/lcd.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/lcd.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/protocol.p1: protocol.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/protocol.p1.d 
	@${RM} ${OBJECTDIR}/protocol.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/protocol.p1 protocol.c 
	@-${MV} ${OBJECTDIR}/protocol.d ${OBJECTDIR}/protocol.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/protocol.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/firmware.p1: firmware.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/firmware.p1.d 
	@${RM} ${OBJECTDIR}/firmware.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/firmware.p1 firmware.c 
	@-${MV} ${OBJECTDIR}/firmware.d ${OBJECTDIR}/firmware.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/firmware.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/protocol_module.p1: protocol_module.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/protocol_module.p1.d 
	@${RM} ${OBJECTDIR}/protocol_module.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/protocol_module.p1 protocol_module.c 
	@-${MV} ${OBJECTDIR}/protocol_module.d ${OBJECTDIR}/protocol_module.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/protocol_module.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/protocol_firmware.p1: protocol_firmware.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/protocol_firmware.p1.d 
	@${RM} ${OBJECTDIR}/protocol_firmware.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/protocol_firmware.p1 protocol_firmware.c 
	@-${MV} ${OBJECTDIR}/protocol_firmware.d ${OBJECTDIR}/protocol_firmware.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/protocol_firmware.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/status.p1: status.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/status.p1.d 
	@${RM} ${OBJECTDIR}/status.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/status.p1 status.c 
	@-${MV} ${OBJECTDIR}/status.d ${OBJECTDIR}/status.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/status.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/protocol_game.p1: protocol_game.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/protocol_game.p1.d 
	@${RM} ${OBJECTDIR}/protocol_game.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/protocol_game.p1 protocol_game.c 
	@-${MV} ${OBJECTDIR}/protocol_game.d ${OBJECTDIR}/protocol_game.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/protocol_game.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/interrupt.p1: interrupt.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/interrupt.p1.d 
	@${RM} ${OBJECTDIR}/interrupt.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/interrupt.p1 interrupt.c 
	@-${MV} ${OBJECTDIR}/interrupt.d ${OBJECTDIR}/interrupt.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/interrupt.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/module.p1: module.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/module.p1.d 
	@${RM} ${OBJECTDIR}/module.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/module.p1 module.c 
	@-${MV} ${OBJECTDIR}/module.d ${OBJECTDIR}/module.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/module.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/game.p1: game.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/game.p1.d 
	@${RM} ${OBJECTDIR}/game.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/game.p1 game.c 
	@-${MV} ${OBJECTDIR}/game.d ${OBJECTDIR}/game.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/game.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/rng.p1: rng.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/rng.p1.d 
	@${RM} ${OBJECTDIR}/rng.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/rng.p1 rng.c 
	@-${MV} ${OBJECTDIR}/rng.d ${OBJECTDIR}/rng.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/rng.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/edgework.p1: edgework.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/edgework.p1.d 
	@${RM} ${OBJECTDIR}/edgework.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/edgework.p1 edgework.c 
	@-${MV} ${OBJECTDIR}/edgework.d ${OBJECTDIR}/edgework.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/edgework.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/needy.p1: modes/needy.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes" 
	@${RM} ${OBJECTDIR}/modes/needy.p1.d 
	@${RM} ${OBJECTDIR}/modes/needy.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/needy.p1 modes/needy.c 
	@-${MV} ${OBJECTDIR}/modes/needy.d ${OBJECTDIR}/modes/needy.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/needy.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/debug/debug.p1: modes/debug/debug.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/debug" 
	@${RM} ${OBJECTDIR}/modes/debug/debug.p1.d 
	@${RM} ${OBJECTDIR}/modes/debug/debug.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/debug/debug.p1 modes/debug/debug.c 
	@-${MV} ${OBJECTDIR}/modes/debug/debug.d ${OBJECTDIR}/modes/debug/debug.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/debug/debug.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/maze/maze.p1: modes/maze/maze.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/maze" 
	@${RM} ${OBJECTDIR}/modes/maze/maze.p1.d 
	@${RM} ${OBJECTDIR}/modes/maze/maze.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/maze/maze.p1 modes/maze/maze.c 
	@-${MV} ${OBJECTDIR}/modes/maze/maze.d ${OBJECTDIR}/modes/maze/maze.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/maze/maze.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/password/password.p1: modes/password/password.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/password" 
	@${RM} ${OBJECTDIR}/modes/password/password.p1.d 
	@${RM} ${OBJECTDIR}/modes/password/password.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/password/password.p1 modes/password/password.c 
	@-${MV} ${OBJECTDIR}/modes/password/password.d ${OBJECTDIR}/modes/password/password.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/password/password.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1: modes/whosonfirst/whosonfirst.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/whosonfirst" 
	@${RM} ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1.d 
	@${RM} ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1 modes/whosonfirst/whosonfirst.c 
	@-${MV} ${OBJECTDIR}/modes/whosonfirst/whosonfirst.d ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/wires/wires.p1: modes/wires/wires.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/wires" 
	@${RM} ${OBJECTDIR}/modes/wires/wires.p1.d 
	@${RM} ${OBJECTDIR}/modes/wires/wires.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/wires/wires.p1 modes/wires/wires.c 
	@-${MV} ${OBJECTDIR}/modes/wires/wires.d ${OBJECTDIR}/modes/wires/wires.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/wires/wires.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/keys/keys.p1: modes/keys/keys.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/keys" 
	@${RM} ${OBJECTDIR}/modes/keys/keys.p1.d 
	@${RM} ${OBJECTDIR}/modes/keys/keys.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/keys/keys.p1 modes/keys/keys.c 
	@-${MV} ${OBJECTDIR}/modes/keys/keys.d ${OBJECTDIR}/modes/keys/keys.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/keys/keys.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/simon/simon.p1: modes/simon/simon.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/simon" 
	@${RM} ${OBJECTDIR}/modes/simon/simon.p1.d 
	@${RM} ${OBJECTDIR}/modes/simon/simon.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c  -D__DEBUG=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/simon/simon.p1 modes/simon/simon.c 
	@-${MV} ${OBJECTDIR}/modes/simon/simon.d ${OBJECTDIR}/modes/simon/simon.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/simon/simon.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
else
${OBJECTDIR}/modes/blank/blank.p1: modes/blank/blank.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/blank" 
	@${RM} ${OBJECTDIR}/modes/blank/blank.p1.d 
	@${RM} ${OBJECTDIR}/modes/blank/blank.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/blank/blank.p1 modes/blank/blank.c 
	@-${MV} ${OBJECTDIR}/modes/blank/blank.d ${OBJECTDIR}/modes/blank/blank.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/blank/blank.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/bootstrap/bootstrap.p1: modes/bootstrap/bootstrap.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/bootstrap" 
	@${RM} ${OBJECTDIR}/modes/bootstrap/bootstrap.p1.d 
	@${RM} ${OBJECTDIR}/modes/bootstrap/bootstrap.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/bootstrap/bootstrap.p1 modes/bootstrap/bootstrap.c 
	@-${MV} ${OBJECTDIR}/modes/bootstrap/bootstrap.d ${OBJECTDIR}/modes/bootstrap/bootstrap.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/bootstrap/bootstrap.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/controller/controller.p1: modes/controller/controller.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/controller" 
	@${RM} ${OBJECTDIR}/modes/controller/controller.p1.d 
	@${RM} ${OBJECTDIR}/modes/controller/controller.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/controller/controller.p1 modes/controller/controller.c 
	@-${MV} ${OBJECTDIR}/modes/controller/controller.d ${OBJECTDIR}/modes/controller/controller.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/controller/controller.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/peripherals/timer/segment.p1: peripherals/timer/segment.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/peripherals/timer" 
	@${RM} ${OBJECTDIR}/peripherals/timer/segment.p1.d 
	@${RM} ${OBJECTDIR}/peripherals/timer/segment.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/peripherals/timer/segment.p1 peripherals/timer/segment.c 
	@-${MV} ${OBJECTDIR}/peripherals/timer/segment.d ${OBJECTDIR}/peripherals/timer/segment.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/peripherals/timer/segment.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/peripherals/keymatrix.p1: peripherals/keymatrix.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/peripherals" 
	@${RM} ${OBJECTDIR}/peripherals/keymatrix.p1.d 
	@${RM} ${OBJECTDIR}/peripherals/keymatrix.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/peripherals/keymatrix.p1 peripherals/keymatrix.c 
	@-${MV} ${OBJECTDIR}/peripherals/keymatrix.d ${OBJECTDIR}/peripherals/keymatrix.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/peripherals/keymatrix.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/peripherals/ports.p1: peripherals/ports.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/peripherals" 
	@${RM} ${OBJECTDIR}/peripherals/ports.p1.d 
	@${RM} ${OBJECTDIR}/peripherals/ports.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/peripherals/ports.p1 peripherals/ports.c 
	@-${MV} ${OBJECTDIR}/peripherals/ports.d ${OBJECTDIR}/peripherals/ports.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/peripherals/ports.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/peripherals/pwmled.p1: peripherals/pwmled.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/peripherals" 
	@${RM} ${OBJECTDIR}/peripherals/pwmled.p1.d 
	@${RM} ${OBJECTDIR}/peripherals/pwmled.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/peripherals/pwmled.p1 peripherals/pwmled.c 
	@-${MV} ${OBJECTDIR}/peripherals/pwmled.d ${OBJECTDIR}/peripherals/pwmled.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/peripherals/pwmled.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/main.p1: main.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.p1.d 
	@${RM} ${OBJECTDIR}/main.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/main.p1 main.c 
	@-${MV} ${OBJECTDIR}/main.d ${OBJECTDIR}/main.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/main.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/nvm.p1: nvm.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/nvm.p1.d 
	@${RM} ${OBJECTDIR}/nvm.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/nvm.p1 nvm.c 
	@-${MV} ${OBJECTDIR}/nvm.d ${OBJECTDIR}/nvm.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/nvm.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/argb.p1: argb.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/argb.p1.d 
	@${RM} ${OBJECTDIR}/argb.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/argb.p1 argb.c 
	@-${MV} ${OBJECTDIR}/argb.d ${OBJECTDIR}/argb.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/argb.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/buzzer.p1: buzzer.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/buzzer.p1.d 
	@${RM} ${OBJECTDIR}/buzzer.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/buzzer.p1 buzzer.c 
	@-${MV} ${OBJECTDIR}/buzzer.d ${OBJECTDIR}/buzzer.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/buzzer.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/can.p1: can.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/can.p1.d 
	@${RM} ${OBJECTDIR}/can.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/can.p1 can.c 
	@-${MV} ${OBJECTDIR}/can.d ${OBJECTDIR}/can.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/can.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/tick.p1: tick.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/tick.p1.d 
	@${RM} ${OBJECTDIR}/tick.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/tick.p1 tick.c 
	@-${MV} ${OBJECTDIR}/tick.d ${OBJECTDIR}/tick.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/tick.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/mode.p1: mode.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/mode.p1.d 
	@${RM} ${OBJECTDIR}/mode.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/mode.p1 mode.c 
	@-${MV} ${OBJECTDIR}/mode.d ${OBJECTDIR}/mode.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/mode.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/lcd.p1: lcd.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/lcd.p1.d 
	@${RM} ${OBJECTDIR}/lcd.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/lcd.p1 lcd.c 
	@-${MV} ${OBJECTDIR}/lcd.d ${OBJECTDIR}/lcd.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/lcd.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/protocol.p1: protocol.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/protocol.p1.d 
	@${RM} ${OBJECTDIR}/protocol.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/protocol.p1 protocol.c 
	@-${MV} ${OBJECTDIR}/protocol.d ${OBJECTDIR}/protocol.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/protocol.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/firmware.p1: firmware.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/firmware.p1.d 
	@${RM} ${OBJECTDIR}/firmware.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/firmware.p1 firmware.c 
	@-${MV} ${OBJECTDIR}/firmware.d ${OBJECTDIR}/firmware.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/firmware.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/protocol_module.p1: protocol_module.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/protocol_module.p1.d 
	@${RM} ${OBJECTDIR}/protocol_module.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/protocol_module.p1 protocol_module.c 
	@-${MV} ${OBJECTDIR}/protocol_module.d ${OBJECTDIR}/protocol_module.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/protocol_module.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/protocol_firmware.p1: protocol_firmware.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/protocol_firmware.p1.d 
	@${RM} ${OBJECTDIR}/protocol_firmware.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/protocol_firmware.p1 protocol_firmware.c 
	@-${MV} ${OBJECTDIR}/protocol_firmware.d ${OBJECTDIR}/protocol_firmware.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/protocol_firmware.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/status.p1: status.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/status.p1.d 
	@${RM} ${OBJECTDIR}/status.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/status.p1 status.c 
	@-${MV} ${OBJECTDIR}/status.d ${OBJECTDIR}/status.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/status.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/protocol_game.p1: protocol_game.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/protocol_game.p1.d 
	@${RM} ${OBJECTDIR}/protocol_game.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/protocol_game.p1 protocol_game.c 
	@-${MV} ${OBJECTDIR}/protocol_game.d ${OBJECTDIR}/protocol_game.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/protocol_game.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/interrupt.p1: interrupt.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/interrupt.p1.d 
	@${RM} ${OBJECTDIR}/interrupt.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/interrupt.p1 interrupt.c 
	@-${MV} ${OBJECTDIR}/interrupt.d ${OBJECTDIR}/interrupt.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/interrupt.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/module.p1: module.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/module.p1.d 
	@${RM} ${OBJECTDIR}/module.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/module.p1 module.c 
	@-${MV} ${OBJECTDIR}/module.d ${OBJECTDIR}/module.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/module.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/game.p1: game.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/game.p1.d 
	@${RM} ${OBJECTDIR}/game.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/game.p1 game.c 
	@-${MV} ${OBJECTDIR}/game.d ${OBJECTDIR}/game.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/game.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/rng.p1: rng.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/rng.p1.d 
	@${RM} ${OBJECTDIR}/rng.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/rng.p1 rng.c 
	@-${MV} ${OBJECTDIR}/rng.d ${OBJECTDIR}/rng.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/rng.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/edgework.p1: edgework.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/edgework.p1.d 
	@${RM} ${OBJECTDIR}/edgework.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/edgework.p1 edgework.c 
	@-${MV} ${OBJECTDIR}/edgework.d ${OBJECTDIR}/edgework.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/edgework.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/needy.p1: modes/needy.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes" 
	@${RM} ${OBJECTDIR}/modes/needy.p1.d 
	@${RM} ${OBJECTDIR}/modes/needy.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/needy.p1 modes/needy.c 
	@-${MV} ${OBJECTDIR}/modes/needy.d ${OBJECTDIR}/modes/needy.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/needy.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/debug/debug.p1: modes/debug/debug.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/debug" 
	@${RM} ${OBJECTDIR}/modes/debug/debug.p1.d 
	@${RM} ${OBJECTDIR}/modes/debug/debug.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/debug/debug.p1 modes/debug/debug.c 
	@-${MV} ${OBJECTDIR}/modes/debug/debug.d ${OBJECTDIR}/modes/debug/debug.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/debug/debug.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/maze/maze.p1: modes/maze/maze.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/maze" 
	@${RM} ${OBJECTDIR}/modes/maze/maze.p1.d 
	@${RM} ${OBJECTDIR}/modes/maze/maze.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/maze/maze.p1 modes/maze/maze.c 
	@-${MV} ${OBJECTDIR}/modes/maze/maze.d ${OBJECTDIR}/modes/maze/maze.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/maze/maze.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/password/password.p1: modes/password/password.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/password" 
	@${RM} ${OBJECTDIR}/modes/password/password.p1.d 
	@${RM} ${OBJECTDIR}/modes/password/password.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/password/password.p1 modes/password/password.c 
	@-${MV} ${OBJECTDIR}/modes/password/password.d ${OBJECTDIR}/modes/password/password.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/password/password.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1: modes/whosonfirst/whosonfirst.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/whosonfirst" 
	@${RM} ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1.d 
	@${RM} ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1 modes/whosonfirst/whosonfirst.c 
	@-${MV} ${OBJECTDIR}/modes/whosonfirst/whosonfirst.d ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/whosonfirst/whosonfirst.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/wires/wires.p1: modes/wires/wires.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/wires" 
	@${RM} ${OBJECTDIR}/modes/wires/wires.p1.d 
	@${RM} ${OBJECTDIR}/modes/wires/wires.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/wires/wires.p1 modes/wires/wires.c 
	@-${MV} ${OBJECTDIR}/modes/wires/wires.d ${OBJECTDIR}/modes/wires/wires.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/wires/wires.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/keys/keys.p1: modes/keys/keys.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/keys" 
	@${RM} ${OBJECTDIR}/modes/keys/keys.p1.d 
	@${RM} ${OBJECTDIR}/modes/keys/keys.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/keys/keys.p1 modes/keys/keys.c 
	@-${MV} ${OBJECTDIR}/modes/keys/keys.d ${OBJECTDIR}/modes/keys/keys.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/keys/keys.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
${OBJECTDIR}/modes/simon/simon.p1: modes/simon/simon.c  nbproject/Makefile-${CND_CONF}.mk 
	@${MKDIR} "${OBJECTDIR}/modes/simon" 
	@${RM} ${OBJECTDIR}/modes/simon/simon.p1.d 
	@${RM} ${OBJECTDIR}/modes/simon/simon.p1 
	${MP_CC} $(MP_EXTRA_CC_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -c   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -DXPRJ_default=$(CND_CONF)  -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits $(COMPARISON_BUILD)  -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto     -o ${OBJECTDIR}/modes/simon/simon.p1 modes/simon/simon.c 
	@-${MV} ${OBJECTDIR}/modes/simon/simon.d ${OBJECTDIR}/modes/simon/simon.p1.d 
	@${FIXDEPS} ${OBJECTDIR}/modes/simon/simon.p1.d $(SILENT) -rsi ${MP_CC_DIR}../  
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.map  -D__DEBUG=1  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto -Wl,-Pflasher=01f800h        $(COMPARISON_BUILD) -Wl,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -o dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}     
	@${RM} dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.hex 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -mcpu=$(MP_PROCESSOR_OPTION) -Wl,-Map=dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.map  -DXPRJ_default=$(CND_CONF)  -Wl,--defsym=__MPLAB_BUILD=1   -mdfp="${DFP_DIR}/xc8"  -fno-short-double -fno-short-float -memi=wordwrite -mrom=0-fbff -O2 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -msummary=-psect,-class,+mem,-hex,-file  -ginhx032 -Wl,--data-init -mno-keep-startup -mno-download -mdefault-config-bits -std=c99 -gdwarf-3 -mstack=compiled:auto:auto:auto -Wl,-Pflasher=01f800h     $(COMPARISON_BUILD) -Wl,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -o dist/${CND_CONF}/${IMAGE_TYPE}/ControlFirmware.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}     
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
