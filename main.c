#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/adc.h"
#include "GPIO/gpio.h"
#include "GPIO/Pins.h"

//*****************************************************************************
//
// Counter to count the number of interrupts that have been called.
//
//*****************************************************************************
volatile uint32_t g_ui32Counter = 0;
uint32_t pui32ADC0Value[1];

//*****************************************************************************
//
// The interrupt handler for the for Systick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update the Systick interrupt counter.
    //
    g_ui32Counter++;

    //
    // Trigger the ADC conversion.
    //
    ADCProcessorTrigger(ADC0_BASE, 3);
}

int 
main(void)
{
    uint32_t ui32PrevCount = 0;

    float vcc = 3.3;
    float v_avg = 0;

    //
    // Set the clocking to run directly from the external crystal/oscillator.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    //
    // Set up the period for the SysTick timer.  The SysTick timer period will
    // be equal to the system clock, resulting in a period of 1 second.
    //
    SysTickPeriodSet(SysCtlClockGet());

    //
    // Initialize the interrupt counter.
    //
    g_ui32Counter = 0;

    //
    // Enable interrupts to the processor.
    //
    IntMasterEnable();

    //
    // Enable the SysTick Interrupt.
    //
    SysTickIntEnable();

    //
    // Enable SysTick.
    //
    SysTickEnable();

    //
    // The ADC0 peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    //
    // ADC0 is used with AIN0 on port E7.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);

    //
    // Enable sample sequence 3 with a processor signal trigger.  Sequence 3
    // will do a single sample when the processor sends a signal to start the
    // conversion.
    //
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

    //
    // Configure step 0 on sequence 3.  Sample channel 0 (ADC_CTL_CH0) in
    // single-ended mode (default) and configure the interrupt flag
    // (ADC_CTL_IE) to be set when the sample is done.  Tell the ADC logic
    // that this is the last conversion on sequence 3 (ADC_CTL_END).  Sequence
    // 3 has only one programmable step.  Sequence 1 and 2 have 4 steps, and
    // sequence 0 has 8 programmable steps.  Since we are only doing a single
    // conversion using sequence 3 we will only configure step 0.
    //
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE |
                             ADC_CTL_END);

    //
    // Since sample sequence 3 is now configured, it must be enabled.
    //
    ADCSequenceEnable(ADC0_BASE, 3);

    //
    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
    //
    ADCIntClear(ADC0_BASE, 3);

    GPIO_PinMode(PORTE_ID,PIN1_ID,DIGITAL_OUTPUT);
    GPIO_WritePin(PORTE_ID,PIN1_ID,PIN_LOW);

    GPIO_PinMode(PORTF_ID,PIN1_ID,DIGITAL_OUTPUT);
    GPIO_WritePin(PORTF_ID,PIN1_ID,PIN_LOW);

    //
    // Loop forever while the SysTick runs.
    //
    while(1)
    {

        //
        // Wait for conversion to be completed.
        //
        while(!ADCIntStatus(ADC0_BASE, 3, false))
        {
        }

        //
        // Clear the ADC interrupt flag.
        //
        ADCIntClear(ADC0_BASE, 3);

        //
        // Read ADC Value.
        //
        ADCSequenceDataGet(ADC0_BASE, 3, pui32ADC0Value);

        if(ui32PrevCount != g_ui32Counter)
        {
            ui32PrevCount = g_ui32Counter;

            v_avg += pui32ADC0Value[0] * (vcc/1023);
        }

        if(g_ui32Counter == 10)
        {
            if ((v_avg/10) > vcc*(3/4)){
            GPIO_WritePin(PORTE_ID,PIN1_ID,PIN_HIGH);
            GPIO_WritePin(PORTF_ID,PIN1_ID,PIN_HIGH);
            }
            if ( (v_avg/10) < vcc*(1/4) ){
                GPIO_WritePin(PORTE_ID,PIN1_ID,PIN_LOW);
            }
            if ( (v_avg/10) < vcc*(3/4) ){
                GPIO_WritePin(PORTF_ID,PIN1_ID,PIN_LOW);
            }

            g_ui32Counter = 0;
            v_avg = 0;
        }
    }
}