//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Blinky
// Application Overview - The objective of this application is to showcase the 
//                        GPIO control using Driverlib api calls. The LEDs 
//                        connected to the GPIOs on the LP are used to indicate 
//                        the GPIO output. The GPIOs are driven high-low 
//                        periodically in order to turn on-off the LEDs.
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup blinky
//! @{
//
//****************************************************************************

// Standard includes
#include <stdio.h>
#include <stdlib.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"

// Common interface includes
#include "gpio_if.h"
#include "uart_if.h"

#include "pin_mux_config.h"

#define APPLICATION_VERSION     "1.4.0"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

unsigned int g_uiSW2Port = 0,g_uiSW3Port = 0;
unsigned char g_ucSW2Pin,g_ucSW3Pin;

#define GPIO_SW2 22
#define GPIO_SW3 13

unsigned int g_ui28Port = 0;
unsigned char g_uc28Pin;
#define GPIO_28 28

#define FRAME_DELAY 5000000

void (*current_routine)(void);
typedef enum
{
    NOP,
    BLINKY,
    COUNT,
} Event;
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************
Event GetEvent();
void CountRoutine();
void NOPRoutine();

void LEDBlinkyRoutine();
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************

Event GetEvent()
{
    unsigned char ucSWStatus = GPIO_IF_Get(GPIO_SW2, g_uiSW2Port, g_ucSW2Pin);
    if (ucSWStatus > 0) {
        Message("SW2 Pressed\r\n");
        current_routine = LEDBlinkyRoutine;
        return BLINKY;
    }
    ucSWStatus = GPIO_IF_Get(GPIO_SW3, g_uiSW3Port, g_ucSW3Pin);
    if (ucSWStatus > 0) {
        Message("SW3 Pressed\r\n");
        current_routine = CountRoutine;
        return COUNT;
    }
    return NOP;
}

void
LEDDisplayNumber(char num)
{
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    if (num & 1 == 1)
    {
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    }
    num >>= 1;
    if (num & 1 == 1)
    {
        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    }
    num >>= 1;
    if (num & 1 == 1)
    {
        GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    }
    num >>= 1;
}

void LEDBlinkyRoutine()
{
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    Event event = NOP;
    for (; event == NOP || event == BLINKY; event = GetEvent())
    {
        MAP_UtilsDelay(FRAME_DELAY);
        GPIO_IF_LedOn(MCU_ALL_LED_IND);
        MAP_UtilsDelay(FRAME_DELAY);
        GPIO_IF_LedOff(MCU_ALL_LED_IND);
    }
}

void CountRoutine()
{
    Event event = NOP;
    char num = 0;
    for (; event == NOP || event == COUNT; event = GetEvent())
    {
        LEDDisplayNumber(num);
        MAP_UtilsDelay(FRAME_DELAY);
        num++;
    }
}

void NOPRoutine()
{
    Event event = NOP;
    for (; event == NOP; event = GetEvent())
    {
    }
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//! This function  
//!    1. Invokes the LEDBlinkyTask
//!
//! \return None.
//
//****************************************************************************
int
main()
{
    BoardInit();
    
    // Power on the corresponding GPIO port B for 9,10,11.
    // Set up the GPIO lines to mode 0 (GPIO)
    PinMuxConfig();
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    // Set up the GPIO for switches
    GPIO_IF_GetPortNPin(GPIO_SW2,
                        &g_uiSW2Port,
                        &g_ucSW2Pin);
    GPIO_IF_GetPortNPin(GPIO_SW3,
                        &g_uiSW3Port,
                        &g_ucSW3Pin);

    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    InitTerm();
    ClearTerm();
    Message("****************************************************\r\n\n"
            "CC3200 GPIO Application\r\n\n"
            "****************************************************\r\n\n"
            "****************************************************\r\n\n"
            "Push SW3 to start LED binary counting\r\n\n"
            "Push SW2 to blink LEDs on and off\r\n\n"
            "****************************************************\r\n\n");

    current_routine = NOPRoutine;
    while(1)
    {
        current_routine();
    }

    return 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
